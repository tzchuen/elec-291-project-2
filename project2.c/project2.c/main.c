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

// Signal strength LEDs
#define YELLOW_LED_1	PORTD4
#define YELLOW_LED_2	PORTD3
#define RED_LED_METER   PORTD2

// Push buttons
#define BUTTON_LEFT_MUTE			PINC3
#define BUTTON_MID_LED				PINC4
#define BUTTON_RIGHT_LED_STRENGTH	PINC5

// LED/Speaker 
#define DETECTOR_LED	PORTB2
#define SPEAKER			PORTB1


// Detector input
#define METAL_DETECT_IN	PIND4

// PuTTY
#define ANSI_CLEAR_SCREEN "\x1b[2J"
#define ANSI_CURSOR_END_LINE "\x1b[0K"

#define LOGIC_HIGH	1
#define LOGIC_LOW	0

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
	while (ICF1 == 0);			// Wait for capture
	period = ICR1;				// Get timestamp
	ICF1 = 0;					// Clear flag
	while (ICF1 == 0);			// Wait for second capture
	period = ICR1 - period;		// Period = time between 2 timestamps
	ICF1 = 0;					// Clear flag
}

int main(void)
{	
	// char noise_cancel_choice;
	
	double frequency;
	
	waitms(500);
	
	//usart_init();	
	
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
	
	// Output sanity checks
	YELLOW_LED_1 = LOGIC_HIGH;
	YELLOW_LED_2 = LOGIC_HIGH;
	RED_LED_METER = LOGIC_HIGH
	DETECTOR_LED = LOGIC_HIGH;
	SPEAKER = LOGIC_HIGH;
	
	waitms(100);
	
	YELLOW_LED_1 = LOGIC_LOW;
	YELLOW_LED_2 = LOGIC_LOW;
	RED_LED_METER = LOGIC_LOW;
	DETECTOR_LED = LOGIC_LOW;
	SPEAKER = LOGIC_LOW;
	
	printf("Initialization complete!\n");
	printf(ANSI_CURSOR_END_LINE);
	sei();	// enable interrupts
	
    while (1) 
    {
		frequency = (double) F_CPU / period;
    }
}	

