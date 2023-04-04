#include <stdio.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "pio_serializer.pio.h"

#define MAXPINENABLE 18

#define DISABLEMAXOUT 1
#define ENABLEMAXOUT 0
#define GLITCHOUTPIN 19

#define SIGNALPINFROMLPC 11


#define CMD_DELAY 'D'
#define CMD_PULSE 'P'
#define CMD_GLITCH 'G'

void initialize_board() {
    gpio_init(MAXPINENABLE);
    gpio_put(MAXPINENABLE, DISABLEMAXOUT);
    gpio_set_dir(MAXPINENABLE, GPIO_OUT);

    gpio_init(GLITCHOUTPIN);
    gpio_put(GLITCHOUTPIN, 0);
    gpio_set_dir(GLITCHOUTPIN, GPIO_OUT);
}

void power_cycle_target() {
    gpio_put(MAXPINENABLE, DISABLEMAXOUT);
    sleep_ms(50);
    gpio_put(MAXPINENABLE, ENABLEMAXOUT);
}


int main() {
    stdio_init_all();
    stdio_set_translate_crlf(&stdio_usb, false);
    initialize_board();


    const uint led_pin = 25;
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    uint32_t delay = 10000;
    uint32_t pulse = 10000;
    gpio_put(led_pin, 1);
    sleep_ms(1000);
    gpio_put(led_pin, 0);
    while (1) {
        uint8_t cmd = getchar();
        switch(cmd) {
            case CMD_DELAY:
                fread(&delay, 1, 4, stdin);
                // dv(delay, pulse);
                break;
            case CMD_PULSE:
                fread(&pulse, 1, 4, stdin);
                // dv(delay, pulse);
                break;
            case CMD_GLITCH:
                power_cycle_target();

                while(!gpio_get(SIGNALPINFROMLPC));
                for(uint32_t i=0; i < delay; i++) {
                    asm("NOP");
                }
                gpio_put(GLITCHOUTPIN, 1);
                for(uint32_t i=0; i < pulse; i++) {
                    asm("NOP");
                }
                gpio_put(GLITCHOUTPIN, 0);
        }
    }

}