/*
* Team Id:     278
* Author List: Raj kumar Bhagat
* Filename:    Instrument.c
* Theme:       Mocking Bot
* Functions:   timer0_init,  timer2_init,  timer1_pwm_init,  IRQ_interrupt_init,  uart0_init,  uart_tx_string,
               uart_tx, extract_headder, mpr121Write,  mpr121_initialise,  boot_switch_pin_config,  initialise_mpr_pin,
               initialise_device,   classify_key,   get_raw_values,  concatenate_filename_send_to_python,
			   read_current_file, classify_key_trumpet, mpr121_Read.
 DESCRIPTION :
				This program is for the final task MockingBot. This program has two part one before the
				boot button is pressed. Other is the execution part after the boot button is pressed.
 
 OVERALL FOLW OF THE PROGRAM :
 
				Before boot button :
 
				As we will power the board it will initialize the MPR121 touch sensor, SD CARD, and
				FAT32 on SD CARD. And will wait for the boot button to be pressed.
				When boot button is pressed it will send the '#' character to python.
 
				After boot button :
 
				After the boot button is pressed it will wait till the MPR121 touch sensor is touched. And
				after that it will get the corresponding file name to be played, read the file and finally 
				will play it in speaker.
				After this it will again wait for the next key to be pressed and will repeat the same. At the
				end it will send the '$' character to python.
*/
#define F_CPU 14745600
#include <avr/io.h>
#include <util/delay.h>
#include <FAT32.h>
//#include <lcd.h>
#include <SD.h>
#include <SPI.h>
#include <avr/interrupt.h>
#include <i2c.h>
#include <mpr121.h>

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ GLOBAL VARIABLES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
uint8_t mpradd = 0x5A;                                                             //mpr121 address


volatile uint8_t file_audio[512];    									       // to store the .wav file
volatile unsigned int k=0;                                                         // to play the .wav file
unsigned int sample_rate=0x00, bits_per_sample=0x00,data_start=0;
unsigned char bytes_per_sample=0x0;
volatile unsigned long play=0x00;
char file_name[12]={};                                                             // to store file name to be read from sd card



uint8_t sample_timer_data =0x00;
unsigned char status_mpr = 0x00, status_mpr_trumpet = 0x00, switch_flag=0x00;                                              // to read status, to read boot button
char keys[8] = {'1', 'C', 'D', 'E', 'F', 'G', 'A', 'B'};                                                                   // to store the key description of piano
char current_key='0';                                                                                                      // to hold the currently pressed key name
unsigned char instrument = 0x00;


unsigned char onset_counter=0x00;                                                                                           // to count the number of on set played
char configuration_table[10][2]={ {'5','N'}, {'5','Y'}, {'5','N'}, {'4','N'}, {'5','Y'}, {'4','N'}, {'6','Y'}, {'4','N'} };  // configuration table, only piano table is 
	                                                                                                                        // added as for progress task we don't need 
																			                                                // need trumpet part.*/                                                        
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/* 
 * Function Name: timer2_init
 * Input: None
 * Output: None
 * Logic: This function will create interrupt as per sample rate to play .wav file with 16 bit pwm in timer1.
 * Example Call: timer2_init();
 */
void timer2_init()
{
	TCNT2=0x00;
	TCCR2A=(1<<WGM21);                      //Timer 2 CTC Mode
	TIMSK2=(1<<OCIE2A);                     //Timer2 Interrupt on Compare Match A
	OCR2A= sample_timer_data;               // this value is as per sample rate of .wav file;
	TCCR2B=(1<<CS21);                       // 8 bit prescale;
}
/* 
 * Function Name: timer1_pwm_init
 * Input: None
 * Output: None
 * Logic: This function will make 8 bit pwm channel to play speaker.
 * Example Call: timer1_pwm_init();
 */
void timer1_pwm_init(){
	TCNT1H=0x00;
	TCNT1L=0x00;
	OCR1AH=0x00;
	OCR1AL=0x00;
	TCCR1A=(1<<WGM10)|(1<<COM1A1);
	TCCR1B=(1<<WGM12)|(1<<CS10);
}

/* 
 * Function Name: extract_headder
 * Input: None
 * Output: None
 * Logic: This function will read the header information of the wav file and retreave information like sample rate, 
          bits per sample etc.
 * Example Call: extract_headder();
 */

void extract_headder()
{
	int i;
	//uint16_t temp_sample=0, temp_sample2=0;
	for(i=24;i<28;i++)
	{
		sample_rate = sample_rate | (unsigned int)(buffer[i] << (8*(i-24)));
		
	}
	sample_timer_data = (1843200/sample_rate);                                                     //          ( F_CPU / 8(prescaler)) = 1843200 . this data is to be given to sample timer
	
	for(i=34;i<36;i++)
	{
		bits_per_sample = bits_per_sample | (buffer[i] << (8*(i-34)));
	}
	bytes_per_sample=bits_per_sample/8;
	for(i=16;i<20;i++)
	{
		data_start = data_start | (buffer[i] << (8*(i-16)));
	}
	data_start=data_start+28;
	/*
	lcd_clear();
	lcd_numeric_value(1,1,sample_rate,5);
	lcd_numeric_value(1,8,bits_per_sample,3);
	lcd_numeric_value(2,1,bytes_per_sample,3);
	lcd_numeric_value(2,8,data_start,3);
	_delay_ms(50);*/
}


/* 
 * Function Name: IRQ_interrupt_init
 * Input: None
 * Output: None
 * Logic: This function will initialize the external interrupt 4 for MPRsensor.
 * Example Call: IRQ_interrupt_init();
 */
void IRQ_interrupt_init(void) //      Making interrupt for IRQ pin from the mpr121 sensor
{
	EICRB=0x00;      //           making int4 interrupt response on low level
	EIMSK=(1<<INT4);
}
/* 
 * Function Name: uart0_init
 * Input: None
 * Output: None
 * Logic: This function will initialize uart0 with baud rate -> 9600, char size -> 8 bit, parity -> disabled.
 * Example Call: uart0_init();
 */
void uart0_init(void)
{
	UCSR0B = 0x00; //disable while setting baud rate
	UCSR0A = 0x00;
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);// same as 0x06
	UBRR0L = 0x5F; //set baud rate lo
	UBRR0H = 0x00; //set baud rate hi
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0);//same as 0x98
}
/* 
 * Function Name: uart_tx
 * Input: char data to be sent
 * Output: None
 * Logic: send a character byte to usart0.
 * Example Call: uart_tx();
 */
void uart_tx(char data)
{
	// waiting to transmit
	while(!(UCSR0A & (1 << UDRE0)));
	
	UDR0 = data;
}
/* 
 * Function Name: uart_tx_string
 * Input: sting pointer to be sent
 * Output: None
 * Logic: send a string to usart0.
 * Example Call: uart_tx_string();
 */
void uart_tx_string(char *data)
{
	while (*data != '\0')
	{
		uart_tx(*data);
		data++;
	}
}

/* 
 * Function Name: mpr121_Read
 * Input: address of the resister of mpr121 sensor to which we want to read.
 * Output: data in the input resister
 * Logic: This  will read the given resister data from the mpr121 sensor.
 * Example Call: mpr121_Read(0x00);
 */
char mpr121_Read(unsigned char address)
{
	char data;
	
	i2cSendStart();
	i2cWaitForComplete();
	
	i2cSendByte(((mpradd << 1) | 0x00));
	i2cWaitForComplete();
	
	i2cSendByte(address);	// write register address
	i2cWaitForComplete();
	
	i2cSendStart();
	
	i2cSendByte(((mpradd << 1) | 0x01));
	i2cWaitForComplete();
	i2cReceiveByte(TRUE);
	i2cWaitForComplete();
	
	data = i2cGetReceivedByte();	// Get MSB result
	i2cWaitForComplete();
	i2cSendStop();
	
	cbi(TWCR, TWEN);	// Disable TWI
	sbi(TWCR, TWEN);	// Enable TWI
	
	return data;
}

/* 
 * Function Name: mpr121Write
 * Input: char address of where to write and char data to be written.
 * Output: None
 * Logic: Function to write given data at the given address
 * Example Call: mpr121Write(0x00,0x00);
 */
void mpr121Write(unsigned char address, unsigned char data)
{
	i2cSendStart();
	i2cWaitForComplete();
	i2cSendByte(((mpradd << 1) | 0x00));
	i2cWaitForComplete();
	i2cSendByte(address);
	i2cWaitForComplete();
	i2cSendByte(data);
	i2cWaitForComplete();
	i2cSendStop();
	
}

/* 
 * Function Name: classify_key_trumpet
 * Input: status of mpr121 (0x01 resister).
 * Output: key which is pressed in trumpet.
 * Logic: this function will take the raw status data from mpr121 sensor and return the corresponding key that has been touched in trumpet
 * Example Call: mpr121Write(0x00,0x00);
 */
char classify_key_trumpet(unsigned char trumpet_byte)
{
	unsigned char a, trumpet_key;
	a=trumpet_byte&0b00000111;
	switch(a)
	{
		case 0b00000001 :
		trumpet_key='C';
		break;
		case 0b00000010 :
		trumpet_key='D';
		break;
		case 0b00000100 :
		trumpet_key='E';
		break;
		case 0b00000011 :
		trumpet_key='F';
		break;
		case 0b00000110 :
		trumpet_key='G';
		break;
		case 0b00000101 :
		trumpet_key='A';
		break;
		case 0b00000111 :
		trumpet_key='B';
		break;
		default:
		trumpet_key='0';
	}
	return(trumpet_key);
}

/* 
 * Function Name: classify_key
 * Input: char data which shows status of MPR121 electrodes.
 * Output: char key is has been pressed
 * Logic: This function will take the status of MPR121 electrode and return the key which has been pressed.
 * Example Call: classify_key(status_mpr);
 */
char classify_key_piano(unsigned char status_mpr)
{
	unsigned char i,j,count=0x00;
	for( i=0 ; i<8 ; i++)
	{
		if( (status_mpr & (1<<(7-i))) != 0 )
		{
			// (7-i)th key of mpr is touched;
			j=7-i;
			count=count+1;
			//return keys[7-i];
		}
	}
	if(count==1){
		return keys[j];
	}
	else{
		//lcd_clear();
		//lcd_string(1, 1, "multiple key");
		return '0';
	}
}

/* 
 * Function Name: mpr121_initialise
 * Input: None
 * Output: None
 * Logic: Function to initialize the mpr121 touch sensor.
 * Example Call: mpr121_initialise();
 */
void mpr121_initialise(void)     // initializing to read the touch status of the mpr121 sensor.
{
	mpr121Write(0x2B, 0x01);
	mpr121Write(0x2C, 0x01);
	mpr121Write(0x2D, 0x00);
	mpr121Write(0x2E, 0x00);

	mpr121Write(0x2F, 0x01);
	mpr121Write(0x30, 0x01);
	mpr121Write(0x31, 0xFF);
	mpr121Write(0x32, 0x02);
	
	// This group sets touch and release thresholds for each electrode
	mpr121Write(0x41, TOU_THRESH);
	mpr121Write(0x42, REL_THRESH);
	mpr121Write(0x43, TOU_THRESH);
	mpr121Write(0x44, REL_THRESH);
	mpr121Write(0x45, TOU_THRESH);
	mpr121Write(0x46, REL_THRESH);
	mpr121Write(0x47, TOU_THRESH);
	mpr121Write(0x48, REL_THRESH);
	mpr121Write(0x49, TOU_THRESH);
	mpr121Write(0x4A, REL_THRESH);
	mpr121Write(0x4B, TOU_THRESH);
	mpr121Write(0x4C, REL_THRESH);
	mpr121Write(0x4D, TOU_THRESH);
	mpr121Write(0x4E, REL_THRESH);
	mpr121Write(0x4F, TOU_THRESH);
	mpr121Write(0x50, REL_THRESH);
	
	mpr121Write(0x51, TOU_THRESH);
	mpr121Write(0x52, REL_THRESH);
	mpr121Write(0x53, TOU_THRESH);
	mpr121Write(0x54, REL_THRESH);
	mpr121Write(0x55, TOU_THRESH);
	mpr121Write(0x56, REL_THRESH);

	
	mpr121Write(0x5D, 0x04);
	
	mpr121Write(0x5E, 0b10001011);   // start run mode for first 11 electrodes
	
}



/* 
 * Function Name: get_raw_value
 * Input: None
 * Output: None
 * Logic: This function will read the status byte of MPR121 and will call the function to play the particular audio file.
 * Example Call: get_raw_value();
 */
void get_raw_values() {
	
	char key;
	
	status_mpr = mpr121_Read(0x00);
	status_mpr_trumpet = mpr121_Read(0x01);
	
	if(status_mpr!=0)
	{
		TIMSK2=0x00;            // stop whatever is playing
		TCCR1B = 0x00;
		
		key = classify_key_piano(status_mpr);
		if(key!='0'){
			
			current_key = key;
			instrument = 0x01;
			
			read_current_file(current_key);              // function to read and play the required file.
		}
		else{
			//lcd_string(2, 1, "ERROR");
		}
		status_mpr=0x00;
	}
	else if (status_mpr_trumpet !=0 )
	{
		/*lcd_clear();
		lcd_wr_char(1, 1, key);*/
		_delay_ms(200);
		status_mpr_trumpet = mpr121_Read(0x01);            // read once more with 200 ms delay and update the status.(to avoid the error due to different response time in servo)
		
		
		if(status_mpr_trumpet != 0)
		{
			TIMSK2=0x00;            // stop whatever is playing
			TCCR1B = 0x00;
			
			key = classify_key_trumpet(status_mpr_trumpet);
		
		    if(key!='0')
			{
				
				current_key = key;
				instrument = 0x00;
			
				read_current_file(current_key);                         // function to read and play the required file.
		    }
		    else
			{
				//lcd_string(2, 1, "INVALID");
		    }
		    status_mpr_trumpet=0x00;
			
		}
		
	}
	
	
}

/* 
 * Function Name: boot_switch_pin_config
 * Input: None
 * Output: None
 * Logic: to give internal pull-up in boot button
 * Example Call: boot_switch_pin_config();
 */
void boot_switch_pin_config()
{
	DDRD  = DDRD & 0xBF;		// set PD.6 as input
	PORTD = PORTD | 0x40;		// set PD.6 HIGH to enable the internal pull-up
}
/* 
 * Function Name: initialise_mpr_pin
 * Input: None
 * Output: None
 * Logic: to initialize the pin configuration to use mpe through I2C protocol
 * Example Call: initialise_mpr_pin();
 */
void initialise_mpr_pin(void)
{
	DDRE|=0b00000000;//           Pull-up on int4 pin for IRQ pin from the sensor.
	PORTE|=0b00010000;
	
	DDRD|=0x00;//                 Pull-up for the SDA and SCL line.
	PORTD|=0b00000011;
}
/* 
 * Function Name: initialise_device
 * Input: None
 * Output: None
 * Logic: This function will initialize all the components that we are using. like LCD, MPR121, SD CARD and FAT32
 * Example Call: initialise_device();
 */
void initialise_device(void)
{
	// lcd initialization
	//lcd_port_config();
	//lcd_init();
	//lcd_string(1,1, "LCD INIT.");
	//_delay_ms(200);
	
	// mpr121 initialization
	initialise_mpr_pin();
	sei();
	i2cInit();
	mpr121_initialise();
	//lcd_string(1, 1, "MPR START");
	_delay_ms(50);
	
	// SD card and FAT32 initialization
	
	DDRB|=0b00100000;
	PORTB&=0b11011111;

	if (sd_card_init())
	{
		//lcd_clear();
		//lcd_string(1,1,"SD CARD READY");
		_delay_ms(50);
	}
	else
	{
		//lcd_clear();
		//lcd_string(1,1,"CARD NOT FOUND");
		while(1);
	}
	if (get_boot_sector_data())
	{
		//lcd_clear();
		//lcd_string(1,1,"FAT32 READY");
		_delay_ms(50);
	}
	else
	{
		//lcd_clear();
		//lcd_string(1,1,"FAT32 ERROR");
		while(1);
	}
}

/* 
 * Function Name: concatenate_filename_send_to_python
 * Input: key which has been pressed
 * Output: None
 * Logic: This function will make the file name by concatenating current_key and the configuration_table given and
          will save the same in file_name string. And will send the file_name to python through USB.
 * Example Call: concatenate_filename_send_to_python('A');
 */
void concatenate_filename_send_to_python(char key)
{
	
	//file_name=key+(sharp note)+(octave)+(_)+(Pia)/(Tru)+(.wav)
	
	
	unsigned char index=0;
	uart_tx(key);
	file_name[index]=key;
	index = index+1;
	if (configuration_table[onset_counter][1] == 'Y')
	{
		file_name[index] = '#';
		index = index+1;
		uart_tx('#');
	}
	file_name[index]=configuration_table[onset_counter][0];
	uart_tx(configuration_table[onset_counter][0]);
	uart_tx_string("\n");
	index = index+1;
	file_name[index]='_';
	index = index+1;
	
	if(instrument == 0x01)
	{
		file_name[index] = 'P';
		index = index+1;
		file_name[index] = 'i';
		index = index+1;
		file_name[index] = 'a';
		index = index+1;
	}
	else
	{
		file_name[index] = 'T';
		index = index+1;
		file_name[index] = 'r';
		index = index+1;
		file_name[index] = 'u';
		index = index+1;
	}
	
	file_name[index] = '.';
	index = index+1;
	file_name[index] = 'w';
	index = index+1;
	file_name[index] = 'a';
	index = index+1;
	file_name[index] = 'v';
	index = index+1;
	file_name[index] = '\0';
	
}
/* 
 * Function Name: read_current_file
 * Input: char key which has been touched
 * Output: None
 * Logic: This function will call concatenate function and then the read the respective file from sd card and then play
          this file in speaker.
 * Example Call: read_current_file(current_key);
 */
void read_current_file(char key)
{
	unsigned long i=0,pos=0,j=0,cur=0x00, no_of_read=0x00;
	uint16_t temp_sample=0;
	
	if (key == '1')
	{
		cli();                              // clear global interrupt as task is almost done
		uart_tx_string("$");                // send end condition to python    
		uart_tx_string("\n");
		if (get_file_info(READ,"end.wav"))
		{
			//lcd_clear();
			//lcd_string(1,1,"File Info Check");
			//_delay_ms(500);
		}
		else
		{
			//lcd_clear();
			//lcd_string(2,1,"File info ERROR");
			while(1);
		}
	}
	else
	{
		if(instrument == 0x01)
		{
			uart_tx_string("Piano");
			uart_tx_string("\n");
		}
		else
		{
			uart_tx_string("Trumpet");
			uart_tx_string("\n");
		}
		concatenate_filename_send_to_python(key);
		onset_counter = onset_counter+1;
		
		if (get_file_info(READ,file_name))
		{
			/*
			lcd_clear();
			lcd_string(1,1,"File Info Check");
			_delay_ms(500);*/
		}
		else
		{
			//lcd_clear();
			//lcd_string(2,1,"File info ERROR");
			while(1);
		}
	}
			
	
	curr_cluster=first_cluster;
	
	SPCR = (1 << SPE) | (1 << MSTR)| (1 << SPR0);            // increase the speed of SPI to ensure fast response
	
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ALGORITHM TO RETREVE AND PLAY WAV FILE ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	
	--> read the first 512 byte
	--> extract header information
	--> read next  1024 byte , convert 16 bit audio to 8 bit samples and store 512 samples in file_audio variable
	--> start playing at this point
	--> wait till 256 samples is played , read again and replace the already played samples ' continue this step in loop '.
	--> when file_audio playing index reach maximun, make it zero so that it can play new updated sample.
	--> do this till all file size is played or any new key is pressed.
	
	FINALLY IT WILL BECOME A RING VARIABLE IN WHICH BOTH DATA PLAYING AND DATA REPLACEMENT IS DONE SUCH THAT THERE IS NO DISTURBANCE IN 
	PLAYING WAVE FILE
	
	~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	
	do
	{
		first_cluster=curr_cluster;
		j=0;
		curr_pos=0;
		do
		{
			if (pos == 0x00)
			{
				read_file();
				pos=512;
				j=512;
				extract_headder();
			}
			
			read_file();
			
			while ( (play < (no_of_read + 256))  && (pos > 1536) );
			
			for (i=0;i<512;i=i+2)                                          // In this loop we are converting the original 16 bit .wav data to to 8 bit unsigned sample value
			{
				temp_sample = (buffer[i]) | (buffer[i+1]<<8);
				
				if ((temp_sample & 0b1000000000000000) == 0x0000 )
				{
					temp_sample = (temp_sample>>7);
					file_audio[cur] = (uint8_t)temp_sample;
				}
				else
				{
					temp_sample = (uint16_t)((~(temp_sample))+1);
					temp_sample = (temp_sample>>7);
					file_audio[cur] = (uint8_t)temp_sample;
				}
				pos = pos +2;
				cur++;
				
				if(pos >= (file_size-512) )
				{
					pos = file_size;
					break;
				}
			}
			j=j+512;
			no_of_read = no_of_read+256;
			if ( cur >= 512 )
			{
				cur=0x00;
				if (pos <= 1536)
				{
					k=0;
					no_of_read = 0x00;
					play =0x00;
					sei();                         // strt the interrupt, pwm, sample timer to play the file.
					timer2_init();
					timer1_pwm_init();
				}
			}
			
		} while (pos < file_size && j <(bytes_per_sector * sectors_per_cluster));
		
		if(pos<file_size)
		{
			curr_cluster=get_set_next_cluster(GET,first_cluster,0);
			
		}
	} while (pos<file_size);
	
	TIMSK2=0x00;
	TCCR1B = 0x00;
	
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);      // decrease the SPI speed when audio playing is done
	
	current_key='0';
	
}

/* 
 * ISR Name: External interrupt 4
 * Logic: This routine will sense the IRQ pin from the mpr121, so that whenever their is touch
          we can read the status resister of mpr121 and get the touch status of mpr121.
 */

ISR (INT4_vect)
{
	get_raw_values();
	
	
}

/* 
 * ISR Name: Timer2 compare match A 
 * Logic: This routine will give the values of file_audio to pwm of timer 0 to play the speaker as per the sample rate
 */
ISR(TIMER2_COMPA_vect)
{
	OCR1AL = file_audio[k];
	k=k+1;
	play = play+1;
	if (k >= 512)
	{
		k=0;
	}

}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ MAIN FUNCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int main(void)
{
	boot_switch_pin_config();
	initialise_device();
	uart0_init();
	DDRB|=0b00100000;                          // port config speaker.
	while(switch_flag == 0x00)
	{
		// if boot switch is pressed
		if ((PIND & 0x40) != 0x40)
		{
			switch_flag = 1;
			uart_tx_string("#");
			uart_tx_string("\n");
		}
	}
	
	IRQ_interrupt_init();                      // enable interrupt for mpr121.
    while(1);
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/