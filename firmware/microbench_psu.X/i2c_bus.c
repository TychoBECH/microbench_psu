#include "i2c_bus.h"
#include "mcc_generated_files/i2c_host/i2c1.h"

void I2C_Write(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght) {
	I2C1_Host.Write(clientAddress, data, dataLenght);
	while (I2C1_Host.IsBusy()) {
		I2C1_Host.Tasks();
	}
}

bool I2C_Read(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght) {
	I2C1_Host.Read(clientAddress, data, dataLenght);
	while (I2C1_Host.IsBusy()) {
		I2C1_Host.Tasks();
	}
	return true;
}
