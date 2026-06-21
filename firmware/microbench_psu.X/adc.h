#ifndef ADC_H
#define	ADC_H

#include <stdint.h>

#define ADC_ADDR_VOLTAGE  0b1001001
#define ADC_ADDR_CURRENT  0b1001000
#define ADC_ADDR_TEMP     0b1001010

// Raw 12-bit reading, averaged over several samples.
uint16_t ADC_ReadRaw(uint8_t address);

// Measured values scaled to physical units, for the front panel display.
float ADC_ReadVoltageV(void); // measured output voltage in volts
float ADC_ReadCurrentA(void); // measured output current in amps
float ADC_ReadTempC(void);    // measured regulator temperature in degC

// Same measurements as milli-unit integers, for SCPI response formatting.
int32_t ADC_ReadVoltageMv(void);
int32_t ADC_ReadCurrentMa(void);
int32_t ADC_ReadTempMilliC(void);

#endif	/* ADC_H */
