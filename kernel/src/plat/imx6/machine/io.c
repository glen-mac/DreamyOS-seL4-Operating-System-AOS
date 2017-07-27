/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <stdint.h>
#include <util.h>
#include <machine/io.h>
#include <plat/machine/devices.h>

#if defined DEBUG || defined RELEASE_PRINTF

#define URXD  0x00 /* UART Receiver Register */
#define UTXD  0x40 /* UART Transmitter Register */
#define UCR1  0x80 /* UART Control Register 1 */
#define UCR2  0x84 /* UART Control Register 2 */
#define UCR3  0x88 /* UART Control Register 3 */
#define UCR4  0x8c /* UART Control Register 4 */
#define UFCR  0x90 /* UART FIFO Control Register */
#define USR1  0x94 /* UART Status Register 1 */
#define USR2  0x98 /* UART Status Register 2 */
#define UESC  0x9c /* UART Escape Character Register */
#define UTIM  0xa0 /* UART Escape Timer Register */
#define UBIR  0xa4 /* UART BRM Incremental Register */
#define UBMR  0xa8 /* UART BRM Modulator Register */
#define UBRC  0xac /* UART Baud Rate Counter Register */
#define ONEMS 0xb0 /* UART One Millisecond Register */
#define UTS   0xb4 /* UART Test Register */

#define UART_REG(x) ((volatile uint32_t *)(UART_PPTR + (x)))


#define UART_SR2_TXFIFO_EMPTY 14
#define UART_SR2_RXFIFO_RDR    0

/* SR1 bits */
#define UART_SR1_PARITYERR    BIT(15)
#define UART_SR1_RTSS         BIT(14)
#define UART_SR1_TRDY         BIT(13)
#define UART_SR1_RTSD         BIT(12)
#define UART_SR1_ESCF         BIT(11)
#define UART_SR1_FRAMERR      BIT(10)
#define UART_SR1_RRDY         BIT( 9)
#define UART_SR1_AGTIM        BIT( 8)
#define UART_SR1_DTRD         BIT( 7)
#define UART_SR1_RXDS         BIT( 6)
#define UART_SR1_AIRINT       BIT( 5)
#define UART_SR1_AWAKE        BIT( 4)
#define UART_SR1_SAD          BIT( 3)



void init_serial(void) {
    /* read in any characters */
    uint32_t disable, enable;
    while ((*UART_REG(USR2) & BIT(UART_SR2_RXFIFO_RDR))) {
        uint32_t c = *UART_REG(URXD);
        (void)c;
    }
    *UART_REG(UFCR) = ((*UART_REG(UFCR)) & ~0x3fUL) | 0x1;
    /* disable tx interrupts and enable interrupt on rx ready */
    disable = BIT(15) | BIT(13) | BIT(12) |BIT(6) | BIT(2);
    enable = BIT(9);
    *UART_REG(UCR1) = ((*UART_REG(UCR1)) & (~disable)) | enable;
    disable = BIT(13) | BIT(12) | BIT(11) | BIT(9) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(0);
    enable = 0;
    *UART_REG(UCR3) = ((*UART_REG(UCR3)) & (~disable)) | enable;
    disable = BIT(8) | BIT(7) | BIT(6) | BIT(3) | BIT(2) | BIT(1);
    enable = BIT(0);
    *UART_REG(UCR4) = ((*UART_REG(UCR4)) & (~disable)) | enable;
    disable = BIT(15) | BIT(4) | BIT(3);
    enable = BIT(0);
    *UART_REG(UCR2) = ((*UART_REG(UCR2)) & (~disable)) | enable;
}

void
handle_reset_on_serial(void){
    static char* reset_code_ptr = "reset";
    while(*UART_REG(USR2) & BIT(UART_SR2_RXFIFO_RDR)){
        /* We have a character */
        char c = *UART_REG(URXD);
        /* set to clear interrupt flag */
        *UART_REG(USR1) = UART_SR1_RRDY;
        if(c == *reset_code_ptr){
            reset_code_ptr++;
            if(*reset_code_ptr == '\0'){
                /* sequence found */
                volatile uint32_t *src_reg = (volatile uint32_t*)SRC_PPTR;
                volatile uint16_t *wdt_reg = (volatile uint16_t*)WATCHDOG_PPTR;
                printf("\n\nTrying to restart\n");
                src_reg[0] &= (~ (BIT(22) | BIT(23) | BIT(24)));
                src_reg[8] = 0;
                wdt_reg[0] = BIT(2);
                wdt_reg[0] = BIT(2);
                while(1);
            }
        }else{
            reset_code_ptr = "reset";
        }
    }
}

void
imx6_uart_putchar(char c)
{
    putDebugChar(c);
    if (c == '\n') {
        putDebugChar('\r');
    }
}

void putDebugChar(unsigned char c)
{
    while (!(*UART_REG(USR2) & BIT(UART_SR2_TXFIFO_EMPTY)));
    *UART_REG(UTXD) = c;
}

unsigned char getDebugChar(void)
{
    while (!(*UART_REG(USR2) & BIT(UART_SR2_RXFIFO_RDR)));
    return *UART_REG(URXD);
}

#endif /* DEBUG */
