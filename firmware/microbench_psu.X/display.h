#ifndef DISPLAY_H
#define	DISPLAY_H

#include <stdint.h>

#define LED_RED    0b10
#define LED_GREEN  0b01
#define LED_YELLOW 0b11
#define LED_OFF    0b00

#define UNIT_Amps  0
#define UNIT_Volts 1

void Display_Init(void);
void writeDisplay(float number);
void writeUnit(uint8_t unit);
void displayCommit(void);
void displayBlank(void);
void setLeds(uint8_t ledTop, uint8_t ledBottom);

#endif	/* DISPLAY_H */
