#include "encoder.h"
#include "mcc_generated_files/system/pins.h"

static const int8_t encoderLut[16] = {
	0, -1, 1, 0,
	1, 0, 0, -1,
	-1, 0, 0, 1,
	0, 1, -1, 0
};

static volatile int16_t encoder_delta = 0;
static uint8_t prev_state = 0;

static uint8_t encoder_read_pins(void) {
	uint8_t a = IO_RC7_GetValue();
	uint8_t b = IO_RC6_GetValue();
	return (uint8_t)((a << 1) | b);
}

void encoder_task(void) {
	uint8_t curr_state = encoder_read_pins();
	uint8_t index = (uint8_t)((prev_state << 2) | curr_state);
	encoder_delta += encoderLut[index];
	prev_state = curr_state;
}

int16_t encoder_get_delta(void) {
	int16_t delta = encoder_delta;
	encoder_delta = 0;
	return delta;
}
