#include <stdlib.h>
#include <stdio.h>
#include <p89lpc9351.h>

#define XTAL 7373000L
#define BAUD 115200L


// Make sure these definitions match your wiring
#define LCD_RS P2_7
#define LCD_RW P2_6
#define LCD_E  P2_5
#define LCD_D7 P1_4
#define LCD_D6 P1_6
#define LCD_D5 P1_7
#define LCD_D4 P2_0
#define LCD_D3 P2_1
#define LCD_D2 P2_2
#define LCD_D1 P2_3
#define LCD_D0 P2_4
#define CHARS_PER_LINE 16

// Macro Defs
//#define 
#define SIDE_THRESH (100)
#define LEFT_OFFSET (0)
#define RIGHT_OFFSET (30)
#define LEFT_SENSOR (AD1DAT0 + LEFT_OFFSET)
#define RIGHT_SENSOR (AD1DAT2 + RIGHT_OFFSET)
#define LCD_FREQ (100)

/*
		SCROLL TO BOTTOM FOR MAIN FUNCTIONS
*/

// ======= Delay Funcs =======

void Wait50us (void)
{
	_asm
    mov R0, #82
L0: djnz R0, L0 ; 2 machine cycles-> 2*0.27126us*92=50us
    _endasm;
}


void waitms (unsigned int ms)
{
	unsigned int j;
	unsigned char k;
	for(j=0; j<ms; j++)
		for (k=0; k<20; k++) Wait50us();
}


void Wait1S (void)
{
	_asm
	mov R2, #40
L3: mov R1, #250
L2: mov R0, #184
L1: djnz R0, L1 ; 2 machine cycles-> 2*0.27126us*184=100us
    djnz R1, L2 ; 100us*250=0.025s
    djnz R2, L3 ; 0.025s*40=1s
    _endasm;
}

// =============== LCD Funcs =============

void LCD_pulse (void)
{
	LCD_E=1;
	Wait50us();
	LCD_E=0;
}

void LCD_byte (unsigned char x)
{
	// The accumulator in the C8051Fxxx is bit addressable!
	ACC=x;
	LCD_D7=ACC_7;
	LCD_D6=ACC_6;
	LCD_D5=ACC_5;
	LCD_D4=ACC_4;
	LCD_D3=ACC_3;
	LCD_D2=ACC_2;
	LCD_D1=ACC_1;
	LCD_D0=ACC_0;
	LCD_pulse();
}

void WriteData (unsigned char x)
{
	LCD_RS=1;
	LCD_byte(x);
	waitms(2);
}

void WriteCommand (unsigned char x)
{
	LCD_RS=0;
	LCD_byte(x);
	waitms(5);
}

void LCD_8BIT (void)
{
	LCD_E=0;  // Resting state of LCD's enable is zero
	LCD_RW=0; // We are only writing to the LCD in this program
	waitms(20);
	// First make sure the LCD is in 8-bit mode
	WriteCommand(0x33);
	WriteCommand(0x33);
	WriteCommand(0x33); // Stay in 8-bit mode

	// Configure the LCD
	WriteCommand(0x38);
	WriteCommand(0x0c);
	WriteCommand(0x01); // Clear screen command (takes some time)
	waitms(20); // Wait for clear screen command to finsih.
}

void LCDprint(char * string, unsigned char line, bit clear)
{
	unsigned char j;

	WriteCommand(line==2?0xc0:0x80);
	waitms(5);
	for(j=0; string[j]!=0; j++)	WriteData(string[j]);// Write the message
	if(clear) for(; j<CHARS_PER_LINE; j++) WriteData(' '); // Clear the rest of the line
}


// ================= Initialization Funcs ===================

// ------------ Variable Inits ------------
int count=0;
int error = 0;

void InitPorts(void)
{
	P0M1=0;
	P0M2=0;
	P1M1=0;
	P1M2=0;
	P2M1=0;
	P2M2=0;
	P3M1=0;
	P3M2=0;
}


void InitSerialPort(void)
{
	BRGCON=0x00; //Make sure the baud rate generator is off
	BRGR1=((XTAL/BAUD)-16)/0x100;
	BRGR0=((XTAL/BAUD)-16)%0x100;
	BRGCON=0x03; //Turn-on the baud rate generator
	SCON=0x52; //Serial port in mode 1, ren, txrdy, rxempty
	P1M1=0x00; //Enable pins RxD and Txd
	P1M2=0x00; //Enable pins RxD and Txd
}

void InitADC(void)
{
	// Set adc1 channel pins as input only 
	P0M1 |= (P0M1_4 | P0M1_3 | P0M1_2 | P0M1_1);
	P0M2 &= ~(P0M1_4 | P0M1_3 | P0M1_2 | P0M1_1);

	BURST1=1; //Autoscan continuos conversion mode
	ADMODB = CLK0; //ADC1 clock is 7.3728MHz/2
	ADINS  = (ADI13|ADI12|ADI11|ADI10); // Select the four channels for conversion
	ADCON1 = (ENADC1|ADCS10); //Enable the converter and start immediately
	while((ADCI1&ADCON1)==0); //Wait for first conversion to complete
}




// ==================MAIN FUNCTION===========================

void Setup(void)
{
	InitPorts();
	LCD_8BIT();
	
	InitSerialPort();
	InitADC();
}


void UpdateString(void)
{
	if(count++ > LCD_FREQ){
		char string1[17];
		char string2[17];
		sprintf(string1, "L: %i, R: %i", LEFT_SENSOR, RIGHT_SENSOR);
		sprintf(string2, "Error: %i", error);
		LCDprint(string1,1,1);
		LCDprint(string2,2,1);
		count = 0;
	}
}

void driveRight(void)
{
	P3_0=0;
}

void driveLeft(void)
{
	P3_1=0;
}

void stopRight(void)
{
	P3_0=1;
}

void stopLeft(void)
{
	P3_1=1;
}

void drive(void)
{
	error = LEFT_SENSOR - RIGHT_SENSOR;
	
	if(error > SIDE_THRESH) //veering to the right
	{
		stopLeft();
		driveRight();
	}
	else if(error < -SIDE_THRESH)
	{
		stopRight();
		driveLeft();
	}
	else{
		driveLeft();
		driveRight();
	}
}


void main (void)
{
	Setup();
	
	// LOOP	
	while(1)
	{
		UpdateString();
		drive();
	}
	
}














