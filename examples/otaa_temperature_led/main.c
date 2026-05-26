#include "pico/stdlib.h"
#include "pico/lorawan.h"
#include "config.h"

const struct lorawan_sx1262_settings sx1262_settings = {
    .spi = { .inst = spi1, .mosi = 11, .miso = 12, .sck = 10, .nss = 3, },
    .reset = 15, .busy = 2, .dio1 = 20,
};
const struct lorawan_otaa_settings otaa_settings = {
    .device_eui = LORAWAN_DEVICE_EUI,
    .app_eui = LORAWAN_APP_EUI,
    .app_key = LORAWAN_APP_KEY,
    .channel_mask = LORAWAN_CHANNEL_MASK,
};

int main(void) {
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    if (lorawan_init_otaa(&sx1262_settings, LORAWAN_REGION, &otaa_settings) < 0) {
        // fast blink = init failed (radio not responding)
        while(1) { gpio_put(25,1); sleep_ms(100); gpio_put(25,0); sleep_ms(100); }
    }

    lorawan_join();

    // wait for join - LED on while joining
    gpio_put(25, 1);
    while (!lorawan_is_joined()) {
        lorawan_process_timeout_ms(1000);
    }
    gpio_put(25, 0);

    // joined - send a uplink every 30 seconds
    while (1) {
        uint8_t payload = 0x01;
        lorawan_send_unconfirmed(&payload, sizeof(payload), 1);

        // blink once to indicate uplink sent
        gpio_put(25,1); sleep_ms(200); gpio_put(25,0);

        lorawan_process_timeout_ms(30000);
    }

    return 0;
}
