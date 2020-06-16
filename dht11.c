#include <stdio.h>
#include <pigpio.h>
#include "dht11.h"

enum pulse_state { 
    PS_IDLE = 0, 
    PS_PREAMBLE_STARTED, 
    PS_DIGITS 
};

struct dht11_data dht11_output = {
    .temp_high = 0, 
    .temp_low = 0, 
    .hum_high = 0, 
    .hum_low = 0
};

/**
 * Callback function when GPIO changes state
 * @param level 0: falling edge; 1: rising edge; 2: no change
 * @param tick microseconds since boot
 */
static void pulse_reader(int gpio, int level, uint32_t tick) 
{
    static enum pulse_state state = PS_IDLE;
    static uint32_t lastTick = 0;
    static uint64_t accum = 0;
    static int count = 0;
    uint32_t len = tick - lastTick;

    lastTick = tick;

    switch (state) {
        case PS_IDLE:
	        /* An 80µS low pulse could be the start of a preamble */
            if (level == 1 && len > 70 && len < 95) 
                state = PS_PREAMBLE_STARTED;
            else 
                state = PS_IDLE;

            break;

        case PS_PREAMBLE_STARTED:
            /* An 80µs high pulse completes the preamble. Anything else and we are back to idle */
            if (level == 0 && len > 70 && len < 95) {
                state = PS_DIGITS;
                accum = 0;
                count = 0;
            } 
            else {
                state = PS_IDLE;
            }

	        break;

        case PS_DIGITS:
	        /* As long as we receive digits, accumulate them into our 64 bit number */
            if (level == 1 && len >= 35 && len <= 65);
            else if (level == 0 && len >= 15 && len <= 35) { accum <<= 1; count++; }
            else if (level == 0 && len >= 60 && len <= 80) { accum = (accum << 1) + 1; count++; }
            else { state = PS_IDLE; }

	        /* When we get our 40th bit, we can process the data */
	        if (count == 40) {
	            state = PS_IDLE;

	            uint8_t parity = (accum & 0xff);
	            uint8_t tempLow = ((accum >> 8) & 0xff);
	            uint8_t tempHigh = ((accum >> 16) & 0xff);
	            uint8_t humLow = ((accum >> 24) & 0xff);
	            uint8_t humHigh = ((accum >> 32) & 0xff);

                uint8_t sum = tempLow + tempHigh + humLow + humHigh;
	            int valid = (parity == sum);
	    
                if (valid) {
                    dht11_output.temp_high = tempHigh;
                    dht11_output.temp_low = tempLow;
                    dht11_output.hum_high = humHigh;
                    dht11_output.hum_low = humLow;
                }
	        }

	        break;
    }
}

/**
 * DHT11 initialization
 */ 
void dht11_start(void)
{
    /* Setup callback */
    gpioSetWatchdog(DHT11_PIN, 50);
    gpioSetAlertFunc(DHT11_PIN, pulse_reader);

    gpioSetMode(DHT11_PIN, PI_INPUT);
    gpioSetPullUpDown(DHT11_PIN, PI_PUD_UP);
    gpioSetMode(DHT11_PIN, PI_OUTPUT);
    gpioWrite(DHT11_PIN, 0);
}

/**
 * DHT11 read data
 */ 
void dht11_read(void) {
    gpioSetMode(DHT11_PIN, PI_OUTPUT);
    gpioDelay(19 * 1000);
    gpioSetMode(DHT11_PIN, PI_INPUT);
    gpioSetPullUpDown(DHT11_PIN, PI_PUD_UP);
}
