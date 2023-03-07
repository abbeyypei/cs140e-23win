#include "nrf.h"
#include "nrf-hw-support.h"

// enable crc, enable 2 byte
#   define set_bit(x) (1<<(x))
#   define enable_crc      set_bit(3)
#   define crc_two_byte    set_bit(2)
#   define mask_int         set_bit(6)|set_bit(5)|set_bit(4)
enum {
    // pre-computed: can write into NRF_CONFIG to enable TX.
    tx_config = enable_crc | crc_two_byte | set_bit(PWR_UP) | mask_int,
    // pre-computed: can write into NRF_CONFIG to enable RX.
    rx_config = tx_config  | set_bit(PRIM_RX)
} ;

nrf_t * nrf_init(nrf_conf_t c, uint32_t rxaddr, unsigned acked_p) {
    // nrf_t *n = staff_nrf_init(c, rxaddr, acked_p);
    nrf_t *n = kmalloc(sizeof *n);
    n->config = c;
    nrf_stat_start(n);
    n->spi = pin_init(c.ce_pin, c.spi_chip);
    n->rxaddr = rxaddr;

    nrf_set_pwrup_off(n);

    nrf_put8(n, NRF_FIFO_STATUS, 0b1000);
    nrf_put8(n, NRF_EN_RXADDR, 0);
    nrf_put8(n, NRF_SETUP_RETR, nrf_default_retran_attempts + ((nrf_default_retran_delay / 250 - 1)<<4));

    if (acked_p) {
        nrf_put8(n, NRF_EN_AA, 0b11);
        nrf_put8(n, NRF_EN_RXADDR, 0b11);
    } else {
        nrf_put8(n, NRF_EN_AA, 0b0);
        nrf_put8(n, NRF_EN_RXADDR, 0b10);
    }
    nrf_put8(n, NRF_SETUP_AW, nrf_get_addr_nbytes(n)-2);

    nrf_put8(n, NRF_RX_PW_P1, c.nbytes);
    nrf_set_addr(n, NRF_RX_ADDR_P1, rxaddr, nrf_default_addr_nbytes);
    nrf_set_addr(n, NRF_TX_ADDR, 0, nrf_default_addr_nbytes);
    nrf_put8(n, NRF_RX_PW_P2, 0);
    nrf_put8(n, NRF_RX_PW_P3, 0);
    nrf_put8(n, NRF_RX_PW_P4, 0);
    nrf_put8(n, NRF_RX_PW_P5, 0);

    nrf_put8(n, NRF_RF_CH, c.channel);

    nrf_put8(n, NRF_RF_SETUP, nrf_default_db | nrf_default_data_rate);

    nrf_rx_flush(n);
    nrf_tx_flush(n);

    nrf_set_pwrup_on(n);
    delay_ms(2);
    nrf_put8(n, NRF_CONFIG, rx_config);
    gpio_set_on(n->config.ce_pin);

    delay_ms(2);
        
    // nrf_rx_flush(n);
    // nrf_tx_flush(n);

    // should be true after setup.
    if(acked_p) {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    } else {
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
        nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
        nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
        nrf_opt_assert(n, nrf_tx_fifo_empty(n));
    }
    assert(!nrf_tx_fifo_full(n));
    assert(nrf_tx_fifo_empty(n));
    assert(!nrf_rx_fifo_full(n));
    assert(nrf_rx_fifo_empty(n));
    return n;
}

int nrf_tx_send_ack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {

    int res = 0;

    // default config for acked state.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    while(nrf_get_pkts(n))
        ;

    // TODO: you would implement the rest of the code at this point.
    // int res = staff_nrf_tx_send_ack(n, txaddr, msg, nbytes);
    // set to tx mode
    gpio_set_off(n->config.ce_pin);
    nrf_put8(n, NRF_CONFIG, tx_config);
    gpio_set_on(n->config.ce_pin);
    // delay_ms(2);

    // set addr
    nrf_set_addr(n, NRF_TX_ADDR, txaddr, nrf_default_addr_nbytes);
    nrf_set_addr(n, NRF_RX_ADDR_P0, txaddr, nrf_default_addr_nbytes);

    if (!nrf_putn(n, NRF_W_TX_PAYLOAD, msg, nbytes)) return -1;

    gpio_set_off(n->config.ce_pin);
    gpio_set_on(n->config.ce_pin);

    while (!nrf_tx_fifo_empty(n)) {
        if (nrf_has_tx_intr(n)) {
            nrf_tx_intr_clr(n);
            res = nbytes;
            goto done;
        }

        if (nrf_has_max_rt_intr(n)) {
            nrf_tx_intr_clr(n);
            nrf_dump("max inter", n);
            panic("max intr\n");
        }

        if (nrf_rx_fifo_full(n)) {
            nrf_tx_intr_clr(n);
            panic("rx fifo full \n");
        }
    }

    // uint8_t cnt = nrf_get8(n, NRF_OBSERVE_TX);
    // n->tot_retrans  += bits_get(cnt,0,3);

    // tx interrupt better be cleared.
    done:
        nrf_opt_assert(n, !nrf_has_tx_intr(n));
        // better be back in rx mode.
        gpio_set_off(n->config.ce_pin);
        nrf_put8(n, NRF_CONFIG, rx_config);
        gpio_set_on(n->config.ce_pin);
        delay_ms(2);
        nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
        return res;
}

int nrf_tx_send_noack(nrf_t *n, uint32_t txaddr, 
    const void *msg, unsigned nbytes) {

    // default state for no-ack config.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    nrf_opt_assert(n, !nrf_pipe_is_enabled(n, 0));
    nrf_opt_assert(n, nrf_pipe_is_enabled(n, 1));
    nrf_opt_assert(n, !nrf_pipe_is_acked(n, 1));
    nrf_opt_assert(n, nrf_tx_fifo_empty(n));

    // if interrupts not enabled: make sure we check for packets.
    while(nrf_get_pkts(n))
        ;


    // TODO: you would implement the code here.
    // int res = staff_nrf_tx_send_noack(n, txaddr, msg, nbytes);
    gpio_set_off(n->config.ce_pin);
    nrf_put8(n, NRF_CONFIG, tx_config);
    gpio_set_on(n->config.ce_pin);
    delay_ms(2);

    nrf_set_addr(n, NRF_TX_ADDR, txaddr, nrf_default_addr_nbytes);

    if (!nrf_putn(n, W_TX_PAYLOAD_NO_ACK, msg, nbytes)) return -1;

    gpio_set_off(n->config.ce_pin);
    gpio_set_on(n->config.ce_pin);

    while (!nrf_tx_fifo_empty(n));

    nrf_tx_intr_clr(n);

    // tx interrupt better be cleared.
    nrf_opt_assert(n, !nrf_has_tx_intr(n));

    gpio_set_off(n->config.ce_pin);
    nrf_put8(n, NRF_CONFIG, rx_config);
    gpio_set_on(n->config.ce_pin);
    delay_ms(2);

    // better be back in rx mode.
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return 1;
}

int nrf_get_pkts(nrf_t *n) {
    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);

    // TODO:
    // data sheet gives the sequence to follow to get packets.
    // p63: 
    //    1. read packet through spi.
    //    2. clear IRQ.
    //    3. read fifo status to see if more packets: 
    //       if so, repeat from (1) --- we need to do this now in case
    //       a packet arrives b/n (1) and (2)
    // done when: nrf_rx_fifo_empty(n)
    // int res = staff_nrf_get_pkts(n);
    int cnt = 0;
    while (!nrf_rx_fifo_empty(n))
    {
        unsigned pipeid = nrf_rx_get_pipeid(n);
        assert(pipeid == 1);
        uint8_t msg[n->config.nbytes];
        uint8_t status = nrf_getn(n, NRF_R_RX_PAYLOAD, &msg, n->config.nbytes);
        if (!cq_push_n(&n->recvq, &msg, n->config.nbytes)) return -1;
        n->tot_sent_msgs ++;
        n->tot_sent_bytes += n->config.nbytes;
        cnt ++;
        nrf_rx_intr_clr(n);
    }
    

    nrf_opt_assert(n, nrf_get8(n, NRF_CONFIG) == rx_config);
    return cnt;
}
