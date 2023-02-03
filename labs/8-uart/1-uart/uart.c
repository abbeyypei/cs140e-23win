// implement:
//  void uart_init(void)
//
//  int uart_can_get8(void);
//  int uart_get8(void);
//
//  int uart_can_put8(void);
//  void uart_put8(uint8_t c);
//
//  int uart_tx_is_empty(void) {
//
// see that hello world works.
//
//
#include "rpi.h"

enum {
    AUX_ENABLES = 0x20215004,
    aux_mu_io_reg = 0x20215040,
    aux_mu_ier_reg = 0x20215044,
    aux_mu_iir_reg = 0x20215048,
    aux_mu_lcr_reg = 0x2021504C,
    aux_mu_cntl_reg = 0x20215060,
    aux_mu_stat_reg = 0x20215064,
    aux_mu_baud_reg = 0x20215068,
};

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
    dev_barrier();

    gpio_set_function(GPIO_TX, GPIO_FUNC_ALT5);
    gpio_set_function(GPIO_RX, GPIO_FUNC_ALT5);
    
    dev_barrier();
    // turn on AUX
    uint32_t aux_enable_reg = GET32(AUX_ENABLES);
    aux_enable_reg |= 0b1;
    PUT32(AUX_ENABLES, aux_enable_reg);

    dev_barrier();
    // disable tx/rx
    uint32_t aux_cntl_reg = GET32(aux_mu_cntl_reg);
    aux_cntl_reg &= (~0b11);
    PUT32(aux_mu_cntl_reg, aux_cntl_reg);

    dev_barrier();
    // Disable interrupts
    uint32_t aux_ier_reg = GET32(aux_mu_ier_reg);
    aux_ier_reg &= (~0b11);
    PUT32(aux_mu_ier_reg, aux_ier_reg);

    dev_barrier();
    // Clear FIFO
    uint32_t aux_iir_reg = GET32(aux_mu_iir_reg);
    aux_iir_reg |= 0b110;
    PUT32(aux_mu_iir_reg, aux_iir_reg);

    dev_barrier();
    // Set baud rate
    uint32_t baud_rate = 270;
    PUT32(aux_mu_baud_reg, baud_rate);

    dev_barrier();
    // Set data to 8 bits
    uint32_t aux_lcr_reg = GET32(aux_mu_lcr_reg);
    aux_lcr_reg |= 0b11;
    PUT32(aux_mu_lcr_reg, aux_lcr_reg);

    dev_barrier();
    // no control flow
    aux_cntl_reg = GET32(aux_mu_cntl_reg);
    aux_cntl_reg &= (~0b1100);
    PUT32(aux_mu_cntl_reg, aux_cntl_reg);

    dev_barrier();
    // Enable tx/rx
    aux_cntl_reg = GET32(aux_mu_cntl_reg);
    aux_cntl_reg |= 0b11;
    PUT32(aux_mu_cntl_reg, aux_cntl_reg);

    dev_barrier();
}

// disable the uart.
void uart_disable(void) {
    uart_flush_tx();
    PUT32(AUX_ENABLES, GET32(AUX_ENABLES) & (~0b1));
}


// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_get8(void) {
    while (!uart_has_data());
    dev_barrier();
    uint32_t read = GET32(aux_mu_io_reg);
    dev_barrier();
    return read & 0b11111111;
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_put8(void) {
    dev_barrier();
    uint32_t stat = GET32(aux_mu_stat_reg);
    dev_barrier();
    return ((stat & 0b10) >> 1);
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
// returns < 0 on error.
int uart_put8(uint8_t c) {
    while (!uart_can_put8());
    PUT32(aux_mu_io_reg, (uint32_t)c);
    return 1;
}

// simple wrapper routines useful later.

// 1 = at least one byte on rx queue, 0 otherwise
int uart_has_data(void) {
    dev_barrier();
    uint32_t stat = GET32(aux_mu_stat_reg);
    dev_barrier();
    return ((stat & 0b1));
}

// return -1 if no data, otherwise the byte.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
    dev_barrier();
    uint32_t stat = GET32(aux_mu_stat_reg);
    dev_barrier();
    return ((stat & 0b100000000) >> 8);
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty()) {
    }
}
