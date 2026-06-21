#include "scpi.h"
#include "mcc_generated_files/system/system.h"
#include "mcc_generated_files/uart/uart1.h"
#include "psu.h"
#include "adc.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#define SCPI_LINE_MAX        32
#define SCPI_BYTE_TIMEOUT_MS 20   // max time to wait for the next byte once a line has started
#define SCPI_LINE_TIMEOUT_MS 200  // hard cap on total time spent draining one line

static char scpi_line[SCPI_LINE_MAX];

static void UART_WriteChar(char c) {
	while (!UART1_IsTxReady());
	UART1_Write((uint8_t)c);
}

static void UART_WriteString(const char *s) {
	while (*s) {
		UART_WriteChar(*s++);
	}
}

static void UART_WriteUint16(uint16_t value) {
	char buf[6];
	uint8_t i = 0;
	if (value == 0) {
		UART_WriteChar('0');
		return;
	}
	while (value > 0) {
		buf[i++] = (char)('0' + (value % 10));
		value /= 10;
	}
	while (i > 0) {
		UART_WriteChar(buf[--i]);
	}
}

// Writes a milli-unit integer (e.g. 12500 -> "12.500")
static void UART_WriteMilli(int32_t milli) {
	if (milli < 0) {
		UART_WriteChar('-');
		milli = -milli;
	}
	uint16_t whole = (uint16_t)(milli / 1000);
	uint16_t frac  = (uint16_t)(milli % 1000);
	UART_WriteUint16(whole);
	UART_WriteChar('.');
	UART_WriteChar((char)('0' + (frac / 100) % 10));
	UART_WriteChar((char)('0' + (frac / 10) % 10));
	UART_WriteChar((char)('0' + frac % 10));
}

// Parses a decimal string (e.g. "12.5") into milli-units (12500)
static int32_t SCPI_ParseMilli(const char *str) {
	bool negative = false;
	if (*str == '-') {
		negative = true;
		str++;
	}
	int32_t whole = 0;
	while (*str >= '0' && *str <= '9') {
		whole = whole * 10 + (*str - '0');
		str++;
	}
	int32_t frac = 0;
	uint8_t frac_digits = 0;
	if (*str == '.') {
		str++;
		while (*str >= '0' && *str <= '9' && frac_digits < 3) {
			frac = frac * 10 + (*str - '0');
			frac_digits++;
			str++;
		}
	}
	while (frac_digits < 3) {
		frac *= 10;
		frac_digits++;
	}
	int32_t result = whole * 1000 + frac;
	return negative ? -result : result;
}

static void SCPI_ProcessLine(char *line) {
	for (char *p = line; *p; p++) {
		*p = (char)toupper((unsigned char)*p);
	}

	char *space = strchr(line, ' ');
	char *arg = NULL;
	if (space) {
		*space = '\0';
		arg = space + 1;
	}

	if (strcmp(line, "*IDN?") == 0) {
		UART_WriteString("TychoBECH,MicrobenchPSU,1,1.0\r\n");

	} else if (strcmp(line, "*RST") == 0) {
		PSU_SetOutputEnabled(false);
		PSU_SetVoltageSetpoint(0);
		PSU_SetCurrentSetpoint(0);

	} else if (strcmp(line, "OUTP?") == 0 || strcmp(line, "OUTPUT?") == 0) {
		UART_WriteString(PSU_IsOutputEnabled() ? "1\r\n" : "0\r\n");

	} else if (strcmp(line, "OUTP") == 0 || strcmp(line, "OUTPUT") == 0) {
		if (arg) {
			if (strcmp(arg, "ON") == 0 || strcmp(arg, "1") == 0) {
				PSU_SetOutputEnabled(true);
			} else if (strcmp(arg, "OFF") == 0 || strcmp(arg, "0") == 0) {
				PSU_SetOutputEnabled(false);
			}
		}

	} else if (strcmp(line, "VOLT?") == 0 || strcmp(line, "VOLTAGE?") == 0) {
		UART_WriteMilli((int32_t)PSU_GetVoltageSetpoint());
		UART_WriteString("\r\n");

	} else if (strcmp(line, "VOLT") == 0 || strcmp(line, "VOLTAGE") == 0) {
		if (arg) {
			int32_t mv = SCPI_ParseMilli(arg);
			if (mv < 0) mv = 0;
			PSU_SetVoltageSetpoint((uint16_t)mv);
		}

	} else if (strcmp(line, "CURR?") == 0 || strcmp(line, "CURRENT?") == 0) {
		UART_WriteMilli((int32_t)PSU_GetCurrentSetpoint());
		UART_WriteString("\r\n");

	} else if (strcmp(line, "CURR") == 0 || strcmp(line, "CURRENT") == 0) {
		if (arg) {
			int32_t ma = SCPI_ParseMilli(arg);
			if (ma < 0) ma = 0;
			PSU_SetCurrentSetpoint((uint16_t)ma);
		}

	} else if (strcmp(line, "MEAS:VOLT?") == 0 || strcmp(line, "MEASURE:VOLTAGE?") == 0) {
		UART_WriteMilli(ADC_ReadVoltageMv());
		UART_WriteString("\r\n");

	} else if (strcmp(line, "MEAS:CURR?") == 0 || strcmp(line, "MEASURE:CURRENT?") == 0) {
		UART_WriteMilli(ADC_ReadCurrentMa());
		UART_WriteString("\r\n");

	} else if (strcmp(line, "MEAS:TEMP?") == 0 || strcmp(line, "MEASURE:TEMPERATURE?") == 0) {
		UART_WriteMilli(ADC_ReadTempMilliC());
		UART_WriteString("\r\n");

	} else if (strcmp(line, "SYST:ERR?") == 0 || strcmp(line, "SYSTEM:ERROR?") == 0) {
		UART_WriteString(PSU_IsOverTemp() ? "OVERTEMP\r\n" : "0\r\n");

	} else if (line[0] != '\0') {
		UART_WriteString("ERR\r\n");
	}
}

// There is no RX FIFO/interrupt wired up, so once a line starts arriving we must
// drain it with tight polling rather than returning to the slow ~50 ms main loop,
// or later bytes get silently dropped by the single-byte hardware receive register.
void SCPI_Task(void) {
	if (!UART1_IsRxReady()) {
		return;
	}

	uint8_t line_len = 0;
	uint16_t total_waited_ms = 0;

	while (1) {
		uint16_t waited_ms = 0;
		while (!UART1_IsRxReady()) {
			__delay_ms(1);
			waited_ms++;
			total_waited_ms++;
			if (waited_ms >= SCPI_BYTE_TIMEOUT_MS || total_waited_ms >= SCPI_LINE_TIMEOUT_MS) {
				return; // incomplete line -- drop it
			}
		}

		uint8_t c = UART1_Read();
		if (c == '\r' || c == '\n') {
			if (line_len > 0) {
				scpi_line[line_len] = '\0';
				SCPI_ProcessLine(scpi_line);
			}
			return;
		}
		if (line_len < SCPI_LINE_MAX - 1) {
			scpi_line[line_len++] = (char)c;
		}
	}
}
