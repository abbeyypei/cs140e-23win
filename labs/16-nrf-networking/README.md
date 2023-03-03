## Get two nrf24l01p RF tranceivers to talk to each other.

Today you'll build some code to make the NRF chips we have talk to
each other.   The lab is organized as a fetch-quest where you'll build
the routines to (1) initialize, (2) receive, (3) send non-acked packets,
(4) send acked packets.  This gives you a simple starting point for
networking.  

The code is currently setup so that all the tests *should* pass if you
just run `make check`.
   - ***NOTE: with 50+ people in one room we will have signficant
     RF interference***
   - So: if the tests don't pass, this doesn't mean the code is broken.
     It may just mean you are getting interference.
   - if you look in `nrf-default-values.h` there are different addresses
     you can try (you can use others too).  Worth plugging them in 
     to see if reduces issues.  You can also use a different channel.

Parthiv's board makes this much easier than in the past.  On the other
hand the size of the datasheet makes it not that easy.  As a result,
we have way more starter code than usual.

What you will change:
  - `nrf-driver.c`: all the code you write will be in here.
  - `nrf-default-values.h`: different default values for the NRF.  You 
    can change these.

What you should not have to change:
  
  - `nrf-public.c`: helpers that wrap up the NRF driver interface for
    clients to send and receive.  These are the routines that will call
    your call your driver.
  - `nrf-hw-support.c` and `nrf-hw-support.h`: a bunch of support for
    reading and writing the NRF over SPI.
  - `nrf-test.h`: helpers for testing.  Useful to look at to see how
    to use the NRF interfaces.
  - `tests/*.c`: tests.  Useful to look at to see how
    to use the NRF interfaces.

### Checkoff

Pretty simple:
  1.  You should have implemented your own copies of the `staff_` routines
      and removed `staff-nrf-driver.o` from the makefile.
  2. `make check` should pass.

Extension:
  - You can always do this lab on hard mode and build your own from scratch:
     you'll learn alot.  The tests give reasonable iterfaces.

--------------------------------------------------------------------------------
#### Part 1: Implement `nrf-driver.c:nrf_init`.

This is the longest part, since you need to set all the regsiters,
but it's also probably the most superficial, in that you can just
use `nrf_dump` to get our hardware configuration and then walk down,
replicating it.

It can get setup either for acknowledgements (`ack_p=1`) or no
acknwledgements (`ack_p=0`) but not both.

   -  `ack_p=0`: for this you only have to enable pipe 1.
      No other pipe should be enabled.  

   - `ack_p=1`: for this you will have to enable both pipe 0 and pipe 1.
      This is used by a test `1-one-way-ack.c` which sends a 4 byte
      value back and forth between the client and the server.

You'll want to make sure that the output after running each test program
matches up.


Before you start reading and writing the NRF you need to setup the 
structure:

        nrf_t *n = kmalloc(sizeof *n);
        n->config = c;
        nrf_stat_start(n);
        n->spi = pin_init(c.ce_pin, c.spi_chip);
        n->rxaddr = rxaddr;


Cheat code:
   - If you get stuck you can use `nrf_dump` to print the values we set
     the device too and make sure you set them to the same thing.
     It should be the case that if you change default values that both
     still agree!

When you swap in your `nrf_init`, all the tests should still pass.

--------------------------------------------------------------------------------
#### Part 2: Implement `nrf-driver.c:nrf_tx_send_noack`.

You'll implement sending without acknowledgements.
   1. Set the device to TX mode.
   2. Set the TX address.
   3. Use `nrf_putn` and `NRF_W_TX_PAYLOAD` to write the message to the device
   4. Pulse the `CE` pin.
   5. Wait until the TX fifo is empty.
   6. Clear the TX interrupt.
   7. When you are done, don't forget to set the device back in RX mode.

When you change `nrf_send_noack` to call your `nrf_tx_send_noack` the first test
should still work.

--------------------------------------------------------------------------------
#### Part 3: Implement `nrf-driver.c:nrf_get_pkts`.

For this part, you'll just spin until the RX fifo is empty, pulling
packets off the RX fifo and pushing them onto their associated pipe.
For today, we're only using a single pipe (pipe 1) so you should assert
all packets are for it (you can use `nrf_rx_get_pipeid` for this).  For
each packet you get, the code will push it onto the pipe's circular queue
(just as we did in previous labs).  You should clear the RX interrupt.
When the RX fifo is empty, return the byte count.

When you swap in `nrf_get_pkts` in `nrf-public/nrf_pipe_nbytes` both of
the tests should still work.

--------------------------------------------------------------------------------
#### Part 4: Implement `nrf-driver.c:nrf_tx_send_ack`.

You'll implement sending with acknowledgements.  It will look similar to 
the no-ack version, except:
   1. You need to set the P0 pipe to the same address you are sending to (for acks).
   2. You need to check success using the TX interrupt (and clear it).
   3. You need to check for failure using the max retransmission interrupt (and clear it).
   4. When you are done, don't forget to set the device back in RX mode.

The second test should still work when you swap in.

Congratulations!  You now have a very useful networking system.

--------------------------------------------------------------------------------
#### Extensions

##### Use interrupts

The main one I'd suggest:  change it to use interrupts!  This should
not take that long.

  1. Connect the NRF interrupt pin to a free GPIO pin.
  2. Grab the code from the GPIO interrupt lab.  and steal the initialization and 
     interrupt handler code.
  3. Should hopefully just take 20 minutes or so.
 
##### Implement your own software SPI

I just used the SPI code in the wikipedia page; worked first try.
Make sure you set the pins to input or output.   Also, make sure you
are setting the chip select pin high and low as needed.

##### Remote put32/get32

Do a remote put32/get32:
  1. You can write a small piece of code that waits for PUT32 and GET32 messages and
     performs them locally.
  2. This lets remote pi's control it.
  3. Should be pretty easy to make a small shim library that can run your old programs
     remotely.

##### Other stuff
  
Many many extensions:
  0. Do a network bootloader.
  1. Use more pipes.
  2. See how fast you can go
  3. Change the interface to make it easy to change the different messages sizes.
