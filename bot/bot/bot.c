/*
 * Team Id:     278
 * Author List: Raj kumar Bhagat
 * Filename:    Bot.c
 * Theme:       Mocking Bot
 * Functions:   boot_switch_pin_config, uart0_init, arrange_data,port_init,timer1_init,
                 timer3_init, timer4_init, timer0_init, servo_n, servo _n_free, play_piano,
			     initialise_device.


 DESCRIPTION : 
                 This program is for the final task, Mocking Bot. This program
			     has two part one before the boot button is pressed. Other is the
			     execution part after the boot button is pressed.
			   
 OVERALL FOLW OF THE PROGRAM :
                 
                 Before boot button :
			   
                 As we will power the board it will initialize the UART, servo,
			     set servos at initial position. It will then receive all the
			     data from python and arrange them in required array.
			   
			     After boot button :
			   
			     After the boot button is pressed it will strike the required key
			     of Piano and Trumpet at the stored onset time. After task is completed it 
			     will free all the servo.


/*******************************************************************************************************************
 
 Connection Details:  
 
            PORTB 5 OC1A --> Servo 1:
            PORTB 6 OC1B --> Servo 2:
            PORTB 7 OC1C --> Servo 3: 
            PORTE 3 OC3A --> servo 4:
            PORTE 5 OC3C --> servo 5:
            PORTH 3 OC4A --> servo 6:
            PORTH 4 OC4B --> servo 7:
            PORTH 5 OC4C --> servo 8:
			PORTL 3 OC5A --> servo 9:
			PORTL 4 OC5B --> servo 10:
			PORTL 5 OC5C --> servo 11:
              
*********************************************************************************************************************/
#define F_CPU 14745600
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>



/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ GLOBAL VARIABLES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
unsigned char switch_flag=0x00;
unsigned char data, usart_flag=0x00, number_of_onset=0x00, column=0, row=0x00, decimal_flag=0,onset_temp1,onset_temp2; 
unsigned char instrument[20];
char notes[20][3];
float onset[20];
volatile float time_sec= 0.00;
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/





/* 
 * Function Name: boot_switch_pin_config()
 * Input: None
 * Output: None
 * Logic: This function will initialize internal pull-ups in PD6 pin.
 */
void boot_switch_pin_config()
{
	DDRD  = DDRD & 0xBF;		// set PD.6 as input
	PORTD = PORTD | 0x40;		// set PD.6 HIGH to enable the internal pull-up
}

/* 
 * Function Name: uart0_init
 * Input: None
 * Output: None
 * Logic: To Initialize UART0, baud rate:9600, char size: 8 bit,  parity: Disabled
 */

void uart0_init(void)
{
	UCSR0B = 0x00; //disable while setting baud rate
	UCSR0A = 0x00;
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);// same as 0x06
	UBRR0L = 0x5F; //set baud rate lo
	UBRR0H = 0x00; //set baud rate hi
	UCSR0B |= (1 << RXEN0) | (1 << TXEN0) | (1<<RXCIE0);
}

/* 
 * Function Name: arrange_data
 * Input: uart received char data
 * Output: None
 * Logic: This function will arrange all the received data in the required variable / array format
             number of onset --> number_of_onset
             instruments     --> instrument[]
             onsets          --> onset[]
             notes           --> notes[][]
 
*/
void arrange_data(unsigned char uart_data)
{
	if (usart_flag == 0x00)                   // if usart_flag is 0 then received data is number of notes.
	{
		number_of_onset = uart_data;
	}
	else if (usart_flag == 0x01)              // if usart_flag is 1 then received data is for instrument -> 0x01 implies piano, ->0x00 implies trumpet.
	{
		instrument[column] = uart_data;
		column = column+1;
	}
	else if (usart_flag == 0x02)              // if usart_flag is 2 then received data is for onset time.
	{
		if (decimal_flag == 0x00)             // first receive the value after the decimal of onset.
		{
			onset_temp1 = uart_data;
			decimal_flag = 0x01;
		}
		else if (decimal_flag == 0x01)        // Then receive the value of integer part of onset. And store the complete float onset time in the onset array.
		{
			onset_temp2 = uart_data;
			decimal_flag = 0x00;
			onset[column] = ((onset_temp2*100)+onset_temp1);
			onset[column] = onset[column]/100;
			column = column+1;
		}
	}
	else if (usart_flag == 0x03)              // if usart_flag is 3 then received data are notes.
	{
		if (uart_data == 0xFD)                // 0xFD is to separate between to notes.
		{
			column = 0x00;
			row = row+1;
		}
		else
		{
			notes[row][column] = uart_data;
			column = column+1;
		}
	}
	else if (usart_flag >= 0x04)               // if usart_flag is 4 then all receiving is completed.
	{
		 // receiving complete do noting.
	}
}


/* 
 * Function Name: timer0_init
 * Input: None
 * Output: None
 * Logic: This function will initialize timer 0, which will act as the clock to detect the onset time.
 */
void timer0_init()
{
  TCCR0A=(1<<WGM01);                     //Timer 0 CTC Mode 
  TIMSK0=(1<<OCIE0A);                    //Timer0 Interrupt on Compare Match A       
  OCR0A=144;                             // tick every 0.01 second
  TCCR0B=(1<<CS00)|(1<<CS02);
}
/* 
 * Function Name: port_init
 * Input: None
 * Output: None
 * Logic: This function will configure the port so that we can connect control pin of the
          servo motor to the PWM pin of the micro controller. 
 * Example Call: port_init();
 */
void port_init(void)
{
	DDRB  = DDRB | 0b11100000;             // Pin configuration servo 1,2,3.
	PORTB = PORTB | 0b11100000;
	
	
	DDRE  = DDRE | 0b00101000;             // Pin configuration servo 4,5.
	PORTE = PORTE | 0b00101000;
	
	
	DDRH  = DDRH | 0b00111000;             // Pin configuration servo 6,7,8.
	PORTH = PORTH | 0b00111000;
	
	
	DDRL  = DDRL | 0b00111000;             // Pin configuration servo 9,10,11.
	PORTL = PORTL | 0b00111000;
}
/* 
 * Function Name: timer1_init
 * Input: None
 * Output: None
 * Logic: This function will initialize timer 1 PWM pin for the servo control. 
 * Example Call: timer1_init();
 */  
// prescaler :256
void timer1_init(void)
{
 TCCR1B = 0x00; 
 TCNT1H = 0xFC; 
 TCNT1L = 0x01; 
 OCR1AH = 0x03; 
 OCR1AL = 0xFF; 
 OCR1BH = 0x03; 
 OCR1BL = 0xFF; 
 OCR1CH = 0x03; 
 OCR1CL = 0xFF; 
 ICR1H  = 0x03; 
 ICR1L  = 0xFF;
 TCCR1A = 0xAB;
 TCCR1C = 0x00;
 TCCR1B = 0x0C; 
}


/* 
 * Function Name: timer3_init
 * Input: None
 * Output: None
 * Logic: This function will initialize timer 3 PWM pin for the servo control. 
 * Example Call: timer3_init();
 */  
// prescaler :256
void timer3_init(void)
{
 TCCR3B = 0x00;
 TCNT3H = 0xFC;
 TCNT3L = 0x01;
 OCR3AH = 0x03;
 OCR3AL = 0xFF;
 OCR3CH = 0x03;
 OCR3CL = 0xFF; 
 ICR3H  = 0x03; 
 ICR3L  = 0xFF;
 TCCR3A = 0b10001011;
 TCCR3C = 0x00;
 TCCR3B = 0x0C;
}

/* 
 * Function Name: timer4_init
 * Input: None
 * Output: None
 * Logic: This function will initialize timer 4 PWM pin for the servo control. 
 * Example Call: timer4_init();
 */  
// prescaler :256
void timer4_init(void)
{
 TCCR4B = 0x00; 
 TCNT4H = 0xFC; 
 TCNT4L = 0x01;
 OCR4AH = 0x03; 
 OCR4AL = 0xFF; 
 OCR4BH = 0x03; 
 OCR4BL = 0xFF; 
 OCR4CH = 0x03; 
 OCR4CL = 0xFF; 
 ICR4H  = 0x03; 
 ICR4L  = 0xFF;
 TCCR4A = 0xAB; 
 TCCR4C = 0x00;
 TCCR4B = 0x0C; 
}
/* 
 * Function Name: timer5_init
 * Input: None
 * Output: None
 * Logic: This function will initialize timer 5 PWM pin for the servo control. 
 * Example Call: timer5_init();
 */  
// prescaler :256
void timer5_init(void)
{
 TCCR5B = 0x00;
 TCNT5H = 0xFC; 
 TCNT5L = 0x01;
 OCR5AH = 0x03; 
 OCR5AL = 0xFF; 
 OCR5BH = 0x03; 
 OCR5BL = 0xFF; 
 OCR5CH = 0x03; 
 OCR5CL = 0xFF; 
 ICR5H  = 0x03; 
 ICR5L  = 0xFF;
 TCCR5A = 0xAB; 
 TCCR5C = 0x00;
 TCCR5B = 0x0C; 
}
/* 
 * Function Name: servo_n
 * Input: angle in degree
 * Output: None
 * Logic: This function will rotate the servo in the given angle 
 * Example Call: servo_3(30);
 */  
void servo_1(unsigned char degrees)  
{
 float PositionPanServo = 0;
 PositionPanServo = ((float)degrees / 1.86) + 35.0;
 OCR1AH = 0x00;
 OCR1AL = (unsigned char) PositionPanServo;
}
void servo_2(unsigned char degrees)
{
 float PositionTiltServo = 0;
 PositionTiltServo = ((float)degrees / 1.86) + 35.0;
 OCR1BH = 0x00;
 OCR1BL = (unsigned char) PositionTiltServo;
}
void servo_3(unsigned char degrees)
{
 float PositionServo = 0;
 PositionServo = ((float)degrees / 1.86) + 35.0;
 OCR1CH = 0x00;
 OCR1CL = (unsigned char) PositionServo;
}
void servo_4(unsigned char degrees)
{
  float PositionPanServo = 0;
  PositionPanServo = ((float)degrees / 1.86) + 35.0;
  OCR3AH = 0x00;
  OCR3AL = (unsigned char) PositionPanServo;
}
void servo_5(unsigned char degrees)
{
  float PositionPanServo = 0;
  PositionPanServo = ((float)degrees / 1.86) + 35.0;
  OCR3CH = 0x00;
  OCR3CL = (unsigned char) PositionPanServo;
}
void servo_6(unsigned char degrees)
{
  float PositionPanServo = 0;
  PositionPanServo = ((float)degrees / 1.86) + 35.0;
  OCR4AH = 0x00;
  OCR4AL = (unsigned char) PositionPanServo;
}
void servo_7(unsigned char degrees)
{
  float PositionPanServo = 0;
  PositionPanServo = ((float)degrees / 1.86) + 35.0;
  OCR4BH = 0x00;
  OCR4BL = (unsigned char) PositionPanServo;
}
void servo_8(unsigned char degrees)
{
  float PositionPanServo = 0;
  PositionPanServo = ((float)degrees / 1.86) + 35.0;
  OCR4CH = 0x00;
  OCR4CL = (unsigned char) PositionPanServo;
}
void servo_9(unsigned char degrees)
{
	float PositionPanServo = 0;
	PositionPanServo = ((float)degrees / 1.86) + 35.0;
	OCR5AH = 0x00;
	OCR5AL = (unsigned char) PositionPanServo;
}
void servo_10(unsigned char degrees)
{
	float PositionPanServo = 0;
	PositionPanServo = ((float)degrees / 1.86) + 35.0;
	OCR5BH = 0x00;
	OCR5BL = (unsigned char) PositionPanServo;
}
void servo_11(unsigned char degrees)
{
	float PositionPanServo = 0;
	PositionPanServo = ((float)degrees / 1.86) + 35.0;
	OCR5CH = 0x00;
	OCR5CL = (unsigned char) PositionPanServo;
}
/* 
 * Function Name: servo_n_free
 * Input: None
 * Output: None
 * Logic: This function will make PWN pin to 100% duty cycle, to make the servo off, hence consume low power. 
 * Example Call: servo_2_free();
 */  
void servo_1_free (void) //makes servo 1 free rotating
{
 OCR1AH = 0x03; 
 OCR1AL = 0xFF; //Servo 1 off
}

void servo_2_free (void) //makes servo 2 free rotating
{
 OCR1BH = 0x03;
 OCR1BL = 0xFF; //Servo 2 off
}

void servo_3_free (void) //makes servo 3 free rotating
{
 OCR1CH = 0x03;
 OCR1CL = 0xFF; //Servo 3 off
} 

void servo_4_free (void) //makes servo 4 free rotating
{
  OCR3AH = 0x03;
  OCR3AL = 0xFF; //Servo 4 off
}

void servo_5_free (void) //makes servo 5 free rotating
{
  OCR3CH = 0x03;
  OCR3CL = 0xFF; //Servo 5 off
}

void servo_6_free (void) //makes servo 6 free rotating
{
  OCR4AH = 0x03;
  OCR4AL = 0xFF; //Servo 6 off
}

void servo_7_free (void) //makes servo 7 free rotating
{
  OCR4BH = 0x03;
  OCR4BL = 0xFF; //Servo 7 off
}
void servo_8_free (void) //makes servo 8 free rotating
{
  OCR4CH = 0x03;
  OCR4CL = 0xFF; //Servo 8 off
}
void servo_9_free (void) //makes servo 9 free rotating
{
	OCR5AH = 0x03;
	OCR5AL = 0xFF; //Servo 9 off
}

void servo_10_free (void) //makes servo 10 free rotating
{
	OCR5BH = 0x03;
	OCR5BL = 0xFF; //Servo 10 off
}
void servo_11_free (void) //makes servo 11 free rotating
{
	OCR5CH = 0x03;
	OCR5CL = 0xFF; //Servo 11 off
}
/* 
 * Function Name: play_servo
 * Input: note to be played
 * Output: None
 * Logic: This function will make the servo strike the electrode of mpr121 on onset.
 * Example Call: play_servo('A');
 */
void play_servo(char key)
{
  // following are the condition for piano.
  
  if(key=='C'){
	  servo_2(135);
	  _delay_ms(300);
	  servo_2(155);
  }
  else
  if(key=='D'){
	  servo_3(97);
	  _delay_ms(300);
	  servo_3(118);
  }
  else
  if(key=='E'){
	  servo_4(113);
	  _delay_ms(300);
	  servo_4(133);
  }
  else
  if(key=='F'){
	  servo_5(100);
	  _delay_ms(300);
	  servo_5(120);
  }
  else
  if(key=='G'){
	  servo_6(140);
	  _delay_ms(300);
	  servo_6(160);
  }
  else
  if(key=='A'){
	  servo_7(87);
	  _delay_ms(300);
	  servo_7(107);
  }
  else
  if(key=='B'){
	  servo_8(120);
	  _delay_ms(300);
	  servo_8(140);
  }
  else
  if(key=='1'){
	  servo_1(105);
	  _delay_ms(300);
	  servo_1(125);
	  _delay_ms(500);
  }
  
  
  // following are the condition for trumpet.
  
  else
  if(key=='c'){
	  servo_9(75);
	  _delay_ms(300);
	  servo_9(115);
  }
  else
  if(key=='d'){
	  servo_10(175);
	  _delay_ms(300);
	  servo_10(155);
  }
  else
  if(key=='e'){
	  servo_11(55);
	  _delay_ms(300);
	  servo_11(35);
  }
  else
  if(key=='f'){
	  servo_9(75);
	  servo_10(175);
	  _delay_ms(300);
	  servo_9(115);
	  servo_10(155);
  }
  else
  if(key=='g'){
	  servo_10(175);
	  servo_11(55);
	  _delay_ms(300);
	  servo_10(155);
	  servo_11(35);
  }
  else
  if(key=='a'){
	  servo_9(75);
	  servo_11(55);
	  _delay_ms(300);
	  servo_9(115);
	  servo_11(35);
  }
  else
  if(key=='b'){
	  servo_9(75);
	  servo_10(175);
	  servo_11(55);
	  _delay_ms(300);
	  servo_9(115);
	  servo_10(155);
	  servo_11(35);
  }
}
/* 
 * Function Name: initialise_device()
 * Input: None
 * Output: None
 * Logic: This function will initialize the whole device. which includes timers, pin and initial position of servo.
 */
void initialise_device(void)
{
	  port_init();
	  timer1_init();
	  timer3_init();
	  timer4_init();
	  timer5_init();


	  
	  // start with all the servo in un-hold position (not touching) to electrode of mpr121.
	  
	  servo_1(125);
	  _delay_ms(200);
	  servo_2(155);
	  _delay_ms(200);
	  servo_3(118);
	  _delay_ms(200);
	  servo_4(133);
	  _delay_ms(200);
	  servo_5(120);
	  _delay_ms(200);
	  servo_6(160);
	  _delay_ms(200);
	  servo_7(107);
	  _delay_ms(200);
	  servo_8(140);
	  _delay_ms(200);
	  servo_9(115);
	  _delay_ms(200);
	  servo_10(155);
	  _delay_ms(200);
	  servo_11(35);
	  _delay_ms(200);
	  
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ISR ROUTINES ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* 
 * ISR Name: Timer0 compare match A 
 * Logic: This routine will increment the time variable time_sec.
 */
ISR(TIMER0_COMPA_vect) {
  time_sec=time_sec+0.01;

}
/* 
 * ISR Name: UART0 receive complete 
 * Logic: This routine receive the data from python and call the arrange_data function which will arrange the received data.
 */
ISR(USART0_RX_vect)
{
	data = UDR0; 				//making copy of data from UDR2 in 'data' variable

	//UDR0 = data; 				//echo data back to PC
	if(data == 0xFE)            // whenever 0xfe is received increment the usart_flag variable which is being used for arranging the received data.
	{
		usart_flag = usart_flag+1;
		column = 0;
	}
	else
	{
		arrange_data(data);
	}
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/




/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ MAIN FUNCTION ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
int main(void)
{
	unsigned char i = 0;
	boot_switch_pin_config();
	cli();
	uart0_init();
	initialise_device();
	sei();
	while(switch_flag == 0x00)
	{
		// if boot switch is pressed
		if ((PIND & 0x40) != 0x40)
		{
			UCSR0B = 0x00;   // disable UART
			switch_flag = 1;
		}
	}
	cli();
	sei();
	timer0_init();          // enable clock timer0 just after the boot button is pressed.


 while(1)
 {
  
   if((time_sec > (onset[i]-0.01 )) && (time_sec < (onset[i]+0.01)) && i < number_of_onset )
   {
	   if (instrument[i] == 0x01)             // send uppercase for piano
	   {
		   play_servo(notes[i][0]);
	   } 
	   else                                   // send lowercase for trumpet i.e. add 32 to ASCII value of capital character.
	   {
		   play_servo(notes[i][0] + 32);
	   }
     i=i+1;
     }
   else
   if(i==number_of_onset)   // If the task is completed free all the servo.
   {


    _delay_ms(1000);


    
     servo_1_free();
     servo_2_free();
     servo_3_free();
     servo_4_free();
     servo_5_free();
     servo_6_free();
     servo_7_free();
     servo_8_free();
	 servo_9_free();
	 servo_10_free();
	 servo_11_free();
	 _delay_ms(500);
     i=i+1;
     
   }
 
 }
 return 0;
}
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~END~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/