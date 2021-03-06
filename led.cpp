// Header files
extern "C" {
	#include <asf.h>
}
#include "common.h"
#include "fan.h"
#include "led.h"


// Definitions
#define LED_ENABLE_PIN IOPORT_CREATE_PIN(PORTE, 3)
#define LED_CHANNEL TC_CCD
#define LED_TIMER FAN_TIMER
#define LED_TIMER_PERIOD FAN_TIMER_PERIOD


// Supporting function implementation
void Led::initialize() {

	// Configure LED
	ioport_set_pin_dir(LED_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Turn on LED
	setBrightness(LED_MAX_BRIGHTNESS);
}

void Led::setBrightness(uint8_t brightness) {

	// Set brightness
	tc_write_cc(&LED_TIMER, LED_CHANNEL, getValueInRange(brightness, LED_MIN_BRIGHTNESS, LED_MAX_BRIGHTNESS) * LED_TIMER_PERIOD / LED_MAX_BRIGHTNESS);
}
