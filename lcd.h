#define lcd_port PORTD																	// LCD screen connected on port D
#define lcd_data_dir  DDRD																// Define ports on input or output
#define rs PD0																			// Data pin 4, 5, 6, and 7 are for sending the data to the LCD. Enable and RS pins are for controlling
																						// Setting rs to PIND0
#define en PD1																			// Enable pin set to PIND1

#define US_PORT PORTC																	// Ultrasonic sensor on port C. We need two pins of the ultrasonic sensor to be connected on port C
#define	US_PIN	PINC																	// Initialize the pin resistor when we want to take input
#define US_DDR 	DDRC																	// Data-direction-resistor (DDR) to set the direction of data flow. This is set to input or output

#define US_TRIG_POS	PC0																	// Trigger pin of ultrasonic sensor is connected to PINC0
#define US_ECHO_POS	PC1																	// Echo pin of the ultrasonic sensor is connected to PINC1

#define US_ERROR		-1																// Variables two know if the ultrasonic sensor is working or not
#define	US_NO_OBSTACLE	-2

int distance, previous_distance;														// distance and previous_distance are distances measured and calculated with the equation 
int timer2flag = 0;																		// timer2flag is an interrupt flag used in Timer2 (with this we are changing how many times loop "for" needs to be counted 

void HCSR04Init();																		// Function to initialize HCSR04
void HCSR04Trigger();																	// Function to Trigger HCSR04 (set the trigger position to high and after 10us to low)
void lcd_command(unsigned char);														// Function to set LCD screen pointer to starting position 

void HCSR04Init()																		// Initialise HCSR04
{
	US_DDR |= (1 << US_TRIG_POS);														// We're setting the trigger pin as output as it will generate ultrasonic sound wave
}

void HCSR04Trigger()																	// This function will generate ultrasonic sound wave for 10 microseconds	//Send a 10uS pulse on trigger line
{
	US_PORT |= (1 << US_TRIG_POS);														// Trigger pin set to high
	_delay_us(10);																		// Wait 10us
	US_PORT &= (~(1 << US_TRIG_POS));													// Trigger pin set to low
}

uint16_t GetPulseWidth()																// Function to measure the pulse duration. Micro controller will read the time that passed between trigger and echo using the ultrasonic sensor connected to it.
{
	uint32_t i, result;																	// Checking if the ultrasonic is working or not
																						
	for(i = 0; i < 600000; i++)															// Check the echo for a certain amount of time. If there is no signal it means the sensor is not working or not connected
	{
		if(!(US_PIN & (1 << US_ECHO_POS)))												// If not (USPIN and US_ECHO_POS shifted to left)
		{
			continue;																	// Line is low, continue
		}
		else                                                                            // Else
		{
			break;																		// High detected, break
		}
	}

	if(i == 600000)                                                                     // If variable i is equal to 600000
	{
		return US_ERROR;																// Time out
	}
																						// High found. Preparing the timer for counting pulse.
	TCCR0A = 0X00;																		// Setup Timer0
	TCCR0B = (1 << CS01);																// Sets the resolution of the timer
	TCNT0 = 0x00;																		// Start counting time and check if there is any object or not
	
	for(i = 0; i < 600000; i++)															// The 600000 value is random to denote a very small amount of time, almost 40 miliseconds
	{
		if((US_PIN & (1 << US_ECHO_POS)))												// If USPIN and US_ECHO_POS shifted to left
		{
			if(TCNT0 > 60000)															// If TCNT0 is greater than 60000
			{
				break;																	// Then break
			}
			else																		// Else
			{
				continue;																// If the TCNT0 value is lower than 60000 it means there is not object in the range of the sensor
			}
		}
		else                                                                            // Else
		{
			break;																		// Then break
		}
	}
	if(i == 600000)																		// If variable i is equal to 600000
	{
		return US_NO_OBSTACLE;														 	// Indicates time out
	}
	
	result = TCNT0;																		// Falling edge found. The value of the counted pulse time is in the TCNT0 register.
	
	TCCR0B = 0x00;																		// Stop Timer

	if(result > 60000)																	// If variable result is greater than 60000
	{
		return US_NO_OBSTACLE;															// No obstacle
	}
	else                                                                                // Else
	{
		return (result >> 1);															// Return result with bit moved to right
	}
}

void initialize(void)																	// This function is to initialize the LCD screen
{
	lcd_data_dir = 0xFF;																// Set the LCD pins connected on the microcontroller as output
	_delay_ms(15);																		// Show data on the LCD we need to send commands first then the data
	lcd_command(0x02);																	// Returns the cursor to the first row and first column position
	lcd_command(0x28);																	// Sets to 4-bit 2 lines 5x7 dots
	lcd_command(0x0c);																	// Display ON, cursor OFF
	lcd_command(0x06);																	// Increment cursor (shift cursor to right)
	lcd_command(0x01);																	// Clear display screen
	_delay_ms(2);																		// Wait 2ms
}

void lcd_command(unsigned char cmnd)													// We're using 4 bit data communication but the data is 8-bit. We will send the data divinding it into 2 section. Higher 4 bit and lower 4 bit
{
	lcd_port = (lcd_port & 0x0F) | (cmnd & 0xF0);										// This line writes the command on the data pins of the lcd connected to th microcontroller portD pin 4 to 7
	lcd_port &= (~(1 << rs));															// Set the RS pin to zero
	lcd_port |= (1 << en);																// Enable pin to high
	_delay_us(1);																		// Wait for one microsecond
	lcd_port &= (~(1 << en));															// Enable pin to low
	_delay_us(200);																		// Wait 200 microseconds
	lcd_port = (lcd_port & 0x0F) | (cmnd << 4);											// Send the lower 4 bit of the data
	lcd_port |= (1 << en);																// Enable pin to high
	_delay_us(1);																		// Wait for one microsecond
	lcd_port &= (~(1 << en));															// Enable pin to low
	_delay_ms(2);																		// Wait 2 microseconds
}

void lcd_clear()																		// This function is to clear LCD screen
{
	lcd_command (0x01);																    // Clear the LCD screen
	_delay_ms(2);																		// Wait two milliseconds
	lcd_command (0x80);																	// Set the cursor to the row 1 column 1
}

void lcd_print (char *str)																// This function will be used to display the string on the LCD screen
{
	int i;																				// Integer i being used we need a loop
	for(i = 0; str[i] != 0; i++)														// Send character by character. Data sending is same as sending a command but in this case the RS pin is HIGH and RS pin is zero 
	{
		lcd_port = (lcd_port & 0x0F) | (str[i] & 0xF0);									// Works on pin 4 to 7
		lcd_port |= (1 << rs);															// Set the RS pin to one
		lcd_port |= (1 << en);															// Enable pin to high
		_delay_us(1);																	// Wait for one microsecond
		lcd_port &= (~(1 << en));														// Enable pin to low
		_delay_us(200);																	// Wait 200 microseconds
		lcd_port = (lcd_port & 0x0F) | (str[i] << 4);									// Send the lower 4 bit of the data
		lcd_port |= (1 << en);															// Enable pin to high
		_delay_us(1);																	// Wait for one microsecond
		lcd_port &= (~(1 << en));														// Enable pin to low
		_delay_ms(2);																	// Wait 2 microseconds
	}
}

void set_Diodes(int x)																	// This function is to set diodes depending on the distance and timer2flag (frequency of th buzzer)
{																						// NOTE I am using PINB1 to generate 1Hz signal on a diode so i need to use pins B: 0 2 3 4 (I can't write it as 0b00000000 as it will affect PINB1)
	switch(x)																			// Using switch case function
	{
		case 1:
		PORTB &= (~(1 << PINB0)) & (~(1 << PINB2)) & (~(1 << PINB3)) & (~(1 << PINB4)); // All diodes pins are set to 0 
		timer2flag = 20;																// Timer2 loop will count 20 times (16ms * 20 = 320ms )
		break;																			// Break from the switch case function
		
		case 2:
		PORTB |= (1 << PINB0);															// Only PINB0 is turned on
		PORTB &= (~(1 << PINB2)) & (~(1 << PINB3)) & (~(1 << PINB4));					// Look line above
		timer2flag = 15;																// Timer2 loop will count 15 times (16ms * 15 = 240ms )
		break;																			// Break from the switch case function
		
		case 3:
		PORTB |= (1 << PINB0) | (1 << PINB2);											// Only PINB0 and PINB2 are turned on
		PORTB &= (~(1 << PINB3)) & (~(1 << PINB4));										// Look line above
		timer2flag = 10;																// Timer2 loop will count 15 times (16ms * 10 = 160ms )
		break;																			// Break from the switch case function
		
		case 4:
		PORTB |= (1 << PINB0) | (1 << PINB2) | (1 << PINB3);							// PINB0, PINB2 and PINB3 are turned on
		PORTB &= (~(1 << PINB4));														// Look line above
		timer2flag = 5;																	// Timer2 loop will count 15 times (16ms * 5 = 80ms )
		break;																			// Break from the switch case function
		
		case 5:
		PORTB |= (1 << PINB0) | (1 << PINB2) | (1 << PINB3) | (1 << PINB4);				// Every diode pin is turned on
		timer2flag = 0;																	// Constant buzz (buzzer beeps without stopping)
		break;																			// Break from the switch case function
	}
}

void check_distance(int dis)															// This function is also used to check the distance of the diodes it is setting x values as a flag depending on ditance which then is used in switch case function
{
	int x = 0;																			// Define integer x and set to 0 
	_delay_ms(2);																		// Wait 2 ms
	if(dis >= 0 && dis < 5)																// If distance is between 0 and 5 
	{
		x = 5;																			// Set x to 5 
	}
	_delay_ms(2);																		// Wait 2 ms																	
	if(dis >= 5 && dis < 10)															// If distance is between 5 and 10 
	{
		x = 4;																			// Set x to 4 
	}
	_delay_ms(2);																		// Wait 2 ms
	if(dis >= 10 && dis < 15)															// If distance is between 10 and 15 
	{
		x = 3;																			// Set x to 3
	}
	_delay_ms(2);																		// Wait 2 ms
	if(dis >= 15 && dis < 20)															// If distance is between 15 and 20 
	{
		x = 2;																			// Set x to 2 
	}
	_delay_ms(2);																		// Wait 2 ms
	if(dis >= 20)																		// If distance is greater than 20 
	{
		x = 1;																			// Set x to 1 
	}
	_delay_ms(2);																		// Wait 2 ms
	set_Diodes(x);																		// Go to set_Diodes funtion with obtained x value
	x = 0;																				// Set x to 0 
}

void lcd_setCursor(unsigned char x, unsigned char y)									// Function will be used to set cursor. The place where we want to display the data
{
	unsigned char adr[] = {0x80, 0xC0};													// The 16x2 LCD has two rows first row has a value of 0x80. So let's say we want to go to the seconds column of first row
	
	lcd_command(adr[y-1] + x-1);														// Set cursor to certain place
	_delay_us(100);																		// Wait 100us
}
