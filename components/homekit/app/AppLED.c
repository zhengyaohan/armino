//Beken Corporation,porting from nRF52. Just for compiling

#include "HAP.h"

#include "AppLED.h"

/**
 * Convert LED identifier to GPIO PIN number
 *
 * @param id   LED identifier
 *
 * @return LED GPIO pin number
 */
 #if 0
static uint32_t LEDIdToPin(AppLEDIdentifier id) {
#if 0
    switch (id) {
        case kAppLEDIdentifier_1:
            return LED_1;
        case kAppLEDIdentifier_2:
            return LED_2;
    }
    HAPFatalError();
    return 0;
#endif
	return 0;
}
#endif


void AppLEDInit(void) {
#if 0
    nrf_gpio_pin_set(LED_1);
    nrf_gpio_cfg_output(LED_1);
    nrf_gpio_pin_set(LED_2);
    nrf_gpio_cfg_output(LED_2);
    nrf_gpio_pin_set(LED_3);
    nrf_gpio_cfg_output(LED_3);
    nrf_gpio_pin_set(LED_4);
    nrf_gpio_cfg_output(LED_4);
#endif
}

void AppLEDDeinit(void) {
    // Do nothing
}

void AppLEDSet(AppLEDIdentifier id, bool on) {
#if 0
    if (on) {
        nrf_gpio_pin_clear(LEDIdToPin(id));
    } else {
        nrf_gpio_pin_set(LEDIdToPin(id));
    }
#endif
}
