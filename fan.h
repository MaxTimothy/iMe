// Header gaurd
#ifndef FAN_H
#define FAN_H


// Definitions
#define FAN_TIMER TCE0
#define FAN_TIMER_PERIOD 0x208D
#define FAN_MIN_SPEED 0
#define FAN_MAX_SPEED 255


// Fan class
class Fan {

	// Public
	public:
	
		// Initialize
		static void initialize();
		
		// Set speed
		static void setSpeed(uint8_t speed);
};


#endif
