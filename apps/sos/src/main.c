/*
 * Dreamy OS
 *
 * Cameron Lonsdale & Glenn McGuire
 */

#define verbose 5

#include <autoconf.h>
#include <clock/clock.h>
#include <cspace/cspace.h>
#include <cpio/cpio.h>
#include <dev/sos_serial.h>
#include <elf/elf.h>
#include <fs/sos_nfs.h>
#include <nfs/nfs.h>
#include <sys/debug.h>
#include <sys/panic.h>
#include <syscall/syscall.h>
#include <utils/page.h>
#include <utils/util.h>
#include <vfs/vfs.h>
#include <vm/vm.h>
#include <vm/frametable.h>

#include "mapping.h"
#include "network.h"
#include "picoro.h"
#include <proc/elf.h>
#include <proc/proc.h>
#include <ut_manager/ut.h>
#include <vm/layout.h>

/* For unit tests: REMOVE BEFORE FINAL SUBMISSION */
#include "tests.h"

/* 
 * This is the index where a clients syscall enpoint will
 * be stored in the clients cspace.
 */
#define USER_EP_CAP (1)

/* 
 * To differencient between async and and sync IPC, we assign a
 * badge to the async endpoint. The badge that we receive will
 * be the bitwise 'OR' of the async endpoint badge and the badges
 * of all pending notifications.
 */
#define IRQ_EP_BADGE (1 << (seL4_BadgeBits - 1))

/* 
 * All badged IRQs set high bet, then we use uniq bits to
 * distinguish interrupt sources
 */
#define IRQ_BADGE_NETWORK (1 << 0)
#define IRQ_BADGE_TIMER (1 << 1)

#define TTY_NAME CONFIG_SOS_STARTUP_APP

/*
 * The linker will link this symbol to the start address
 * of an archive of attached applications.
 */
extern char _cpio_archive[];

const seL4_BootInfo *_boot_info;

/*
 * End point capabilities
 */
seL4_CPtr _sos_ipc_ep_cap;
seL4_CPtr _sos_interrupt_ep_cap;

/* 
 * Syscall coroutine
 */
coro syscall_coro = NULL;

/*
 * Private functions
 */
static void sos_init(seL4_CPtr *ipc_ep, seL4_CPtr *async_ep);
static void sos_ipc_init(seL4_CPtr *ipc_ep, seL4_CPtr *async_ep);
static void sos_driver_init(void);
static seL4_CPtr badge_irq_ep(seL4_CPtr ep, seL4_Word badge);

/*
 * Print the startup logo for the operating system
 */
void
print_startup(void)
{
    dprintf(0, "\n\n\n\n");
    dprintf(0,
"           /$$                                                                   /$$$$$$   /$$$$$$  \n"
"          | $$                                                                  /$$__  $$ /$$__  $$ \n"
" /$$  /$$$$$$$  /$$$$$$   /$$$$$$   /$$$$$$  /$$$$$$/$$$$  /$$   /$$ /$$       | $$  \\ $$| $$  \\__/\n"
"|__/ /$$__  $$ /$$__  $$ /$$__  $$ |____  $$| $$_  $$_  $$| $$  | $$|__/       | $$  | $$|  $$$$$$  \n"
"    | $$  | $$| $$  \\__/| $$$$$$$$  /$$$$$$$| $$ \\ $$ \\ $$| $$  | $$           | $$  | $$ \\____  $$\n"
" /$$| $$  | $$| $$      | $$_____/ /$$__  $$| $$ | $$ | $$| $$  | $$ /$$       | $$  | $$ /$$  \\ $$\n"
"|__/|  $$$$$$$| $$      |  $$$$$$$|  $$$$$$$| $$ | $$ | $$|  $$$$$$$|__/       |  $$$$$$/|  $$$$$$/ \n"
"     \\_______/|__/       \\_______/ \\_______/|__/ |__/ |__/ \\____  $$            \\______/  \\______/ \n"
"                                                           /$$  | $$                               \n"
"                                                          |  $$$$$$/                                \n"
"                                                           \\______/                                \n");
    dprintf(0, "\n\n");
    dprintf(0, "                                    Glenn McGuire & Cameron Lonsdale                    \n");
    dprintf(0, "                                                AOS 2017                                \n");
    dprintf(0, "\n\n\n\n");
}

/*
 * Event handler loop
 * @param ep, the endpoint where messages come in
 */
void
event_loop(const seL4_CPtr ep)
{
    seL4_Word badge;
    seL4_Word label;
    seL4_MessageInfo_t message;

    while (TRUE) {
        message = seL4_Wait(ep, &badge);
        label = seL4_MessageInfo_get_label(message);
        if (badge & IRQ_EP_BADGE) {
            /* Interrupt */
            if (badge & IRQ_BADGE_NETWORK)
                network_irq();

            /* Needs to be an if, interrupts can group together */
            if (badge & IRQ_BADGE_TIMER)
                timer_interrupt();

        } else if (label == seL4_VMFault) {
            vm_fault();
        } else if (label == seL4_NoFault) {
            syscall_coro = coroutine(handle_syscall);
            resume(syscall_coro, badge);
        } else {
            LOG_INFO("Rootserver got an unknown message");
        }
    }
}


/*
 * Initialise the IPC endpoints for SOS to talk to other 
 * services running ontop of seL4
 * @param ipc_ep, IPC endpoint
 * @param async_ep, asynchronos endpoint
 */
static void
sos_ipc_init(seL4_CPtr *ipc_ep, seL4_CPtr *async_ep)
{
    seL4_Word ep_addr;
    seL4_Word aep_addr;
    int err;

    /* Create an Async endpoint for interrupts */
    aep_addr = ut_alloc(seL4_EndpointBits);
    conditional_panic(!aep_addr, "No memory for async endpoint");
    err = cspace_ut_retype_addr(aep_addr, seL4_AsyncEndpointObject, seL4_EndpointBits, cur_cspace, async_ep);
    conditional_panic(err, "Failed to allocate c-slot for Interrupt endpoint");

    /* Bind the Async endpoint to our TCB */
    err = seL4_TCB_BindAEP(seL4_CapInitThreadTCB, *async_ep);
    conditional_panic(err, "Failed to bind ASync EP to TCB");

    // TODO: Will each process need its own endpoint?

    /* Create an endpoint for user application IPC */
    ep_addr = ut_alloc(seL4_EndpointBits);
    conditional_panic(!ep_addr, "No memory for endpoint");
    err = cspace_ut_retype_addr(ep_addr, seL4_EndpointObject, seL4_EndpointBits, cur_cspace, ipc_ep);
    conditional_panic(err, "Failed to allocate c-slot for IPC endpoint");
}

/*
 * Initialise SOS's drivers
 * Network, Serial, Timer
 */
static void
sos_driver_init(void)
{
    int err;

    /* Initialise the network hardware */
    network_init(badge_irq_ep(_sos_interrupt_ep_cap, IRQ_BADGE_NETWORK));

    /* Map in the GPT into virtual memory and provide that address to the timer library */
    init_timer(map_device((void *)CLOCK_GPT, CLOCK_GPT_SIZE));

    /* Initialise timer with badged capability */
    err = start_timer(badge_irq_ep(_sos_interrupt_ep_cap, IRQ_BADGE_TIMER));
    conditional_panic(err, "Failed to start the timer\n");

    /* Intialise the serial device and register it with the VFS */
    err = sos_serial_init();
    conditional_panic(err, "Failed to initialise serial driver\n");

    /* Initialise the NFS file system and register with the VFS */
    err = sos_nfs_init();
    conditional_panic(err, "Failed to mount NFS\n");
}

/*
 * Initialise Simple Operating System
 * @param ipc_ep, the IPC endpoint
 * @param async_epc, the asynchronous endpoint
 */
static void
sos_init(seL4_CPtr *ipc_ep, seL4_CPtr *async_ep)
{
    seL4_Word dma_addr;
    seL4_Word low;
    seL4_Word high;
    int err;

    print_startup();

    /* Retrieve boot info from seL4 */
    _boot_info = seL4_GetBootInfo();
    conditional_panic(!_boot_info, "Failed to retrieve boot info\n");
    
    print_bootinfo(_cpio_archive, _boot_info);

    /* Initialise the untyped sub system and reserve memory for DMA */
    err = ut_table_init(_boot_info);
    conditional_panic(err, "Failed to initialise Untyped Table\n");

    /* DMA uses a large amount of memory that will never be freed */
    dma_addr = ut_steal_mem(DMA_SIZE_BITS);
    conditional_panic(dma_addr == (seL4_Word)NULL, "Failed to reserve DMA memory\n");

    /* find available memory */
    ut_find_memory(&low, &high);

    /* 
     * Our frame table allocation has some redundancy as we size it based on a larger block of UT
     * but the simplicity is worth it. We steal memory to place the table contiguosly at a physical address.
     * We then map this into virtual memory in frame_table_init.
     */
    seL4_Word n_pages = BYTES_TO_4K_PAGES(high - low);
    seL4_Word frame_table_size_in_bits = LOG_BASE_2(nearest_power_of_two(sizeof(frame_entry) * n_pages));
    seL4_Word frame_table_paddr = ut_steal_mem(frame_table_size_in_bits);
    conditional_panic(frame_table_paddr == (seL4_Word)NULL, "Failed to reserve frametable memory\n");

    /* Resize memory after we just stole from it */
    ut_find_memory(&low, &high);

    /* Initialise the untyped memory allocator */
    ut_allocator_init(low, high);

    /* Initialise the cspace manager */
    err = cspace_root_task_bootstrap(ut_alloc, ut_free, ut_translate, malloc, free);
    conditional_panic(err, "Failed to initialise the cspace\n");

    /* Initialise DMA memory */
    err = dma_init(dma_addr, DMA_SIZE_BITS);
    conditional_panic(err, "Failed to intiialise DMA memory\n");

    /* Initialise the frame table */
    err = frame_table_init(frame_table_paddr, frame_table_size_in_bits, low, high);
    conditional_panic(err, "Failed to intiialise frame table\n");

    /* Initialise IPC endpoints */
    sos_ipc_init(ipc_ep, async_ep);

    /* Initialise the virtual file system */
    err = vfs_init();
    conditional_panic(err, "Failed to virtual file system\n");

    /* Initialise drivers: Must happen after vfs_init as devices are registered */
    sos_driver_init();
}

/*
 * Badge an endpoint
 * @param ep, the endpoint
 * @param badge
 * @return badged capability
 */
static inline 
seL4_CPtr badge_irq_ep(const seL4_CPtr ep, const seL4_Word badge)
{
    seL4_CPtr badged_cap = cspace_mint_cap(
        cur_cspace, cur_cspace, ep, 
        seL4_AllRights, seL4_CapData_Badge_new(badge | IRQ_EP_BADGE)
    );
    conditional_panic(!badged_cap, "Failed to allocate badged cap");

    return badged_cap;
}

/*
 * Main entry point - called by crt.
 */
int 
main(void)
{
#ifdef SEL4_DEBUG_KERNEL
    seL4_DebugNameThread(seL4_CapInitThreadTCB, "SOS:root");
#endif

    /* Initialise the operating system */
    sos_init(&_sos_ipc_ep_cap, &_sos_interrupt_ep_cap);

    /* Start the user application */
    start_first_process(_cpio_archive, TTY_NAME, _sos_ipc_ep_cap);
    
    /* Unit tests */
    // test_m2();
    // test_m1(); /* After so as to have time to enter event loop */

    /* Wait on synchronous endpoint for IPC */
    LOG_INFO("SOS entering event loop");
    event_loop(_sos_ipc_ep_cap);

    panic("should not be reached");
    return 0;
}
