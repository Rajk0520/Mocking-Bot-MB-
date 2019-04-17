/*
 * SPI.h
 *
 * Created: 30-12-2018 09:25:22
 *  Author: ERTS
 */ 


#ifndef SPI_H_
#define SPI_H_


/****************************************************************/
/*																*/
/*		 		+===============================+				*/
/*				|	SPI Pins on ATmega2560		|				*/
/*				+===============================+				*/
/*																*/
/*		PB0 -- CS (Output)		|	PB1 -- SCK (Output)			*/
/*		PB2 -- MOSI (Output)	|	PB3 -- MISO (Input)			*/
/*	                                                            */
/****************************************************************/


#include <avr/io.h>


// SPI Port and Pins of ATmega2560

#define spi_port PORTB

#define CS		0
#define SCK		1
#define MOSI	2
#define MISO	3

#define set_bit(reg, bit)		reg |= (1 << bit)		// macro definition for setting a bit in a register
#define clear_bit(reg, bit)		reg &= ~(1 << bit)		// macro definition for clearing a bit in a register


// SPCR (SPI Control Register) bits

#define SPIE	7		// SPI Interrupt Enable
#define SPE		6		// SPI Enable
#define DORD	5		// Data Order
#define MSTR	4		// Master/Slave Select
#define CPOL	3		// Clock Polarity
#define CPHA	2		// Clock Phase
#define SPR1	1		// SPI Clock Rate 1
#define SPR0	0		// SPI Clock Rate 0


// SPSR (SPI Status Register) bits

#define SPIF	7		// SPI Interrupt Flag
#define WCOL	6		// Write Collision Flag
#define SPI2X	0		// Double SPI Speed Bit


// SPI related functions

void spi_pin_config();									// to configure the SPI pins

void spi_init();										// to initialize SPI protocol

void spi_cs_low();										// start SPI communication by making CS low

void spi_cs_high();										// stop SPI communication by making CS high

unsigned char spi_send_data(unsigned char data);		// send data to slave

unsigned char spi_receive_data();						// receive data from slave


#endif /* SPI_H_ */