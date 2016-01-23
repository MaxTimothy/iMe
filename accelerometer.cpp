// Header files
extern "C" {
	#include <asf.h>
}
#include "accelerometer.h"


// Definitions

// Pins
#define TWI_MASTER TWIC
#define ACCELEROMETER_VDDIO IOPORT_CREATE_PIN(PORTB, 1)
#define ACCELEROMETER_SDA IOPORT_CREATE_PIN(PORTC, 0)
#define ACCELEROMETER_SCL IOPORT_CREATE_PIN(PORTC, 1)

// Bus details
#define MASTER_ADDRESS 0x00
#define ACCELEROMETER_ADDRESS 0x1D
#define ACCELEROMETER_SPEED 400000
#define DEVICE_ID 0x4A
#define SENSITIVITY_2G (2048 / 2)
#define SENSITIVITY_4G (2048 / 4)
#define SENSITIVITY_8G (2048 / 8)

// Registers
#define STATUS 0x00
#define STATUS_XDR 0b00000001
#define STATUS_YDR 0b00000010
#define STATUS_ZDR 0b00000100
#define OUT_X_MSB 0x01
#define OUT_X_LSB 0x02
#define OUT_Y_MSB 0x03
#define OUT_Y_LSB 0x04
#define OUT_Z_MSB 0x05
#define OUT_Z_LSB 0x06
#define WHO_AM_I 0x0D
#define XYZ_DATA_CFG 0x0E
#define XYZ_DATA_CFG_FS0 0b00000001
#define XYZ_DATA_CFG_FS1 0b00000010
#define PL_CFG 0x11
#define CTRL_REG1 0x2A
#define CTRL_REG1_ACTIVE 0b00000001
#define CTRL_REG1_DR0 0b00001000
#define CTRL_REG1_DR1 0b00010000
#define CTRL_REG1_DR2 0b00100000
#define CTRL_REG2 0x2B
#define CTRL_REG2_MODS0 0b00000001
#define CTRL_REG2_MODS1 0b00000010
#define CTRL_REG2_RST 0b01000000
#define OFF_X 0x2F
#define OFF_Y 0x30
#define OFF_Z 0x31


// Supporting function implementation
Accelerometer::Accelerometer() {

	// Initialize variables
	uint8_t value;
	
	// Configure VDDIO, SDA and SCL pins
	ioport_set_pin_dir(ACCELEROMETER_VDDIO, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(ACCELEROMETER_VDDIO, IOPORT_PIN_LEVEL_HIGH);
	ioport_set_pin_mode(ACCELEROMETER_SDA, IOPORT_MODE_WIREDANDPULL);
	ioport_set_pin_mode(ACCELEROMETER_SCL, IOPORT_MODE_WIREDANDPULL);
	
	// Configure interface
	twi_options_t options;
	options.speed = ACCELEROMETER_SPEED;
	options.chip = MASTER_ADDRESS;
	options.speed_reg = TWI_BAUD(sysclk_get_cpu_hz(), ACCELEROMETER_SPEED);
	
	// Initialize interface
	sysclk_enable_peripheral_clock(&TWI_MASTER);
	twi_master_init(&TWI_MASTER, &options);
	twi_master_enable(&TWI_MASTER);

	// Create packet
	twi_package_t packet;
	packet.addr[0] = WHO_AM_I;
	packet.addr_length = 1;
	packet.chip = ACCELEROMETER_ADDRESS;
	packet.buffer = &value;
	packet.length = 1;
	packet.no_wait = false;
	
	// Check if transmitting or receiving failed
	if(twi_master_read(&TWI_MASTER, &packet) != TWI_SUCCESS || value != DEVICE_ID)
	
		// Clear is working
		isWorking = false;
	
	// Otherwise
	else {
	
		// Reset the accelerometer
		writeValue(CTRL_REG2, CTRL_REG2_RST);
		
		// Wait enough time for accelerometer to initialize
		delay_ms(1);
		
		// Initialize settings
		initializeSettings();
		
		// Calibrate
		//calibrate();
	
		// Set is working
		isWorking = true;
	}
}

void Accelerometer::readAccelerationValues() {

	// Wait until data is available
	while(!dataAvailable());
	
	// Read values
	uint8_t values[6];
	readValue(OUT_X_MSB, values, 6);
	
	// Set values
	xValue = ((values[0] << 8) | values[1]) >> 4;
	yValue = ((values[2] << 8) | values[3]) >> 4;
	zValue = ((values[4] << 8) | values[5]) >> 4;
	
	// Calculate acceleration values and account for chips orientation
	zAcceleration = static_cast<int32_t>(xValue) * 1000 / SENSITIVITY_2G;
	yAcceleration = static_cast<int32_t>(yValue) * 1000 / SENSITIVITY_2G;
	xAcceleration = static_cast<int32_t>(zValue) * 1000 / SENSITIVITY_2G;
}

void Accelerometer::initializeSettings() {

	// Put accelerometer into standby mode
	writeValue(CTRL_REG1, 0);

	// Set dynamic range to 2g
	writeValue(XYZ_DATA_CFG, 0);
	
	// Set oversampling mode to high resolution
	writeValue(CTRL_REG2, CTRL_REG2_MODS1);
	
	// Set output data rate frequency to 6.25Hz and enable active mode
	writeValue(CTRL_REG1, CTRL_REG1_DR2 | CTRL_REG1_DR1 | CTRL_REG1_DR0 | CTRL_REG1_ACTIVE);
}

void Accelerometer::calibrate() {

	// Initialize variables
	int32_t averageXValue = 0;
	int32_t averageYValue = 0;
	int32_t averageZValue = 0;

	// Put accelerometer into standby mode
	writeValue(CTRL_REG1, 0);

	// Set dynamic range to 2g
	writeValue(XYZ_DATA_CFG, 0);
	
	// Set oversampling mode to high resolution
	writeValue(CTRL_REG2, CTRL_REG2_MODS1);
	
	// Clear offsets
	writeValue(OFF_X, 0);
	writeValue(OFF_Y, 0);
	writeValue(OFF_Z, 0);
	
	// Set output data rate frequency to 1.56Hz and enable active mode
	writeValue(CTRL_REG1, CTRL_REG1_DR2 | CTRL_REG1_DR1 | CTRL_REG1_DR0 | CTRL_REG1_ACTIVE);
	
	// Get average values
	for(uint8_t i = 0; i < 10; i++) {

		// Read acceleration values
		readAccelerationValues();
		
		// Add values to average
		averageXValue += xValue;
		averageYValue += yValue;
		averageZValue += zValue;
	}
	
	// Compute averages
	averageXValue /= 10;
	averageYValue /= 10;
	averageZValue /= 10;
	
	// Put accelerometer into standby mode
	writeValue(CTRL_REG1, 0);
	
	// Set offsets
	int8_t xOffset = -averageXValue / 2;
	int8_t yOffset = -averageYValue / 2;
	int8_t zOffset = -(averageZValue - SENSITIVITY_2G) / 2;
	writeValue(OFF_X, xOffset);
	writeValue(OFF_Y, yOffset);
	writeValue(OFF_Z, zOffset);
	
	// Initialize settings
	initializeSettings();
}

void Accelerometer::sendCommand(uint8_t command) {

	// Transmit request
	transmit(command);
}

void Accelerometer::writeValue(uint8_t address, uint8_t value) {

	// Transmit request
	transmit(address, value, true);
}

bool Accelerometer::dataAvailable() {

	// Return if data is available
	uint8_t buffer;
	readValue(STATUS, &buffer);
	return buffer & (STATUS_XDR | STATUS_YDR | STATUS_ZDR);
}

void Accelerometer::readValue(uint8_t address, uint8_t *responseBuffer, uint8_t responseLength) {

	// Get response
	transmit(address, 0, false, responseBuffer, responseLength);
}

void Accelerometer::transmit(uint8_t command, uint8_t value, bool sendValue, uint8_t *responseBuffer, uint8_t responseLength) {
	
	// Create packet
	twi_package_t packet;
	packet.addr[0] = command;
	if(sendValue) {
		packet.addr[1] = value;
		packet.addr_length = 2;
	}
	else
		packet.addr_length = 1;
	packet.chip = ACCELEROMETER_ADDRESS;
	packet.buffer = responseBuffer;
	packet.length = responseLength;
	packet.no_wait = false;
	
	// Wait until transmission is done
	while(twi_master_transfer(&TWI_MASTER, &packet, responseLength) != TWI_SUCCESS);
}