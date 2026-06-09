



//Write the includes here

#include "mcp47cxdx.h"
#include "mcc_generated_files/i2c_host/i2c1.h"
#include "mcc_generated_files/system/pins.h"

//Functions

/*TODO maybe write a init function but for the moment reset values are fine
void MCP47_Init(void) {
	//Build array for transmission
	uint8_t temp_transit_data[];
	temp_transit_data[0] = MCP47_VREF_REGISTER_ADDRESS;
	temp_transit_data[1] = 0x00;
	temp_transit_data[2] = 0x00;//Use VDD unbuffered as reference
	
	temp_transit_data[3] = MCP47_PD0_REGISTER_ADDRESS;
	temp_transit_data[4] = 0x00;
	temp_transit_data[5] = 0x00;//Use VDD unbuffered as reference
}
*/

void MCP47_SetOutput(MCP47_CANNEL_t channel, uint16_t value) {
	uint8_t temp_transit_data[3];
	if (channel == MCP47_CHANNEL_0) {
		temp_transit_data[0] = MCP47_DAC0_REGISTER_ADDRESS;
	}
	if (channel == MCP47_CHANNEL_1) {
		temp_transit_data[0] = MCP47_DAC1_REGISTER_ADDRESS;
	}

	//build array for transmission
	temp_transit_data[1] = (value >> 8);
	temp_transit_data[2] = (value & 0xFF);
	
	//Start i2c transmission and wait for it to complete
	I2C1_Host.Write(MCP47_I2C_Address,temp_transit_data,3);
	while (I2C1_Host.IsBusy()) {
		I2C1_Host.Tasks();
	}
	
	//strobe Latch pin to set output of the dac
	IO_RC5_SetLow();
	NOP(); NOP();//min time is 20ns
	IO_RC5_SetHigh();
}

void MCP47_PowerDown(void) {

}

void MCP47_PowerUp(void) {

}


