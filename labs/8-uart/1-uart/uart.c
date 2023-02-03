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
    AUX_BASE = 0x20215040,
    aux_mu_io_reg = (AUX_BASE + 0x00),
    aux_mu_iir_reg = (AUX_BASE + 0x08),
    aux_mu_cntl_reg = (AUX_BASE + 0x20),

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

    uint32_t aux_enable_reg = GET32(AUX_ENABLES);
    aux_enable_reg |= 0b1;
    PUT32(AUX_ENABLES, aux_enable_reg);

    dev_barrier();

    uint32_t aux_cntl_reg = GET32(aux_mu_cntl_reg);
    aux_cntl_reg &= 0b00;
    PUT32(aux_mu_cntl_reg, aux_cntl_reg);

    dev_barrier();
    
    uint32_t aux_iir_reg = GET32(aux_mu_iir_reg);
    aux_iir_reg |= 0b110;
    PUT32(aux_mu_iir_reg, aux_iir_reg);

}

// disable the uart.
void uart_disable(void) {
}


// returns one byte from the rx queue, if needed
// blocks until there is one.
int uart_get8(void) {
	return 0;
}

// 1 = space to put at least one byte, 0 otherwise.
int uart_can_put8(void) {
    return 0;
}

// put one byte on the tx qqueue, if needed, blocks
// until TX has space.
// returns < 0 on error.
int uart_put8(uint8_t c) {
    return 0;
}

// simple wrapper routines useful later.

// 1 = at least one byte on rx queue, 0 otherwise
int uart_has_data(void) {
    todo("must implement\n");
}

// return -1 if no data, otherwise the byte.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// 1 = tx queue empty, 0 = not empty.
int uart_tx_is_empty(void) {
    unimplemented();
}

// flush out all bytes in the uart --- we use this when 
// turning it off / on, etc.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty())
        ;
}
