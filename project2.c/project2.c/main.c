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

//// Signal strength LEDs
//#define YELLOW_LED_1	PORTD4
//#define YELLOW_LED_2	PORTD3
//#define RED_LED_METER   PORTD2
//
//// Push buttons
//#define BUTTON_LEFT_MUTE			PINC3
//#define BUTTON_MID_LED				PINC4
//#define BUTTON_RIGHT_LED_STRENGTH	PINC5
//
//// LED/Speaker 
//#define DETECTOR_LED	PORTB2
//#define SPEAKER			PORTB1
//
//
//// Detector input
//#define METAL_DETECT_IN	PIND4

// PuTTY
#define ANSI_CLEAR_SCREEN "\x1b[2J"
#define ANSI_CURSOR_END_LINE "\x1b[0K"

//#define LOGIC_HIGH	1
//#define LOGIC_LOW	0

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

unsigned int ReadChannel(unsigned char mux)
{
	ADCSRA = (1<<ADEN) | (1<<ADPS1) | (1<<ADPS0); // frequency prescaler
	ADMUX = mux; // channel select
	ADMUX |= (1<<REFS1) | (1<<REFS0);
	ADCSRA |= (1<<ADSC); // Start conversion
	while ( ADCSRA & (1<<ADSC) ) ;
	ADCSRA |= (1<<ADSC); // a transformation ?single conversion?
	while ( ADCSRA & (1<<ADSC) );
	ADCSRA &= ~(1<<ADEN); // Disable ADC
	return ADCW;
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
	// char noise_cancel_choice;
	
	double frequency;
	
	waitms(500);	// Give PuTTY a chance to start
	
	usart_init();	
	
	printf(ANSI_CLEAR_SCREEN);
	printf("Metal Detector System v1.0\n"
	"ELEC 291 - Project 2\n"
	"Author: Zhi Chuen Tan (65408361)\n"
	"Term: 2019W2\n"
	"Compiled: %s, %s\n\n",
	__DATE__, __TIME__);
	
	//printf("This system uses a TIMER1 Input Capture Unit for frequency measurements.\n"
		   //"Please indicate if you would like to activate the Input Capture Noise Canceler.\n"
		   //"NOTE: Activating this function will result in the Input Capture to be delayed by four oscillator cycles\n"
		   //"1 - Yes\n"
		   //"2 - No\n\n");
	
	// noise_cancel_choice = usart_getchar();
	
	// ICU Timer Initializations
	TCCR1A &= 0x00;	// not using compare output modes
	TCCR1B |= 0x41; // no noise canceler; capture at rising edge; no prescaling
	TIFR1 &= ~(0x20); // clear input capture flag
	
	// Pin/Port configuration
	DDRB  |= 0x07; // Set B0, 1, 2 (outputs, Pin 14-16)
	DDRC  |= 0x00; // All C pins are inputs
	PORTC |= 0x38; // Set pull-ups for push buttons
	DDRD  |= 0x1c; // Set D2, D3, D4 (outputs, Pin 4-6)
	
	/* Outputs:
	 * YellowL D4
	 * YellowM D3
	 * Red	   D2
	 * Green   B2
	 * Speaker B1
	 */

	// Output LED/speaker sanity check
	PORTD |= 0x1c;	// turn on small LEDs
	PORTB |= 0x06;	// turn on speaker and green LED
	waitms(200);
	PORTD &= ~(0x1c);	// turn off small LEDs
	PORTB &= ~(0x06);	// turn off speaker and green LED
	
	printf("Initialization complete!\n");
	printf(ANSI_CURSOR_END_LINE);
	sei();	// enable interrupts
    
	while (1) 
    {
		frequency = (double) F_CPU / period;
		printf("\r F= %fHz", frequency);
    }
}	

