/*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.0
 */

/*
� [2026] Microchip Technology Inc. and its subsidiaries.

	Subject to your compliance with these terms, you may use Microchip 
	software and any derivatives exclusively with Microchip products. 
	You are responsible for complying with 3rd party license terms  
	applicable to your use of 3rd party software (including open source  
	software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
	NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
	SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
	MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
	WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
	INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
	KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
	MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
	FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
	TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
	EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
	THIS SOFTWARE.
 */
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/timer/tmr4.h"
#include "mcp47cxdx.h"

/*
	Main application
 */

uint8_t clientAddr = 0b1100011;
uint8_t writeLenght;
uint8_t readLenght;

#define LED_RED 0b10
#define LED_GREEN 0b01
#define LED_YELLOW 0b11
#define LED_OFF	0b00

const uint8_t segLut[10] = {
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

//---------ENCODER---------
const int8_t encoderLut[16] = {
	0, -1, 1, 0,
	1, 0, 0, -1,
	-1, 0, 0, 1,
	0, 1, -1, 0
};

#define VOLTAGE_STEP_MV  100
#define VOLTAGE_MIN_MV   0
#define VOLTAGE_MAX_MV   24000

#define CURRENT_STEP_MA  10
#define CURRENT_MIN_MA   0
#define CURRENT_MAX_MA   2000

#define UNIT_Amps 0
#define UNIT_Volts 1

static volatile int16_t encoder_delta = 0;
static uint8_t prev_state = 0;

uint8_t encoder_read_pins(void);
int16_t encoder_get_delta(void);
void encoder_task(void);



void I2C_Write(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght);
bool I2C_Read(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght);
void writeDisplay(float number);
void setLeds(uint8_t ledTop, uint8_t ledBottom);
void writeUnit(uint8_t unit);
void setOutputVoltage(uint16_t milivolt);
void setCurrentLimit(uint16_t miliamp);

int main(void) {
	SYSTEM_Initialize();

	IO_RA4_SetHigh();

	TMR4_OverflowCallbackRegister(encoder_task);
	INTERRUPT_GlobalInterruptEnable();

	uint8_t data2[] = {0x0D, 0b00001000};
	I2C1_Host.Write(clientAddr, data2, 2);
	while (I2C1_Host.IsBusy()) {
		I2C1_Host.Tasks();
		NOP();
	}

	uint16_t voltage_set_point = 0;
	uint16_t current_set_point = 0;

	setOutputVoltage(voltage_set_point);
	setCurrentLimit(current_set_point);

	while (1) {
		int16_t delta = encoder_get_delta();
		if (delta != 0) {
			int32_t new_v = (int32_t)voltage_set_point + (int32_t)delta * VOLTAGE_STEP_MV;
			if (new_v < VOLTAGE_MIN_MV) new_v = VOLTAGE_MIN_MV;
			if (new_v > VOLTAGE_MAX_MV) new_v = VOLTAGE_MAX_MV;
			voltage_set_point = (uint16_t)new_v;
			setOutputVoltage(voltage_set_point);
		}

		writeDisplay(voltage_set_point / 1000.0f);
		__delay_us(50);
		writeUnit(UNIT_Volts);
		__delay_ms(50);
	}
}

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
	uint8_t update[] = {0x0C, 0x00};
	I2C_Write(clientAddr, update, 2);
}

void setLeds(uint8_t ledTop, uint8_t ledBottom) {
	uint8_t transmitionData[] = {0x08, 0x00};

	transmitionData[1] = ledTop | (ledBottom << 5);
	//Notes:Bit0-> Top Green
	//		Bit1-> Top Red
	//		Bit5-> Bottom Green

	I2C_Write(clientAddr, transmitionData, 2);
	uint8_t update[] = {0x0C, 0x00};
	I2C_Write(clientAddr, update, 2);
}

void writeUnit(uint8_t unit) {
	uint8_t transmitionData[] = {0x04, 0x00};

	if (unit == UNIT_Volts) {
		transmitionData[1] = 0b11110010;
	}
	if (unit == UNIT_Amps) {
		transmitionData[1] = 0b10110111;
	}
	I2C_Write(clientAddr, transmitionData, 8);

	//Update display
	uint8_t update[] = {0x0C, 0x00};
	I2C_Write(clientAddr, update, 2);
}

void setOutputVoltage(uint16_t milivolt) {
	uint16_t DAC_value = ((uint32_t) milivolt * 10600) >> 16;
	if (DAC_value > 4095) {
		DAC_value = 4095;
	}
	MCP47_SetOutput(MCP47_CHANNEL_1, DAC_value);
}

void setCurrentLimit(uint16_t miliamp) {
	uint16_t DAC_value = ((uint32_t) miliamp * 16500) >> 16;
	if (DAC_value > 4095) {
		DAC_value = 4095;
	}
	MCP47_SetOutput(MCP47_CHANNEL_0, DAC_value);
}

uint8_t encoder_read_pins(void) {
	uint8_t a = IO_RC7_GetValue();
	uint8_t b = IO_RC6_GetValue();
	return (a << 1) | b;
}

void encoder_task(void) {
	uint8_t curr_state = encoder_read_pins();
	uint8_t index = (prev_state << 2) | curr_state;
	encoder_delta += encoderLut[index];
	prev_state = curr_state;
}

int16_t encoder_get_delta(void) {
	int16_t delta = encoder_delta;
	encoder_delta = 0;
	return delta;
}