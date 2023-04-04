#include <stdio.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "hardware/dma.h"
#include "hardware/sync.h"
#include "pio_serializer.pio.h"

#define MAXPINENABLE 18

#define DISABLEMAXOUT 1
#define ENABLEMAXOUT 0
#define GLITCHOUTPIN 19

#define SIGNALPINFROMLPC 11

#define CMD_DELAY 'D'
#define CMD_PULSE 'P'
#define CMD_GLITCH 'G'


/**
 * Command structure
 * 32bit per each row
 * - pins initial state
 * - clockcycles delay after gpio20 high signal
 * - clockcycles width of glitch
 * - pins set for glitch.
 * - pins normal state
 */
static uint32_t cmds[1+1+1+1];

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

int dma_chan;
// send a bit every PIO_SERIAL_CLKDIV clock cycles
#define PIO_SERIAL_CLKDIV 1.f

// size of glitch buffer, every entry accounts for 32 samples
#define GLITCH_BUFFER_SIZE (6117)
static uint32_t wavetable[GLITCH_BUFFER_SIZE + 1] = {0};

void setup_dma() {
    uint offset = pio_add_program(pio0, &pio_serialiser_program);
    pio_serialiser_program_init(pio0, 0, offset, GLITCHOUTPIN, PIO_SERIAL_CLKDIV);
    dma_chan = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
    channel_config_set_read_increment(&c, true);
    channel_config_set_dreq(&c, DREQ_PIO0_TX0);

    dma_channel_configure(
            dma_chan,
            &c,
            &pio0_hw->txf[0],   // Write address (only need to set this once)
            NULL,               // Don't provide a read address yet
            GLITCH_BUFFER_SIZE+1, // Write the same value many times, then halt and interrupt
            false               // Don't start yet
    );

}
void prepare_dma() {
    dma_channel_set_read_addr(dma_chan, wavetable, false);
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
                prepare_dma();
                power_cycle_target();

                // Timing matters here
                dma_channel_start(dma_chan);

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