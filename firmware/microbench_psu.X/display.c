#include "display.h"
#include "i2c_bus.h"

static const uint8_t clientAddr = 0b1100011;

static const uint8_t segLut[10] = {
	//bit layout: EDCFdpGBA
	0b11110011, // 0
	0b00100010, // 1
	0b11000111, // 2
	0b01100111, // 3
	0b00110110, // 4
	0b01110101, // 5
	0b11110101, // 6
	0b00100011, // 7
	0b11110111, // 8
	0b01110111 // 9
};

void Display_Init(void) {
	uint8_t data2[] = {0x0D, 0b00001000};
	I2C_Write(clientAddr, data2, 2);
}

void writeDisplay(float number) {
	uint8_t transmitionData[] = {0x01, 0x00, 0x02, 0x00, 0x07, 0x00, 0x04, 0x00};
	uint16_t scaled = 0;
	uint8_t decimalPos = 0;

	if (number < 10.0) {
		scaled = (uint16_t) (number * 1000);
		decimalPos = 1;
	} else if (number < 100.0) {
		scaled = (uint16_t) (number * 100);
		decimalPos = 3;
	} else if (number < 1000.0) {
		scaled = (uint16_t) (number * 10);
		decimalPos = 5;
	} else {
		scaled = (uint16_t) number;
	}
	if (number > 9999) {
		scaled = 9999;
	}

	transmitionData[1] = segLut[(scaled / 1000) % 10];
	transmitionData[3] = segLut[(scaled / 100) % 10];
	transmitionData[5] = segLut[(scaled / 10) % 10];
	transmitionData[7] = segLut[(scaled) % 10];

	if (decimalPos != 0) {
		transmitionData[decimalPos] += 0b1000; //Add DP
	}

	I2C_Write(clientAddr, transmitionData, 8);
}

void writeUnit(uint8_t unit) {
	uint8_t transmitionData[] = {0x04, 0x00};

	if (unit == UNIT_Volts) {
		transmitionData[1] = 0b11110010;
	}
	if (unit == UNIT_Amps) {
		transmitionData[1] = 0b10110111;
	}
	I2C_Write(clientAddr, transmitionData, 2);
}

void displayCommit(void) {
	uint8_t update[] = {0x0C, 0x00};
	I2C_Write(clientAddr, update, 2);
}

void displayBlank(void) {
	uint8_t clear[] = {0x01, 0x00, 0x02, 0x00, 0x07, 0x00, 0x04, 0x00};
	I2C_Write(clientAddr, clear, 8);
	uint8_t clearUnit[] = {0x04, 0x00};
	I2C_Write(clientAddr, clearUnit, 2);
	displayCommit();
}

void setLeds(uint8_t ledTop, uint8_t ledBottom) {
	uint8_t transmitionData[] = {0x08, 0x00};

	transmitionData[1] = ledTop | (ledBottom << 5);
	//Notes:Bit0-> Top Green
	//		Bit1-> Top Red
	//		Bit5-> Bottom Green

	I2C_Write(clientAddr, transmitionData, 2);
	displayCommit();
}
