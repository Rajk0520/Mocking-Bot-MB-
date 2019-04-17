/*
 * SD.c
 *
 * Created: 30-12-2018 09:28:29
 *  Author: ERTS
 */ 


#include "SD.h"
#include "SPI.h"


// initialize SD card
unsigned char sd_card_init()
{
	unsigned char i, response;
	unsigned int retry = 0;
	unsigned long arg;
	
	spi_pin_config();
	
	spi_init();
	
	for (i = 0; i < 10; i++)
	{
		spi_send_data(0xFF);
	}
	
	spi_cs_low();				// start SPI communication by driving CS low
	
	// resets the SD memory card (GO_IDLE_STATE or CMD0)
	
	do
	{
		response = sd_card_send_command(GO_IDLE_STATE, 0);
		retry++;
		if (retry > 0xFE)		// time out, card not detected
		{
			return 0;
		}
		
	} while (response != 0x01);
	
	/*
	lcd_clear();
	lcd_string(1, 1, "card: set idle");
	_delay_ms(500);
	*/
	
	retry = 0;
	
	// send SD memory card interface condition to
	// provide supply voltage range and version (SEND_IF_COND or CMD8)
	
	do
	{
		response = sd_card_send_command(SEND_IF_COND, 0x1AA);
		retry++;
		if (retry > 0xFE)
		{
			card_type = 1;
			return 0;
		}
		
	} while (response != 0x01);
	
	for (i = 0; i < 3; i++)
	{
		spi_receive_data();
	}
	
	if (spi_receive_data() == 0xAA)
	{
		card_type = 2;
		/*
		lcd_clear();
		lcd_string(1, 1, "card version: 2");
		_delay_ms(500);
		*/
	}
	else
	{
		card_type = 1;
		/*
		lcd_clear();
		lcd_string(1, 1, "card version: 1");
		_delay_ms(500);
		*/
	}
	
	retry = 0;
	
	// for SDHC, send support information and
	// activate the card's initialization process (SDHC_SEND_OP_COND or ACMD41)
	
	arg = card_type == 2 ? 0x40000000 : 0;
	
	do
	{
		sd_card_send_command(APP_CMD, 0);
		response = sd_card_send_command(SDHC_SEND_OP_COND, arg);
		retry++;
		if (retry > 0xFE)
		{
			return 0;
		}
		
	} while (response != 0x00);
	
	retry = 0;
	
	// reads the OCR (Operation Condition) register of a card,
	// CCS (card capacity status) bit is assigned to OCR[30] (CMD58)
	
	if (card_type == 2)
	{
		sd_card_send_command(READ_OCR, 0);
	}
	
	spi_cs_high();				// stop SPI communication by driving CS high
	
	return 1;
}


// send command to SD card
unsigned char sd_card_send_command(unsigned char cmd, unsigned long arg)
{
	unsigned char response, status;
	unsigned char sd_card_crc = 0xFF;

	set_bit(spi_port, MOSI);							// drive MOSI pin to HIGH for send
	
	spi_send_data(0xFF);
	
	_delay_ms(10);
	
	spi_send_data(cmd | 0x40);							// send cmd with addition of number 64
	
	spi_send_data(arg >> 24);							// send four bytes of arguments
	spi_send_data(arg >> 16);
	spi_send_data(arg >> 8);
	spi_send_data(arg);
	
	if (cmd == GO_IDLE_STATE)							// correct CRC for GO_IDLE_STATE or CMD0 command
	{
		sd_card_crc = 0x95;
	} 
	else if (cmd == SEND_IF_COND)						// correct CRC for SEND_IF_COND or CMD8 command
	{
		sd_card_crc = 0x87;
	}
	
	spi_send_data(sd_card_crc);							// send last byte of CRC
	
	set_bit(spi_port, MISO);							// drive MISO pin to HIGH for receive
	
	for (int i = 0; ((response = spi_receive_data()) & 0x80) && i != 255; i++);
	
	if (response == 0x00 && cmd == READ_OCR)
	{
		status = spi_receive_data() & 0x40;
		
		if (status == 0x40)
		{
			SDHC_flag = 1;
			/*
			lcd_clear();
			lcd_string(1, 1, "card: SDHC");
			_delay_ms(500);
			*/
		} 
		else
		{
			SDHC_flag = 0;
			/*
			lcd_clear();
			lcd_string(1, 1, "card: not SDHC");
			_delay_ms(500);
			*/
		}
		
		spi_receive_data();
		spi_receive_data();
		spi_receive_data();
	}
	
	return response;
}


// read single block of data from SD card
unsigned char read_single_block(unsigned int block_addr)
{
	unsigned char response;
	unsigned int i, retry = 0;
	
	spi_cs_low();				// start SPI communication by driving CS low
	
	// ask the SD card to read from block_addr (READ_SINGLE_BLOCK or CMD17)
	
	do
	{
		response = sd_card_send_command(READ_SINGLE_BLOCK, block_addr);
		retry++;
		if (retry > 0xFE)		// time out, card not detected
		{
			return 0;
		}
		
	} while (response != 0x00);
	
	retry = 0;
	
	// wait for start block token (0xFE)
	while (spi_receive_data() != 0xFE)
	{
		if (retry++ > 0xFE)
		{
			return 0;
		}
	}
	/*
	lcd_clear();
	lcd_string(1, 1, "card: start read");
	_delay_ms(500);
	*/
	// empty the buffer
	for (i = 0; i < 512; i++)
	{
		buffer[i] = 0;
	}
	
	// read a block of data
	for (i = 0; i < 512; i++)
	{
		buffer[i] = spi_receive_data();
		
		// to read the block of 512 bytes received
		/*if (buffer[i] > 0)
		{
			lcd_clear();
			lcd_numeric_value(1, 1, buffer[i], 4);
			lcd_numeric_value(1, 8, i, 3);
			_delay_ms(500);
		}*/
	}
	
	spi_receive_data();			// receive incoming CRC (16-bit), CRC is ignored here
	spi_receive_data();
	
	spi_receive_data();			// extra 8 SCK pulses
	
	spi_cs_high();				// stop SPI communication by driving CS high
	
	return 1;
}




