#include "adc.h"
#include "i2c_bus.h"

#define ADC_AVG_SAMPLES 20

// Scaling: adjust to match the hardware (voltage divider / current sense gain / thermistor)
#define VOLTAGE_ADC_MV_PER_LSB  12.15f  // calibrated against hardware voltage divider
#define CURRENT_ADC_MA_PER_LSB  1.1745f // 5000 mV / 4095 LSB / 1.03958 mV per mA
#define TEMP_ADC_C_PER_LSB      0.0407f // 5000 mV / 4095 LSB / 30 mV per degC

// Returned when an I2C transfer fails (bus stuck, ADC unresponsive). Deliberately
// the max 12-bit value so a fault reads as "hot"/"high" rather than silently
// looking like zero -- a failed temperature read must never look safe.
#define ADC_FAULT_RAW_VALUE 0x0FFF

uint16_t ADC_ReadRaw(uint8_t address) {
	uint8_t data[2];
	uint32_t sum = 0;
	for (uint8_t i = 0; i < ADC_AVG_SAMPLES; i++) {
		if (!I2C_Read(address, data, 2)) {
			return ADC_FAULT_RAW_VALUE;
		}
		sum += ((uint16_t)(data[0] & 0x0F) << 8) | data[1];
	}
	return (uint16_t)(sum / ADC_AVG_SAMPLES);
}

float ADC_ReadVoltageV(void) {
	return ADC_ReadRaw(ADC_ADDR_VOLTAGE) * VOLTAGE_ADC_MV_PER_LSB / 1000.0f;
}

float ADC_ReadCurrentA(void) {
	return ADC_ReadRaw(ADC_ADDR_CURRENT) * CURRENT_ADC_MA_PER_LSB / 1000.0f;
}

float ADC_ReadTempC(void) {
	return ADC_ReadRaw(ADC_ADDR_TEMP) * TEMP_ADC_C_PER_LSB;
}

int32_t ADC_ReadVoltageMv(void) {
	return (int32_t)(ADC_ReadRaw(ADC_ADDR_VOLTAGE) * VOLTAGE_ADC_MV_PER_LSB);
}

int32_t ADC_ReadCurrentMa(void) {
	return (int32_t)(ADC_ReadRaw(ADC_ADDR_CURRENT) * CURRENT_ADC_MA_PER_LSB);
}

int32_t ADC_ReadTempMilliC(void) {
	return (int32_t)(ADC_ReadRaw(ADC_ADDR_TEMP) * TEMP_ADC_C_PER_LSB * 1000.0f);
}
