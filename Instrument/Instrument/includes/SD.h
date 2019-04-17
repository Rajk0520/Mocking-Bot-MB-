/*
 * SD.h
 *
 * Created: 30-12-2018 09:27:29
 *  Author: ERTS
 */ 


#ifndef SD_H_
#define SD_H_


#define F_CPU 14745600

#include <util/delay.h>
#include "SPI.h"
#include "lcd.h"


// SD card commands

#define GO_IDLE_STATE			0		// resets the SD memory card (CMD0)
#define SEND_OP_COND			1		// sends SD memory card support information to
										// activate the card's initialization process (CMD1)
#define SEND_IF_COND			8		// send SD memory card interface condition to
										// provide supply voltage range and version (CMD8)
#define SEND_CSD				9		// ask the card to send its card specific data register - CSD (CMD9)
#define SEND_CID				10		// ask the card to send its card identification register - CID (CMD10)
#define STOP_TRANSMISSION		12		// force the card to stop transmission
										// in Multiple Read Block operation (CMD12)
#define SET_BLOCKLEN			16		// sets a block length in bytes for all read and write commands
										// for SDHC, block length are fixed to 512 bytes (CMD16)
#define READ_SINGLE_BLOCK		17		// reads a block of size set by SET_BLOCKLEN command (CMD17)
#define READ_MULTIPLE_BLOCK		18		// continuously transfer data blocks from card to host
										// until interrupted by a STOP_TRANSMISSION command (CMD18)
#define WRITE_SINGLE_BLOCK		24		// writes a block of size set by SET_BLOCKLEN command (CMD24)
#define WRITE_MULTIPLE_BLOCK	25		// continuously writes data blocks from host to card
										// until interrupted by a STOP_TRANSMISSION command (CMD25)
#define SDHC_SEND_OP_COND		41		// for SDHC, send support information and activate the card's
										// initialization process (ACMD41), it has to be preceded with APP_CMD (CMD55)
#define APP_CMD					55		// defines to the SD card that the next command is an
										// application specific command rather than a standard command
#define READ_OCR				58		// reads the OCR (Operation Condition) register of a card,
										// CCS (card capacity status) bit is assigned to OCR[30] (CMD58)


// types of responses

#define R1_IDLE_STATE		0x01		// card is in idle state
#define R1_ILLEGAL_CMD		0x04		// card received illegal command
#define R1_CRC_ERROR		0x08		// CRC error in command
#define R1_SEQ_ERROR		0x10		// erase sequence error
#define R1_ADD_ERROR		0x20		// address error
#define R1_PARA_ERR			0x40		// parameter error


// global variables

unsigned char SDHC_flag, card_type, buffer[512];
unsigned int block_of_cluster;
unsigned long curr_pos, curr_cluster, first_cluster, cluster_start_sector;


// member functions

// initialize SD card
unsigned char sd_card_init();

// send specific cmd to SD card
unsigned char sd_card_send_command(unsigned char cmd, unsigned long arg);

// read single block of data from SD card
unsigned char read_single_block(unsigned int block_addr);


#endif /* SD_H_ */