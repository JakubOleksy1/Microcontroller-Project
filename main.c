/*
 * Final Project.c
 *
 * Created: 24.10.2022 11:40:58 
 * Author : Jakub Oleksy 1220420  David Tertre 1220515
 */ 

#include <avr/io.h>																					// This header file includes the appropriate Input/output definitions for the device
#include <avr/interrupt.h>																			// Header file to use interrupts (for Timer1 and Timer2)
#include <util/delay.h>																				// Library to use delay functions
#include <stdlib.h>																					// We'll be using itoa() function to convert integer to character array that is in this library
#include "lcd.h"																					// Header file made by me (at first there was functions before "int main()" )

int main()																							// Main function
{
	DDRB = (1 << PINB5) | (1 << PINB4) | (1 << PINB3) | (1 << PINB2) | (1 << PINB1) | (1 << PINB0);	// Set PINB: 0 - 5 as an output (0 2 3 4 for LED) (1 for LED 1 Hz signal) (5 for buzzer) 
	
	TIMSK1 |= (1 << OCIE1A);																		// Timer1 interrupt mask register set to Output compare A match
	TCCR1A &= (~(1 << WGM10)) & (~(1 << WGM11));													// Waveform generation MODE 0 (normal) timer counter control register
	TCCR1B &= (~(1 << WGM12)) & (~(1 << WGM13));													// Look line above
	TCCR1A |= (1 << COM1A0);																		// Clear OC1A/OC1B on compare match (output low level).
	TCCR1A &= (~(1 << COM1A1));																		// Look line above
	TCCR1B |= (1 << CS10) | (1 << CS11);															// Set prescaler to 64 (CS10 and CS11 set to 1 and CS12 set to 0)
	TCCR1B &= (~(1 << CS12));														   				// Look lime above
	TCNT1 = 0;																						// Measure time between ping and echo start from 0 64us per tick
	OCR1A = 15625;																	   				// 1000000/64 = 15625 this is the number to type in order to get 1Hz
	
	TIMSK2 |= (1 << OCIE2A);																		// Timer2 interrupt mask register set to Output compare A match
	TCCR2A &= (~(1 << WGM20)) & (~(1 << WGM22));													// Set waveform generation to normal
	TCCR2A |= (1 << WGM21);																			// Look line above
	TCCR2B |= (1 << CS20) | (1 << CS21);															// Setting Timer2 prescaler to 1024 (CS20 and CS21 and CS22 set to 1)
	TCCR2B |= (1 << CS22);																			// Look line above
	TCNT2 = 0;																						// Start counting
	OCR2A = 250;																					// Interrupt when it counts to 250 (16ms)
	
	sei();																							// Enable global interrupts
	
	initialize();																					// Initialize the LCD

	char numberString[4];																			// Define an array of character. It will be utilized later to store integer to be displayed on the LCD screen

	while(1)																						// Infinite loop
	{	
		uint16_t r = 0;																				// Define r as uint16_t and set to 0 (this will read our distance in time and we will need to calcualte it)
		_delay_ms(100);																 				// Wait 100ms
																				
		HCSR04Init();																 				// Set i/o port direction of sensor
		
		while(1)																					// Infinite loop
		{						
			HCSR04Trigger();																		// Send a trigger pulse

			r = GetPulseWidth();																	// Measure the width of pulse. Duration of the ultrasound took to echo back after hitting the object
																				
			if(r == US_ERROR)																		// Handle Errors. If micro controller doesn't get any pulse then it will set the US_ERROR variable to -1 and display and error
			{
				lcd_setCursor(1, 1);																// Lcd_setCursor(column, row)
				lcd_print("Error!");																// Print and error
			}
			else                                                                                    // Else
			{
				distance = (r*300/1000);															// Calculate the distance in centimeters
				check_distance(distance);															// Check distance function with new calculated distance (diodes and buzzer)
				if (distance != previous_distance)													// Change the distance if it is not equal the previous one
				{
					lcd_clear();																	// Clear LCD screen
				}
				lcd_setCursor(1, 1);																// Set cursor to starting position
				lcd_print("Distance sensor");														// Print text "Distance sensor"
				lcd_setCursor(1, 2);																// Set the row and column to display the data
				lcd_print("Distance = ");															// Print text "Distance = "
				lcd_setCursor(12, 2);																// Set cursor to write the distance in cm ( lcd_setCursor(column, row) )
				itoa(distance, numberString, 10);													// Since distance is an integer number, we can't display it on the LCD. Convert integer into array of character
				lcd_print(numberString);															// Print the distance as an array of character
				lcd_setCursor(14, 2);																// Set the row to 1 and and column to 14 to display the text cm
				lcd_print(" cm");																	// Print text "cm"
				previous_distance = distance;														// Change previous distance to current one
				_delay_ms(30);																		// Wait 30ms
			}
		}
	}
}

ISR(TIMER1_COMPA_vect)																				// Interrupt to Timer1 by compare
{
	TCNT1 = 0;																						// Measure time between ping and echo
	OCR1A = 15625;																					// 1000000/64 = 15625 this is the number to type in order to get 1Hz
}

ISR(TIMER2_COMPA_vect)																				// Interrupt to Timer2 by compare
{
	static int timeri = 0;																			// Variable timeri will be to count loops x * 16ms = our delay
	timeri++;																						// Increase timeri by 1 
	if(timeri >= timer2flag)																		// If timeri is greater or equal timer2flag (previously set in set_Diodes() function)
	{
		timeri = 0;																					// Reset timeri 
		PORTB ^= (1 << PINB5);																		// Change PINB5 from LOW ==> HIGH or from HIGH ==> LOW (depending if current PIB5 is 1 or 0)
	}
}
