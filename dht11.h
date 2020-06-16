#ifndef DHT11_H
#define DHT11_H

#define DHT11_PIN   17

struct dht11_data {
    uint8_t temp_high;
    uint8_t temp_low;
    uint8_t hum_high;
    uint8_t hum_low;
};

extern struct dht11_data dht11_output;

void    dht11_start     (void);
void    dht11_read      (void);

#endif
