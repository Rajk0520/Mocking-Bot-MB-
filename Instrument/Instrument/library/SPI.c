/*
 * SPI.c
 *
 * Created: 30-12-2018 09:24:29
 *  Author: ERTS
 */ 


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


#include "SPI.h"


// to configure the SPI pins
void spi_pin_config()
{
	DDRB = DDRB | ((1 << CS) | (1 << SCK) | (1 << MOSI));		// set only SS, SCK and MOSI pins as output and MISO as input
	PORTB = PORTB | ((1 << CS) | (1 << SCK) | (1 << MOSI));
}


// to initialize SPI protocol
void spi_init()
{
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
	SPSR = 0x00;
	SPDR = 0x00;
}


// start SPI communication by making CS low
void spi_cs_low()
{
	clear_bit(PORTB, CS);
}


// stop SPI communication by making CS high
void spi_cs_high()
{
	set_bit(PORTB, CS);
}


// send data to slave
unsigned char spi_send_data(unsigned char data)
{
	SPDR = data;
	
	asm volatile("nop");
	
	while(!(SPSR & (1<<SPIF)));		// wait for transmission to complete
	
	data = SPDR;
	
	return data;
}


// receive data from slave
unsigned char spi_receive_data()
{
	unsigned char data;
	
	spi_send_data(0xFF);			// send dummy data to slave
	
	data = SPDR;
	
	return data;
}



