/*
 * STP9CF55_TINY404.cpp
 *
 * Created: 7/20/2021 4:10:18 PM
 * Author : MILAD
 */ 

#define F_CPU 10000000UL
#define Release_CCP		0xD8
#define CLK_PER_DISABLE	0x00
#define CLK_PER_EN_TO_2	0x01
#define CLK_PER_EN_TO_4	0x03

#define USART0_BAUD_RATE(BAUD_RATE) ((float)(F_CPU * 64 / (16 * (float)BAUD_RATE)) + 0.5)

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>
#include <avr/interrupt.h>
#include <math.h>
//#include "EEPROM.h"


#define UV_OUT_CH			1

float Alpha = 1; // Co-efficient to convert ADC output to UVI

void WD_Reset_2S(void)
{
	CPU_CCP = Release_CCP; // Allows to change Configuration Change Protected (CCP) register
	WDT.CTRLA = WDT_PERIOD_2KCLK_gc;
}

void WD_Reset_8mS(void)
{
	CPU_CCP = Release_CCP; // Allows to change Configuration Change Protected (CCP) register
	WDT.CTRLA = WDT_PERIOD_8CLK_gc;
}

void USART_Init(void)
{
	PORTA.DIR |= PIN6_bm; // Set TX pin as output
	PORTA.DIR &= ~PIN7_bm; // Set RX pin as input
	
	USART0.BAUD = (uint16_t)USART0_BAUD_RATE(9600);
	USART0.CTRLB |= USART_TXEN_bm | USART_RXEN_bm;
}

void USART_Transmit(char c)
{
	while (!(USART0.STATUS & USART_DREIF_bm))
	{
		;
	}
	USART0.TXDATAL = c;
}

char USART_Receive()
{
	while (!(USART0.STATUS & USART_RXCIF_bm))
	{
		;
	}
	return USART0.RXDATAL;
}

ISR(USART0_RXC_vect)
{
	char rx;
	rx = USART_Receive();
	
	if (rx == 'r')
	{
		WD_Reset_8mS();
		_delay_ms(50);
	}

}

// void USART_sendString(char *str)
// {
// 	for(uint8_t i = 0; i < strlen(str); i++)
// 	{
// 		USART_Transmit(str[i]);
// 	}
//


void ADC0_init(void);
uint16_t ADC0_read(void);
void ADC0_start(void);
bool ADC0_conersionDone(void);

void ADC0_init(void)
{
	/* Disable digital input buffer */
	PORTA.PIN1CTRL &= ~PORT_ISC_gm;
	PORTA.PIN1CTRL |= PORT_ISC_INPUT_DISABLE_gc;
	/* Disable pull-up resistor */
	PORTA.PIN1CTRL &= ~PORT_PULLUPEN_bm;

	ADC0.CTRLC = ADC_PRESC_DIV4_gc /* CLK_PER divided by 4 */
	| ADC_REFSEL_VDDREF_gc;

	ADC0.CTRLA = ADC_ENABLE_bm /* ADC Enable: enabled */
	| ADC_RESSEL_10BIT_gc; /* 10-bit mode */
}

void ADC0_Select_CH(uint8_t CH)
{
	/* Select ADC channel */
	//	ADC0.MUXPOS = ADC_MUXPOS_AIN1_gc;
	ADC0.MUXPOS = (CH<<0);

	/* Enable FreeRun mode */
	ADC0.CTRLA |= ADC_FREERUN_bm;
	
	/* Start conversion */
	ADC0.COMMAND = ADC_STCONV_bm;
}
uint16_t ADC0_read(void)
{
	/* Wait until ADC conversion done */
	while ( !(ADC0.INTFLAGS & ADC_RESRDY_bm) )
	{
		;
	}

	/* Clear the interrupt flag by writing 1: */
	ADC0.INTFLAGS = ADC_RESRDY_bm;

	return ADC0.RES;
}


int main(void)
{
	/*  Clock Setting	*/
	// Clock source is internal 20Mhz and divided by 2 -> CPU clock = 10Mhz
	CPU_CCP = Release_CCP; // Allows to change Configuration Change Protected (CCP) register
	CLKCTRL_MCLKCTRLA = CLKCTRL_CLKSEL_OSC20M_gc; // Select internal 20 Mhz as the source clock
	CPU_CCP = Release_CCP;
	CLKCTRL_MCLKCTRLB = CLK_PER_EN_TO_2; // Enable CLK Prescaler and set it to 2
	_delay_ms(10);
	
	WD_Reset_2S(); // Set WD timer of 2 second
	
	/*  Initialization	*/
	USART_Init();
	USART0.CTRLA |= USART_RXCIE_bm; // enable receive interrupt vector
	sei(); // Enable Interrupts

	ADC0_init(); // UV_OUT is read through PA6, ADC CH 1

	_delay_ms(100);
	while (1)
	{
		WD_Reset_2S(); // Set WD timer of 2 second
		
		float UV_OUT;
		
		UV_OUT = 0.0;
		
		for (uint8_t i=0 ; i<10 ; i++)
		{
			ADC0_Select_CH(UV_OUT_CH);
			_delay_ms(10);
			UV_OUT += ADC0_read();
		}

		UV_OUT *= 0.000320626; //   0.00320626 = 3.28(VCC)/1023(10bit ADC) 
		
		
		UV_OUT = 0.1569*exp(3.394*UV_OUT);
		
		if (UV_OUT<=0.17) UV_OUT = 0;
		UV_OUT *=10;
			
		if (UV_OUT - uint8_t(UV_OUT) > 0.5) UV_OUT++; //To have more accurate casting from float number
		
		USART_Transmit(0x03); // Start Bit ID for UV Meter
		
		USART_Transmit(UV_OUT); // Send UVI*10 to the receiver
		USART_Transmit('\n'); // Packet terminator
		_delay_ms(300); // Set refresh rate
	}
}


/* /////////   Port and Pins
	PORTB.DIR |= PIN0_bm;
	PORTB.OUT |= PIN0_bm;
*/


/* /////////	EEPROM
//			uint8_t EE;
// 			EEPROM.write(0x01,0);
// 			_delay_ms(100);
//			EE = EEPROM.read(0x01);
// 			_delay_ms(100);
//			USART_Transmit(EE);
//			_delay_ms(1000);
*/


