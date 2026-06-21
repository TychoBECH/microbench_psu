#include "psu.h"
#include "adc.h"
#include "eeprom_settings.h"
#include "mcp47cxdx.h"
#include "mcc_generated_files/system/pins.h"

#define TEMP_SHUTDOWN_C  85.0f
#define TEMP_RECOVER_C   70.0f

#define EEPROM_WRITE_TICKS  100  // 100 @ 50 ms = 5.0 s, see PSU_Task()

static uint16_t g_voltage_set_point  = 3300;
static uint16_t g_current_set_point  = 100;
static bool     g_output_enabled     = false;
static bool     g_over_temp          = false;
static uint8_t  g_eeprom_dirty_timer = 0;
static float    g_last_temp_c        = 0.0f;

static void setOutputVoltage(uint16_t milivolt) {
	uint16_t DAC_value = ((uint32_t) milivolt * 10600) >> 16;
	if (DAC_value > 4095) {
		DAC_value = 4095;
	}
	MCP47_SetOutput(MCP47_CHANNEL_1, DAC_value);
}

static void setCurrentLimit(uint16_t miliamp) {
	uint16_t DAC_value = ((uint32_t) miliamp * 16500) >> 16;
	if (DAC_value > 4095) {
		DAC_value = 4095;
	}
	MCP47_SetOutput(MCP47_CHANNEL_0, DAC_value);
}

void PSU_Init(void) {
	settings_load(&g_voltage_set_point, &g_current_set_point);
	IO_RA4_SetLow();
	setOutputVoltage(0);
	setCurrentLimit(g_current_set_point);
}

void PSU_SetOutputEnabled(bool enable) {
	if (enable && g_over_temp) return; // refuse to enable while over temperature
	g_output_enabled = enable;
	if (enable) {
		IO_RA4_SetHigh();
		setOutputVoltage(g_voltage_set_point);
	} else {
		IO_RA4_SetLow();
		setOutputVoltage(0);
	}
}

void PSU_SetVoltageSetpoint(uint16_t milivolt) {
	if (milivolt > VOLTAGE_MAX_MV) milivolt = VOLTAGE_MAX_MV;
	g_voltage_set_point = milivolt;
	if (g_output_enabled) setOutputVoltage(milivolt);
	g_eeprom_dirty_timer = EEPROM_WRITE_TICKS;
}

void PSU_SetCurrentSetpoint(uint16_t miliamp) {
	if (miliamp > CURRENT_MAX_MA) miliamp = CURRENT_MAX_MA;
	g_current_set_point = miliamp;
	setCurrentLimit(miliamp);
	g_eeprom_dirty_timer = EEPROM_WRITE_TICKS;
}

bool PSU_IsOutputEnabled(void) {
	return g_output_enabled;
}

bool PSU_IsOverTemp(void) {
	return g_over_temp;
}

uint16_t PSU_GetVoltageSetpoint(void) {
	return g_voltage_set_point;
}

uint16_t PSU_GetCurrentSetpoint(void) {
	return g_current_set_point;
}

float PSU_GetTempC(void) {
	return g_last_temp_c;
}

void PSU_Task(void) {
	g_last_temp_c = ADC_ReadTempC();
	if (!g_over_temp && g_last_temp_c >= TEMP_SHUTDOWN_C) {
		g_over_temp = true;
		PSU_SetOutputEnabled(false);
	} else if (g_over_temp && g_last_temp_c < TEMP_RECOVER_C) {
		g_over_temp = false;
	}

	if (g_eeprom_dirty_timer > 0) {
		g_eeprom_dirty_timer--;
		if (g_eeprom_dirty_timer == 0) {
			settings_save(g_voltage_set_point, g_current_set_point);
		}
	}
}
