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
#include "mcc_generated_files/nvm/nvm.h"
#include "mcp47cxdx.h"
#include <stdbool.h>

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

#define VOLTAGE_STEP_MV  50
#define VOLTAGE_MIN_MV   0
#define VOLTAGE_MAX_MV   24000

#define CURRENT_STEP_MA  5
#define CURRENT_MIN_MA   0
#define CURRENT_MAX_MA   2000

// ADC I2C addresses
#define ADC_ADDR_VOLTAGE  0b1001001
#define ADC_ADDR_CURRENT  0b1001000
#define ADC_ADDR_TEMP     0b1001010

#define VOLTAGE_ADC_MV_PER_LSB  12.15f  // placeholder: 24000 mV / 4095
#define CURRENT_ADC_MA_PER_LSB  1.1745f // 5000 mV / 4095 LSB / 1.03958 mV per mA
#define TEMP_ADC_C_PER_LSB      0.0407f // 5000 mV / 4095 LSB / 30 mV per °C

#define TEMP_SHUTDOWN_C  85.0f
#define TEMP_RECOVER_C   70.0f

#define SETPOINT_DISPLAY_TICKS  20      // 20 @ 50 ms = 1.0 s
#define LONG_PRESS_TICKS        20      // 20 @ 50 ms = 1.0 s
#define EEPROM_WRITE_TICKS      100     // 100 @ 50 ms = 5.0 s

// EEPROM layout
#define EEPROM_MAGIC_ADDR    0x00
#define EEPROM_MAGIC_VALUE   0xAB
#define EEPROM_VOLTAGE_HI    0x01
#define EEPROM_VOLTAGE_LO    0x02
#define EEPROM_CURRENT_HI    0x03
#define EEPROM_CURRENT_LO    0x04

#define UNIT_Amps 0
#define UNIT_Volts 1

typedef enum {
    EDIT_VOLTAGE,
    EDIT_CURRENT
} edit_mode_t;


static volatile int16_t encoder_delta = 0;
static uint8_t prev_state = 0;

uint8_t encoder_read_pins(void);
int16_t encoder_get_delta(void);
void encoder_task(void);



void settings_save(uint16_t voltage, uint16_t current);
void settings_load(uint16_t *voltage, uint16_t *current);

void I2C_Write(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght);
bool I2C_Read(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght);
uint16_t ADC_ReadRaw(uint8_t address);
void writeDisplay(float number);
void writeUnit(uint8_t unit);
void displayCommit(void);
void displayBlank(void);
void setLeds(uint8_t ledTop, uint8_t ledBottom);
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

	uint16_t voltage_set_point = 3300;
	uint16_t current_set_point = 100;
	settings_load(&voltage_set_point, &current_set_point);

	edit_mode_t edit_mode = EDIT_VOLTAGE;
	bool output_enabled = false;
	bool standby = false;
	bool over_temp = false;
	uint8_t setpoint_display_timer = 0;
	uint8_t long_press_counter = 0;
	uint8_t eeprom_dirty_timer = 0;
	uint8_t led_flash_counter = 0;

	uint8_t prev_btn_enc    = 1;
	uint8_t prev_btn_enable = 1;

	IO_RA4_SetLow();
	setOutputVoltage(0);
	setCurrentLimit(current_set_point);

	while (1) {
		uint8_t btn_enc    = IO_RC4_GetValue();
		uint8_t btn_enable = IO_RC3_GetValue();

		// --- standby mode ---
		if (standby) {
			// wake on falling edge only and ignores the held button that triggered sleep
			if ((!btn_enc && prev_btn_enc) || (!btn_enable && prev_btn_enable)) {
				standby = false;
				long_press_counter = 0;
			}
			prev_btn_enc    = btn_enc;
			prev_btn_enable = btn_enable;
			__delay_ms(50);
			continue;
		}

		// --- long press detection on encoder button (RC4) ---
		if (!btn_enc) {
			long_press_counter++;
			if (long_press_counter >= LONG_PRESS_TICKS) {
				// enter standby
				standby = true;
				long_press_counter = 0;
				if (output_enabled) {
					output_enabled = false;
					IO_RA4_SetLow();
					setOutputVoltage(0);
				}
				displayBlank();
				setLeds(LED_OFF, LED_OFF);
				prev_btn_enc    = btn_enc;
				prev_btn_enable = btn_enable;
				__delay_ms(50);
				continue;
			}
		} else {
			// --- short press: released before long press threshold ---
			if (prev_btn_enc == 0 && long_press_counter < LONG_PRESS_TICKS) {
				if (!over_temp) {
					output_enabled = !output_enabled;
					if (output_enabled) {
						IO_RA4_SetHigh();
						setOutputVoltage(voltage_set_point);
					} else {
						IO_RA4_SetLow();
						setOutputVoltage(0);
					}
				}
			}
			long_press_counter = 0;
		}

		// --- enable button (RC3): cycle V/A ---
		if (!btn_enable && prev_btn_enable) {
			edit_mode = (edit_mode == EDIT_VOLTAGE) ? EDIT_CURRENT : EDIT_VOLTAGE;
		}

		prev_btn_enc    = btn_enc;
		prev_btn_enable = btn_enable;

		// --- encoder ---
		int16_t delta = encoder_get_delta();
		if (delta != 0) {
			if (edit_mode == EDIT_VOLTAGE) {
				int32_t new_v = (int32_t)voltage_set_point + (int32_t)delta * VOLTAGE_STEP_MV;
				if (new_v < VOLTAGE_MIN_MV) new_v = VOLTAGE_MIN_MV;
				if (new_v > VOLTAGE_MAX_MV) new_v = VOLTAGE_MAX_MV;
				voltage_set_point = (uint16_t)new_v;
				if (output_enabled) setOutputVoltage(voltage_set_point);
			} else {
				int32_t new_i = (int32_t)current_set_point + (int32_t)delta * CURRENT_STEP_MA;
				if (new_i < CURRENT_MIN_MA) new_i = CURRENT_MIN_MA;
				if (new_i > CURRENT_MAX_MA) new_i = CURRENT_MAX_MA;
				current_set_point = (uint16_t)new_i;
				setCurrentLimit(current_set_point);
			}
			if (output_enabled) setpoint_display_timer = SETPOINT_DISPLAY_TICKS;
			eeprom_dirty_timer = EEPROM_WRITE_TICKS;
		}
		if (setpoint_display_timer > 0) setpoint_display_timer--;

		// --- over-temperature protection ---
		float temp_c = ADC_ReadRaw(ADC_ADDR_TEMP) * TEMP_ADC_C_PER_LSB;
		if (!over_temp && temp_c >= TEMP_SHUTDOWN_C) {
			over_temp = true;
			output_enabled = false;
			IO_RA4_SetLow();
			setOutputVoltage(0);
		} else if (over_temp && temp_c < TEMP_RECOVER_C) {
			over_temp = false;
		}

		// --- EEPROM deferred write ---
		if (eeprom_dirty_timer > 0) {
			eeprom_dirty_timer--;
			if (eeprom_dirty_timer == 0) {
				settings_save(voltage_set_point, current_set_point);
			}
		}

		// --- display ---
		if (over_temp) {
			writeDisplay(temp_c);
			writeUnit(UNIT_Volts); // no temperature unit symbol available, blank is clearest
			displayCommit();
		} else {
			bool show_setpoint = !output_enabled || (setpoint_display_timer > 0);

			if (edit_mode == EDIT_VOLTAGE) {
				float display_val = show_setpoint
					? voltage_set_point / 1000.0f
					: ADC_ReadRaw(ADC_ADDR_VOLTAGE) * VOLTAGE_ADC_MV_PER_LSB / 1000.0f;
				writeDisplay(display_val);
				writeUnit(UNIT_Volts);
			} else {
				float display_val = show_setpoint
					? current_set_point / 1000.0f
					: ADC_ReadRaw(ADC_ADDR_CURRENT) * CURRENT_ADC_MA_PER_LSB / 1000.0f;
				writeDisplay(display_val);
				writeUnit(UNIT_Amps);
			}
			displayCommit();
		}

		// --- LEDs: both flash green when output on, both off when output off ---
		led_flash_counter++;
		if (output_enabled) {
			uint8_t led = (led_flash_counter & 0x08) ? LED_GREEN : LED_OFF;
			setLeds(led, led);
		} else {
			setLeds(LED_OFF, LED_OFF);
		}

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

#define ADC_AVG_SAMPLES 20

uint16_t ADC_ReadRaw(uint8_t address) {
	uint8_t data[2];
	uint32_t sum = 0;
	for (uint8_t i = 0; i < ADC_AVG_SAMPLES; i++) {
		I2C_Read(address, data, 2);
		sum += ((uint16_t)(data[0] & 0x0F) << 8) | data[1];
	}
	return (uint16_t)(sum / ADC_AVG_SAMPLES);
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

void settings_save(uint16_t voltage, uint16_t current) {
	NVM_UnlockKeySet(UNLOCK_KEY);
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_VOLTAGE_HI, voltage >> 8);
	while (NVM_IsBusy());
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_VOLTAGE_LO, voltage & 0xFF);
	while (NVM_IsBusy());
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_CURRENT_HI, current >> 8);
	while (NVM_IsBusy());
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_CURRENT_LO, current & 0xFF);
	while (NVM_IsBusy());
	EEPROM_Write(EEPROM_START_ADDRESS + EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
	while (NVM_IsBusy());
	NVM_UnlockKeyClear();
}

void settings_load(uint16_t *voltage, uint16_t *current) {
	if (EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_MAGIC_ADDR) != EEPROM_MAGIC_VALUE) {
		return; // no saved data — keep defaults
	}
	*voltage = ((uint16_t)EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_VOLTAGE_HI) << 8)
	         |  (uint16_t)EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_VOLTAGE_LO);
	*current = ((uint16_t)EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_CURRENT_HI) << 8)
	         |  (uint16_t)EEPROM_Read(EEPROM_START_ADDRESS + EEPROM_CURRENT_LO);
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