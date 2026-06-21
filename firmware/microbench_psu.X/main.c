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
#include "display.h"
#include "encoder.h"
#include "adc.h"
#include "psu.h"
#include "scpi.h"
#include <stdbool.h>
#include <stdint.h>

#define VOLTAGE_STEP_MV  50
#define CURRENT_STEP_MA  5

#define SETPOINT_DISPLAY_TICKS  20  // 20 @ 50 ms = 1.0 s
#define LONG_PRESS_TICKS        20  // 20 @ 50 ms = 1.0 s

typedef enum {
	EDIT_VOLTAGE,
	EDIT_CURRENT
} edit_mode_t;

int main(void) {
	SYSTEM_Initialize();

	TMR4_OverflowCallbackRegister(encoder_task);
	INTERRUPT_GlobalInterruptEnable();

	Display_Init();
	PSU_Init();

	edit_mode_t edit_mode = EDIT_VOLTAGE;
	bool standby = false;
	uint8_t setpoint_display_timer = 0;
	uint8_t long_press_counter = 0;
	uint8_t led_flash_counter = 0;

	uint8_t prev_btn_enc    = 1;
	uint8_t prev_btn_enable = 1;

	while (1) {
		CLRWDT(); // pet the watchdog once per loop pass (~50 ms normally, bounded well under the ~2 s WDT period)
		SCPI_Task();

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
				PSU_SetOutputEnabled(false);
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
				PSU_SetOutputEnabled(!PSU_IsOutputEnabled());
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
				int32_t new_v = (int32_t)PSU_GetVoltageSetpoint() + (int32_t)delta * VOLTAGE_STEP_MV;
				if (new_v < VOLTAGE_MIN_MV) new_v = VOLTAGE_MIN_MV;
				PSU_SetVoltageSetpoint((uint16_t)new_v);
			} else {
				int32_t new_i = (int32_t)PSU_GetCurrentSetpoint() + (int32_t)delta * CURRENT_STEP_MA;
				if (new_i < CURRENT_MIN_MA) new_i = CURRENT_MIN_MA;
				PSU_SetCurrentSetpoint((uint16_t)new_i);
			}
			if (PSU_IsOutputEnabled()) setpoint_display_timer = SETPOINT_DISPLAY_TICKS;
		}
		if (setpoint_display_timer > 0) setpoint_display_timer--;

		// --- PSU housekeeping: over-temperature protection + deferred EEPROM write ---
		PSU_Task();

		// --- display ---
		if (PSU_IsOverTemp()) {
			writeDisplay(PSU_GetTempC());
			writeUnit(UNIT_Volts); // no temperature unit symbol available, blank is clearest
			displayCommit();
		} else {
			bool show_setpoint = !PSU_IsOutputEnabled() || (setpoint_display_timer > 0);

			if (edit_mode == EDIT_VOLTAGE) {
				float display_val = show_setpoint
					? PSU_GetVoltageSetpoint() / 1000.0f
					: ADC_ReadVoltageV();
				writeDisplay(display_val);
				writeUnit(UNIT_Volts);
			} else {
				float display_val = show_setpoint
					? PSU_GetCurrentSetpoint() / 1000.0f
					: ADC_ReadCurrentA();
				writeDisplay(display_val);
				writeUnit(UNIT_Amps);
			}
			displayCommit();
		}

		// --- LEDs: both flash green when output on, both off when output off ---
		led_flash_counter++;
		if (PSU_IsOutputEnabled()) {
			uint8_t led = (led_flash_counter & 0x08) ? LED_GREEN : LED_OFF;
			setLeds(led, led);
		} else {
			setLeds(LED_OFF, LED_OFF);
		}

		__delay_ms(50);
	}
}
