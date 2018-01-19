# Potential Future Issues

## M1

* Timer callbacks of < 5ms cause the do_while loop in the timer driver to never exit, thus the system is hung processing timer events and you will get a bug where the bottom 32 bits rollover but the top does not, because we dont run the code to handle it.

* If you make interrupt handling multithreaded in the future, make sure to lock the Priority queue.

## M2

* Might want to refactor the free_frames buffer into a general stack data structure
* If we want to share or free the frame table later, we need to keep track of the capabilties for it

# Design Features

## 1. Memory Management
An abstraction layer exists to hide the dirty details of untyped memory.
The key interface functions provided to students include

steal_mem(sizebits) -> steals 1<<sizebits bytes of memory
get_mem(&low, &high) -> returns the low and high addresses of available memory
 
It also has a wrapper for UntypedRetype for convenience 

## 2. DMA memory
The main initialisation function calls steal_mem early to reserve memory for dma_malloc.
It is important to call steal_mem as the size of DMA memory is much larger than the 16K
max allocation size that will be supported by the memory manager. It is important to 
do this early to ensure the frame table has not been initialised yet and also to reduce
fragmentation cause be alignment. 

## 3. CSpace
the cspace of sos is managed by libcsm. 2nd Level CNodes are limited in size to be
the euivelent size of a page directory (16K) to simplify object allocation
single level cspaces for user apps are exposed to the students who must manage
this themselves (need only include EP caps for sos, timer_server, pager? etc).
These caps can be at a fixed offset and therefore fee/allocated slots need not
be managed.

- Circular dependancy:
to allocate an object, we need to reserve memory and allocate a cap. The process
of allocating a cap may require more memory to be reserved for a new CNode. By
seperating the memory allocation from the object retype/creating, we remove this
circular dependancy.

The interface is a simple "sos_cspace_init" which does magic behind the scenes
to transform the cspace given the restrictions mentioned above. From here, the
students will use the libcsm interface to allocate/copy/free caps as needed

## 4. Process Management
No process management
the function "create_initial_thread" is too long! but it serves as a step by step 
guide of how to setup a thread. It does not do any book keeping at all and
maps in a single 4K page of stack as no pager is implemented.

There is no interface here

To initialise the system and intial threads, a skeleton frame table is implemented
with a alloc function but no free. Alloc does nothing but reserve memory and return
base address. It is up to the caller to retype the memory aquired.
