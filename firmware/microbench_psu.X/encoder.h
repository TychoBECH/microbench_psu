#ifndef ENCODER_H
#define	ENCODER_H

#include <stdint.h>

// Registered as the TMR4 overflow callback to sample the quadrature pins.
void encoder_task(void);

// Returns the accumulated step count since the last call and resets it to zero.
int16_t encoder_get_delta(void);

#endif	/* ENCODER_H */
