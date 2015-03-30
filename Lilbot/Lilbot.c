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

//We want timer 0 to interrupt every 100 microseconds ((1/10000Hz)=100 us)
#define FREQ 10000L
//The reload value formula comes from the datasheet...
#define TIMER0_RELOAD_VALUE (65536L-((XTAL)/(2*FREQ)))

// Macro Defs
//#define 
#define SIDE_THRESH (10)
#define LEFT_OFFSET (0)
#define RIGHT_OFFSET (0)
#define LEFT_SENSOR (AD1DAT0 + LEFT_OFFSET)
#define LEFT_THRESH (10)
#define RIGHT_SENSOR (AD1DAT2 + RIGHT_OFFSET)
#define RIGHT_THRESH (10)
#define LCD_FREQ (100)
#define SCALE (0.2)

// PID Settings
#define Kp (1.0)
#define Ki (0.0)
#define Kd (0.2)

// Driving Macros
#define LEFT_ON (LEFT_SENSOR >= LEFT_THRESH)
#define RIGHT_ON (RIGHT_SENSOR >= RIGHT_THRESH)
#define LEFT_OFF (LEFT_SENSOR < LEFT_THRESH)
#define RIGHT_OFF (RIGHT_SENSOR < RIGHT_THRESH)

// Driving Settings
#define BASE_SPEED (80)

// Pulse Settings
#define PULSE_SENSOR (AD1DAT3)
#define PULSE_THRESH (150)
#define PULSE_ON (PULSE_SENSOR >= PULSE_THRESH && (LEFT_ON && RIGHT_ON))
#define PULSE_OFF (PULSE_SENSOR < PULSE_THRESH)
#define RISING_PULSE (pulsed == 1 && lastPulse == 0)
#define FALLING_PULSE (pulsed == 0 && lastPulse == 1)

//Pins
#define LEFT_PIN P3_1
#define RIGHT_PIN P3_0

/*
		SCROLL TO BOTTOM FOR MAIN FUNCTIONS
*/

//These variables are used in the ISR
volatile unsigned char pwmcount;
volatile unsigned char leftSpeed;
volatile unsigned char rightSpeed;

// The volatile keyword prevents the compiler from optimizing out these variables
// that are shared between an interrupt service routine and the main code.
volatile int msCount=0;
volatile unsigned long totalTimeCount=0;
volatile unsigned char secs=0, mins=0;
volatile bit time_update_flag=0;

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
char count=0;
int LCDcount=0;
short error = 0;
short lastError = 0;
short dT = 0;
unsigned long lastPIDtime = 0;
char dir = 0;
char steerOutput = 0;

//pulse
bit pulsed = 0;
bit lastPulse = 0;
char pulseCount = 0;
char expTurn = 0;
unsigned char pulseStartTime = 0; //in secs

// Display Strings
char string1[17];
char string2[17];

void InitPorts(void)
{
	P0M1=0;
	P0M2=0;
	P1M1=0;
	P1M2=0;
	P2M1=0;
	P2M2=0;
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
	P3M1=0x00; //Enable pins RxD and Txd
	P3M2=0x00; //Enable pins RxD and Txd
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

void InitTimer0 (void)
{
	// Initialize timer 0 for ISR 'pwmcounter' below
	TR0=0; // Stop timer 0
	TMOD=(TMOD&0xf0)|0x01; // 16-bit timer
	TH0=TIMER0_RELOAD_VALUE/0x100;
	TL0=TIMER0_RELOAD_VALUE%0x100;
	TR0=1; // Start timer 0 (bit 4 in TCON)
	ET0=1; // Enable timer 0 interrupt
	EA=1;  // Enable global interrupts
}

//Interrupt 1 is for timer 0.  This function is executed every 100 us.
void Timer0Interrupt (void) interrupt 1
{
	//Reload the timer
	TR0=0; // Stop timer 0
	TH0=TIMER0_RELOAD_VALUE/0x100;
	TL0=TIMER0_RELOAD_VALUE%0x100;
	TR0=1; // Start timer 0
	if(++pwmcount>99) pwmcount=0;
	LEFT_PIN=(leftSpeed>pwmcount)?0:1; //reverse because 1 turns off the motor
	RIGHT_PIN=(rightSpeed>pwmcount)?0:1;
	
	
	totalTimeCount++;
	
	if(count++ == 10)
	{
		count = 0;
		msCount++;
	}
	
	if(msCount++==1000)
	{
		time_update_flag=1;
		msCount=0;
		secs++;
		if(secs==60)
		{
			secs=0;
			mins++;
			if(mins==60)
			{
				mins=0;
			}
		}
	}
	
	
}




// ==================MAIN FUNCTION===========================

void Setup(void)
{
	InitPorts();
	LCD_8BIT();
	
	InitSerialPort();
	InitADC();
	
	InitTimer0();
	
	leftSpeed = 0;
	rightSpeed = 0;
	pulseCount = 0;
}


void UpdateString(void)
{

	if(LCDcount++ > LCD_FREQ){
		sprintf(string1, "L: %i, R: %i", LEFT_SENSOR, RIGHT_SENSOR);
		sprintf(string2, "L: %i, R: %i", leftSpeed, rightSpeed);
		LCDprint(string1,1,1);
		LCDprint(string2,2,1);
		LCDcount = 0;
	}
}

void printPulseInfo(void)
{

	if(LCDcount++ > LCD_FREQ){
		sprintf(string1, "P: %i, Pc: %i", (int)PULSE_SENSOR, (int)pulseCount);
		sprintf(string2, "T: %i",  (int)expTurn);
		LCDprint(string1,1,1);
		LCDprint(string2,2,1);
		LCDcount = 0;
	}
}

void UpdateTimeString(void)
{
	if(time_update_flag==1) // If the clock has been updated refresh the display
	{
		time_update_flag=0;
		sprintf(string1, "V=%5.2f", (AD1DAT3/255.0)*3.3); // Display the voltage at pin P0.2
		LCDprint(string1, 1, 1);
		sprintf(string2, "Time: %02d:%02d", mins, secs); // Display the clock
		LCDprint(string2,2,1);
	}	
}

//==============PID CODE=====================


void updatePID(void)
{
	dT = totalTimeCount - lastPIDtime;

	lastError = error;
	lastPIDtime = totalTimeCount;

	error = LEFT_SENSOR - RIGHT_SENSOR;
	/*
	if(abs(error) > SIDE_THRESH)
	{
		error = 0;
	}*/

	steerOutput = Kp*error + ((float)Kd*(error-lastError))/(dT);
	//steerOutput = (int)(Kp*error + Kd*(error - lastError)/((float)dT) + Ki*(error)*dT);

}

//==============PULSE COUNT CODE==================

void updatePulse(void)
{
	lastPulse = pulsed;
	pulsed = PULSE_ON;
}

void checkForPulse(void)
{
	updatePulse();
	if(RISING_PULSE)
	{
			lastPulse = 0;
			pulsed = 1;
			pulseStartTime = secs;
	}
	else if(FALLING_PULSE)
	{
		if(PULSE_OFF) //pulse drops off
		{
			lastPulse = 1;
			pulsed = 0;
			pulseCount++;
		}
	}
}



//=============DRIVING CODE================

int bound(int val, int min, int max)
{
	if(val > max)
	{
		return max;
	}
	else if(val < min)
	{
		return min;
	}
	else
	{
		return val;
	}
}


void drive(void)
{
	if(LEFT_ON && RIGHT_ON)
	{
		updatePID();
		
		if(steerOutput > 0)
		{
			dir = 1;
			leftSpeed = bound(BASE_SPEED - steerOutput, 0, 100);
			rightSpeed = bound(BASE_SPEED + steerOutput*SCALE, 0, 100);
		}
		else
		{
			dir = -1;
			leftSpeed = bound(BASE_SPEED - steerOutput*SCALE, 0, 100);
			rightSpeed = bound(BASE_SPEED + steerOutput, 0, 100);
		}
		
	}
	else if(LEFT_ON && RIGHT_OFF)
	{
		leftSpeed = 0;
		rightSpeed = 100;
		dir = 1;
	}
	else if(LEFT_OFF && RIGHT_ON)
	{
		leftSpeed = 100;
		rightSpeed = 0;
		dir = -1;
	}
	else //both off
	{
		if(dir == 1)
		{
			leftSpeed = 0;
			rightSpeed = 100;
		}
		else
		{
			leftSpeed = 100;
			rightSpeed = 0;
		}
	}
	
}

//===========================MAIN===========================

void main (void)
{
	Setup();
	
	sprintf(string1, "Initializing...");
	LCDprint(string1,1,1);
	Wait1S();
	
	
	//Start
	do{
		printPulseInfo();
		checkForPulse();
		drive();
	}while(pulseCount < 4);
	
	pulseCount = 0;
	pulseStartTime = secs;
	
	// skip the first 2 glitchy pulses
	do{
		printPulseInfo();
		drive();
	}while(secs - pulseStartTime < 4);
	
	pulseStartTime = secs;
	
	do{
		printPulseInfo();
		checkForPulse();
		drive();
		if(secs - pulseStartTime >= 1)
		{
			pulseCount = 0; //signifies that you don't count that pulse
			expTurn = 0;
			break;
		}
	}while(pulseCount < 2);
	
	if(pulseCount == 2) expTurn = 1;
	
	
	// LOOP	
	while(1)
	{
		printPulseInfo();
		checkForPulse();
		drive();
		//leftSpeed = 70;
		//rightSpeed = 20;
	}
	
}














