#ifndef SCPI_H
#define	SCPI_H

// Polls UART1 for an incoming command line and processes it if a full line has
// arrived. Call once per main loop iteration.
void SCPI_Task(void);

#endif	/* SCPI_H */
