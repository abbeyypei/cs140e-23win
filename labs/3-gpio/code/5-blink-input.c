// part2: test GPIO input.
//   - if we get a signal on the input pin 21, turn the LED on.
#include "rpi.h"

void notmain(void) {
    const int blink = 16;
    const int led = 20;
    const int input = 21;

    int counter = 0;

    gpio_set_output(blink);
    gpio_set_output(led);
    gpio_set_input(input);
    while(1) { 
        // could also do: 
        //  gpio_write(input, gpio_read(led));
        if(gpio_read(input))
            gpio_set_on(led);
        else
            gpio_set_off(led);

        if (counter == 0) {
            gpio_set_on(blink);
        }
        if (counter == 1000000) {
            gpio_set_off(blink);
        }
        counter = (counter + 1) % 2000000;
    }
}
