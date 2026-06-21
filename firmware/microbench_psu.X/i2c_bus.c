#include "i2c_bus.h"
#include "mcc_generated_files/i2c_host/i2c1.h"

// Approximate bound on poll iterations while waiting for a transfer to finish.
// There's no hardware timer wired into this loop, so this is a coarse
// "stop waiting eventually" guard rather than a precise time limit -- it exists
// so a stuck bus or unresponsive device hangs the firmware for a bounded
// number of loop iterations instead of forever.
#define I2C_TIMEOUT_ITERATIONS 10000

bool I2C_Write(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght) {
	I2C1_Host.Write(clientAddress, data, dataLenght);
	uint16_t timeout = I2C_TIMEOUT_ITERATIONS;
	while (I2C1_Host.IsBusy()) {
		I2C1_Host.Tasks();
		if (--timeout == 0) {
			return false;
		}
	}
	return true;
}

bool I2C_Read(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght) {
	I2C1_Host.Read(clientAddress, data, dataLenght);
	uint16_t timeout = I2C_TIMEOUT_ITERATIONS;
	while (I2C1_Host.IsBusy()) {
		I2C1_Host.Tasks();
		if (--timeout == 0) {
			return false;
		}
	}
	return true;
}
