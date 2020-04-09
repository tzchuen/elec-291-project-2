#define F_CPU 16000000UL
#define BAUD 19200
#define MYUBRR F_CPU/16/BAUD-1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include "usart.h"

#define F_CPU 16000000UL
#define BAUD 19200
#define MYUBRR F_CPU/16/BAUD-1

#define NULL_CHAR '\0'

#define NUM_LEDS 3

#define SPEAKER_LOW	500
#define SPEAKER_MED	200
#define SPEAKER_FAST 100
// PuTTY
#define ANSI_CLEAR_SCREEN "\x1b[2J"
#define ANSI_CURSOR_CLEAR_LINE "\x1b[0K"

volatile uint16_t period;

void wait_1ms(void)
{
	unsigned int saved_TCNT1;
	
	saved_TCNT1=TCNT1;
	
	while((TCNT1-saved_TCNT1)<(F_CPU/1000L)); // Wait for 1 ms to pass
}

void waitms(int ms)
{
	while(ms--) 
		wait_1ms();
}

// Input Capture Unit Interrupt at ICP1 (Pin 14)
ISR (TIMER1_CAPT_vect) 
{
	while ((TIFR1&1<<ICF1) == 0);	// Wait for capture
	period = ICR1;					// Get timestamp
	TIFR1 = 1<<ICF1;				// Clear flag
	while ((TIFR1&1<<ICF1) == 0);	// Wait for second capture
	period = ICR1 - period;			// Period = time between 2 timestamps
	TIFR1 = 1<<ICF1;				// Clear flag
}

/* DDxn bit in the DDRx Register selects the direction of this pin. If DDxn is written logic one,
 * Pxn is configured as an output pin. If DDxn is written logic zero, Pxn is configured as an input
 * pin.
 * 
 * If PORTxn is written logic one when the pin is configured as an input pin, the pull-up resistor is
 * activated.
 * 
 * If PORTxn is written logic one when the pin is configured as an output pin, the port pin is driven
 * high (one).
 */

/***********************************************************
 * PORTX registers (X=B,C,D) to assign output states	   *
 * ______________________________________________          *
 * Bit	| 7  | 6  | 5  | 4  | 3  | 2  | 1  | 0  |          *
 * Pin  | X7 | X6 | X5 | X4 | X3 | X2 | X1 | X0 |          *
 * ----------------------------------------------          *
 *													       *
 * Note: Can't directly access registers; need to |= or &= *
 * to set bits											   *
 ***********************************************************/

/******************************************************
 * TCCR1B for Input Capture                           *
 * ___________________________________________        *
 * Bit	|  7  |  6  | 5 | 4 | 3  | 2 | 1 | 0 |        *
 * Pin  |ICNC1|ICES1| - |Waveform| CLK select|        *
 * -------------------------------------------        *
 *													  *
 * ICNC1 = Noise canceler                             *
 * ICES1 = Edge select (pos/negedge)                  *
 * 								                      *
 ******************************************************/

/*************************
 *      CLK select	     *
 * _____________________ *
 * | 0x00 | Stop       | *
 * | 0x01 | clk/1      | *
 * | 0x02 | clk/8      | *
 * | 0x03 | clk/64     | *
 * | 0x04 | clk/256    | *
 * | 0x05 | clk/1024   | *
 * | 0x06 | T1 negedge | *
 * | 0x07 | T1 posedge | *
 * --------------------- *
 *************************/

/******************************************************
 * TIFR1 for Timer1 Interrupt Flags                   *
 * ___________________________________________        *
 * Bit	| 7 | 6 |  5   | 4 | 3 | 2 | 1 |  0  |        *
 * Pin  | - | - | ICF1 |   -   |Compare|TOV1 |        *
 * -------------------------------------------        *
 *													  *
 * ICF1 = Input capture flag                          *
 * TOV1 = Timer1 ovf flag                             *
 * 								                      *
 ******************************************************/

int main(void)
{	
	char key_check [2];
	double lower_period;
	double higher_period;
	double frequency;
	double threshold;
	
	waitms(500);	// Give PuTTY a chance to start
	usart_init();
	
	printf(ANSI_CLEAR_SCREEN);
	printf("Metal Detector System v1.0\n"
	"ELEC 291 - Project 2\n"
	"Author: Zhi Chuen Tan (65408361)\n"
	"Term: 2019W2\n"
	"Compiled: %s, %s\n\n",
	__DATE__, __TIME__);

	/* Outputs:
	 * Red	   C4
	 * Yellow1 C3
	 * Yellow2 C2
	 * Speaker D7
	 * 
	 * Inputs:
	 * Mute		D2 (INT0)
	 * Lights	D3 (INT1)
	 */
	// Pin/Port configuration
	DDRC |= 0x7c;	// Pin C2-4 are outputs
	DDRD |= 0xf0;	// Pin D2-3 are inputs
	PORTD |= 0x0c;	// pull-up resistors

	// External interrupts initializations
	EICRA |= 0x00;	// interrups triggered at low logic level
	EIMSK |= 0x02;	// activate external interrupts
	EIFR  &= 0x00;	// clear interrupt flag

	// ICU Timer Initializations
	TCCR1A &= 0x00;	// not using compare output modes
	TCCR1B |= 0xc1; // no noise canceler; capture at rising edge; no prescaling
	TIFR1 &= ~(0x20); // clear input capture flag
	
	printf("******SENSOR CALIBRATION******\n"
		   "Please ensure sensor is clear of all foreign metal.\n"
		   "Press '1' to continue...\n");
		   
	key_check[0] = usart_getchar();
	while(key_check[0] != '1');	// do nothing
	
	lower_period = period; 
	key_check[0] = 'x';
	PORTC |= 0x04;
	
	printf("\n\rPlease place some metal directly on the top of the sensor\n"
		   "Press '1' to continue...\n"
		   "******SENSOR CALIBRATION COMPLETE******\n");
	
	key_check[0] = usart_getchar();
	while(key_check[0] != '1'); // do nothing
	
	higher_period = period;
	PORTC |= 0x0c;
	
	// turn off noise canceling
	TCCR1B &= 0x41; // no noise canceler; capture at rising edge; no prescaling
		
	printf("Initialization complete!\n");
	
	PORTC &= ~(0x0c);
	
	PORTC |= 0x04;	// Turn on Yellow1
	waitms(200);
	PORTC &= ~(0x04); // Turn off Yellow1
	
	PORTC |= 0x08;	// Turn on Yellow2
	waitms(200);
	PORTC &= (~0x08);	// Turn off Yellow2
	
	PORTC |= 0x10;	// Turn on Red
	waitms(200);
	PORTC &= ~(0x10); // Turn off Red
	
	PORTD |= 0x8;	// Turn on speaker
	waitms(200);
	PORTD &= ~(0x80); // Turn off speaker
	
	printf(ANSI_CURSOR_CLEAR_LINE);
	
	while (1) 
    {
		threshold = (higher_period - lower_period)/NUM_LEDS;

		if (period <= lower_period) {
			PORTC &= ~(0x1c);	// turn off all lights
			PORTD &= ~(0x80);	// turn off speaker

			printf(ANSI_CURSOR_CLEAR_LINE);
			printf("\rNo metal detected...");
		}
		
		else if (period >= lower_period && period < (lower_period + threshold))
		{
			PORTC |= 0x04;	// turn on yellow1

			PORTD |= 0x80;
			waitms(SPEAKER_LOW);
			PORTD &= ~(0x80);

			printf(ANSI_CURSOR_CLEAR_LINE);
			printf("\rSome metal detected...");
		}

		else if (period >= (lower_period + threshold) && period < (lower_period + (2*threshold)) )
		{
			PORTC |= 0x0c;	// turn on yellow1 and yellow 2

			PORTD |= 0x80;
			waitms(SPEAKER_MED);
			PORTD &= ~(0x80);

			printf(ANSI_CURSOR_CLEAR_LINE);
			printf("\rGetting closer to metal...");
		}

		else if (period >= (lower_period + (2*threshold)) && period < higher_period)
		{
			PORTC |= 0x1c;	// turn on yellow1 and yellow 2 and red				

			PORTD |= 0x80;
			waitms(SPEAKER_FAST);
			PORTD &= ~(0x80);

			printf(ANSI_CURSOR_CLEAR_LINE);
			printf("\rAlmost at metal...");
		}

		else
		{
			PORTD |= 0x80;

			PORTC |= 0x1c;
			waitms(250);
			PORTC &= ~(0x1c);

			printf(ANSI_CURSOR_CLEAR_LINE);
			printf("\rMetal found!");
		}
    }
}	

