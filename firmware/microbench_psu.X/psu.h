#ifndef PSU_H
#define	PSU_H

#include <stdint.h>
#include <stdbool.h>

#define VOLTAGE_MIN_MV   0
#define VOLTAGE_MAX_MV   24000

#define CURRENT_MIN_MA   0
#define CURRENT_MAX_MA   2000

// Loads saved setpoints from EEPROM and puts the output into a safe (off) state.
void PSU_Init(void);

// Call once per main loop iteration: runs over-temperature protection and
// the deferred EEPROM write (so rapid setpoint changes only cost one write cycle).
void PSU_Task(void);

void PSU_SetOutputEnabled(bool enable);
void PSU_SetVoltageSetpoint(uint16_t milivolt);
void PSU_SetCurrentSetpoint(uint16_t miliamp);

bool PSU_IsOutputEnabled(void);
bool PSU_IsOverTemp(void);
uint16_t PSU_GetVoltageSetpoint(void);
uint16_t PSU_GetCurrentSetpoint(void);

// Regulator temperature as of the last PSU_Task() call (degC).
float PSU_GetTempC(void);

#endif	/* PSU_H */
