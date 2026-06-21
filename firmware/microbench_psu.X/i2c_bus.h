#ifndef I2C_BUS_H
#define	I2C_BUS_H

#include <xc.h>
#include <stdbool.h>

// Returns false if the transfer did not complete within a bounded timeout
// (stuck bus, unresponsive device), instead of hanging forever.
bool I2C_Write(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght);
bool I2C_Read(uint8_t clientAddress, uint8_t *data, uint8_t dataLenght);

#endif	/* I2C_BUS_H */
