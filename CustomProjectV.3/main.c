/*
 * CustomProjectV.3.c
 *
 * Created: 6/7/2017 12:38:25 PM
 * Author : Jeremy Chan
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "timer.h"
#include "io.c"
#include "keypad.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//BIG SQUARE
unsigned char bigX[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};//columns
unsigned char bigY[] = {0x00,0x7E,0x7E,0x7E,0x7E,0x7E,0x7E,0x00};//rows

//BIG MEDIUM SQUARE
unsigned char bigMX[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};//columns
unsigned char bigMY[] = {0xFF,0x81,0xBD,0xBD,0xBD,0xBD,0x81,0xFF};//rows

//SMALL MEDIUM SQUARE
unsigned char smallMX[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};//columns
unsigned char smallMY[] = {0xFF,0xFF,0xC3,0xDB,0xDB,0xC3,0xFF,0xFF};//rows

//SMALL SQUARE
unsigned char smallX[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};//columns
unsigned char smallY[] = {0xFF,0xFF,0xFF,0xE7,0xE7,0xFF,0xFF,0xFF};//rows

//keycode variables and master variable
unsigned char instruction,iCompleted, keyPressed,weightToggle = 0;//instruction is index of rand instruction iCompleted is if the instruction finished
unsigned int gameTimer = 0;//timer of game
unsigned int gameTADD = 100;
unsigned int uPoints = 0;//user points and what they need to get to
char keyPadEntry[4] = {'-','-','-','-'};//user entry
unsigned char keyPadIndex = 0;//where the keypad input will go
char randkeyPadCode[4] = {'-','-','-','-'};//random keycode entered
unsigned char startButtonPressed = 0;
unsigned int seed = 0;
unsigned int firstTime = 0;
unsigned char gameOn = 0;
unsigned char GAMEOVER = 0;
char numInstruc = 1;
unsigned char numIADD = 1;
unsigned char multiplier = 1;
unsigned char killed = 0; // for the asteroid game
unsigned char cnt = 40;
unsigned short onumInstruct;
unsigned short ogameTime;

//weight variables
unsigned int fullW = 40;//harder weight
unsigned int halfW = 150;//weaker weight
unsigned int currW = 0;
unsigned int wTick = 0;

//joystick variables
unsigned short x,y = 0;  // Value of ADC register now stored in variable x
unsigned char direction = 0;
unsigned char directionToggle = 0;
unsigned int jTick = 0;

//Rotary encoder variables
unsigned short rV = 0;  // Value of ADC register now stored in variable x
unsigned char tmpA = 0;
unsigned char toggle = 0;
int MAX = 0;
short rotPos = 0;
unsigned char left = 0;
unsigned char right = 0;
unsigned char toLeft = 0;
unsigned char toRight = 0;
unsigned int rTick = 0;
short randPos = 0;
unsigned char randPosIndex = 0;
unsigned char val;

//Asteroid game
unsigned char shipX[] = {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};//columns
unsigned char shipY[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};//rows

unsigned char resetX = {0x00};
unsigned char resetY = {0xFF};
unsigned char v = 0;
unsigned char shipMoved = 0;
unsigned short aCount = 100;
unsigned char asteroidsPassed = 6;
unsigned char gameStart = 0;
unsigned char aCompleted = 0;
char killedCnt = 20;
unsigned char speed = 0;

//Hyper drive variables
char index = 0;
int T = 0;
short num = 200;
unsigned char hCompleted = 0;
unsigned char hLost = 0;
unsigned char Hkilled = 0;
unsigned char hToggle =0;
unsigned char hButtonPressed = 0;


//all sm states
enum K_States {K_Begin, grabKey} Kstates;
enum W_States {W_Begin, grabWeight} Wstates;
enum J_States {J_Begin, grabDirection} Jstates;
enum R_States {R_Begin, grabRotation} Rstates;
enum A_States {A_Begin, A_inGame} Astates;
enum H_States {H_Begin, H_inGame} Hstates;
enum M_States {M_Begin, M_Sta, M_asteroid, M_hyperDrive ,M_level ,M_gameOver} Mstates;//M_hyperdrive not used
	
void ADC_init() {
	ADCSRA = 0;
	ADMUX = 0;
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}


//Used the code from http://www.embedds.com/interfacing-analog-joystick-with-avr/ to know how to convert the adc of the
//2 axis joystick within one tick. Both the INITADC() and the readadc() functions came from this site

void InitADC(void)
{
	ADMUX = 0;
	ADCSRA = 0;
	ADMUX|=(1<<REFS0);
	ADCSRA|=(1<<ADEN)|(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2); //ENABLE ADC, PRESCALER 128
}

uint16_t readadc(uint8_t ch)
{
	ch&=0b00000111;         //ANDing to limit input to 7
	ADMUX = (ADMUX & 0xf8)|ch;  //Clear last 3 bits of ADMUX, OR with ch
	ADCSRA|=(1<<ADSC);        //START CONVERSION
	while((ADCSRA)&(1<<ADSC));    //WAIT UNTIL CONVERSION IS COMPLETE
	return(ADC);        //RETURN ADC VALUE
}

typedef struct asteroid_
{
	int Y;
	unsigned char X;
	
} asteroid;

asteroid myAsteroid1;
asteroid myAsteroid2;

typedef struct ship_
{
	unsigned char leftLeg;
	unsigned char front;
	unsigned char rightLeg;
	
} ship;

ship myShip;
	
typedef struct _task {
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TaskFct) (int);
} task;

//Used the code from https://ilearn.ucr.edu/bbcswebdav/pid-2932363-dt-content-rid-16110946_1/courses/CS_120B_001_17S/eecs120b_labX_external_shift_register-2.pdf
//to know how to wire the shift registers and the falgs needed to be set to use the registers. Both the transmit_data1() and the transmit_data2() functions came from this site
//and were modified for my specific needs.

void transmit_data1(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}

void transmit_data2(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x80;
		// set SER = next bit of data to be sent.
		PORTB |= (((data >> i) & 0x01)<<4);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x20;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x40;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}

void resetShip()
{
	for(int i = 0; i < 8;i++)
	{
		shipY[i] = 0xFF;
	}
}

void positionAsteroid()
{
		if(myAsteroid1.Y == 7)
		{
			shipY[myAsteroid1.X] = 0x7C + (shipY[myAsteroid1.X] & 0x03);
		}
		else if(myAsteroid1.Y == 6)
		{
			shipY[myAsteroid1.X] = 0xBC + (shipY[myAsteroid1.X] & 0x03);
		}
		else if(myAsteroid1.Y == 5)
		{
			shipY[myAsteroid1.X] = 0xDC + (shipY[myAsteroid1.X] & 0x03);
		}
		else if(myAsteroid1.Y == 4)
		{
			shipY[myAsteroid1.X] = 0xEC + (shipY[myAsteroid1.X] & 0x03);
		}
		else if(myAsteroid1.Y == 3)
		{
			shipY[myAsteroid1.X] = 0xF4 + (shipY[myAsteroid1.X] & 0x03);
		}
		else if(myAsteroid1.Y == 2)
		{
			shipY[myAsteroid1.X] = 0xF8 + (shipY[myAsteroid1.X] & 0x03);
		}
		else if(myAsteroid1.Y == 1)
		{
			shipY[myAsteroid1.X] = 0xFC + (shipY[myAsteroid1.X] & 0x01);
		}
		else if(myAsteroid1.Y == 0)
		{
			shipY[myAsteroid1.X] = 0xFE;
		}
		else if(myAsteroid1.Y == -1)
		{
			shipY[myAsteroid1.X] = 0xFF;
		}	

        if(myAsteroid2.Y == 7)
        {
	        shipY[myAsteroid2.X] = 0x7C + (shipY[myAsteroid2.X] & 0x03);
        }
        else if(myAsteroid2.Y == 6)
        {
	        shipY[myAsteroid2.X] = 0xBC + (shipY[myAsteroid2.X] & 0x03);
        }
        else if(myAsteroid2.Y == 5)
        {
	        shipY[myAsteroid2.X] = 0xDC + (shipY[myAsteroid2.X] & 0x03);
        }
        else if(myAsteroid2.Y == 4)
        {
	        shipY[myAsteroid2.X] = 0xEC + (shipY[myAsteroid2.X] & 0x03);
        }
        else if(myAsteroid2.Y == 3)
        {
	        shipY[myAsteroid2.X] = 0xF4 + (shipY[myAsteroid2.X] & 0x03);
        }
        else if(myAsteroid2.Y == 2)
        {
	        shipY[myAsteroid2.X] = 0xF8 + (shipY[myAsteroid2.X] & 0x03);
        }
        else if(myAsteroid2.Y == 1)
        {
	        shipY[myAsteroid2.X] = 0xFC + (shipY[myAsteroid2.X] & 0x01);
        }
        else if(myAsteroid2.Y == 0)
        {
	        shipY[myAsteroid2.X] = 0xFE;
        }
        else if(myAsteroid2.Y == -1)
        {
	        shipY[myAsteroid2.X] = 0xFF;
        }
}

void positionShip()
{
	shipY[myShip.leftLeg] = (shipY[myShip.leftLeg] & 0xFE) + 0;
	shipY[myShip.front] = (shipY[myShip.front] & 0xFD) + 0;
	shipY[myShip.rightLeg] = (shipY[myShip.rightLeg] & 0xFE) + 0;
}

void drawSpace()
{

		transmit_data1(resetX);
		transmit_data2(resetY);
	
	
	transmit_data1(shipX[v]);
	transmit_data2(shipY[v]);
	if(v >= 7)
	{
		v = 0;
	}
	else
	{
		v++;
	}
}

void clearKeyCode()
{
	keyPadEntry[0] = '-';
	keyPadEntry[1] = '-';
	keyPadEntry[2] = '-';
	keyPadEntry[3] = '-';
}

void randomKeyCode()
{
	unsigned char temp = rand() % 10;
	
	for(int i = 0; i < 4; i++)
	{
		temp = rand() % 10;
		if(temp == 10)
		{
			randkeyPadCode[i] = '#';
		}
		else if(temp == 11)
		{
			randkeyPadCode[i] = 'A';
		}
		else if(temp == 12)
		{
			randkeyPadCode[i] = 'B';
		}
		else if(temp == 13)
		{
			randkeyPadCode[i] = 'C';
		}
		else if(temp == 14)
		{
			randkeyPadCode[i] = 'D';
		}
		else
		{
			randkeyPadCode[i] = (char)temp;
		}
	}
}

int R_tick(int state) {
	switch (state){
		case R_Begin:
		state = grabRotation;
		break;
		
		case grabRotation:
		state = grabRotation;
		break;
		
		default:
		state = R_Begin;
		break;
	}
	
	switch (state){
		case R_Begin:
		break;
		
		case grabRotation:
		if(toggle == 0)
		{
			toggle = 1;
			ADMUX = 0x05;
		}
		else
		{
			toggle = 0;
			ADMUX = 0x06;
		}
		
		rV = ADC;
		
		tmpA = ((char)rV);
		
		if(tmpA < MAX && toggle == 1)
		{
			if(left == 0 && right == 0)
			{
				toRight = 1;
			}
			left = 1;
			val = (val & 0x01) + 0x02;
		}
		else if(tmpA >= MAX && toggle == 1)
		{
			if(left == 1 && toLeft == 1)
			{
				toLeft = 0;
				rotPos++;
			}
			left = 0;
			val = val & 0x01;
		}
		
		if(tmpA < MAX && toggle == 0)
		{
			if(left == 0 && right == 0)
			{
				toLeft = 1;
			}
			right = 1;
			val = (val & 0x02) + 0x01;
			
		}
		else if(tmpA >= MAX && toggle == 0)
		{
			if(right == 1 && toRight == 1)
			{
				toRight = 0;
				rotPos--;
			}
			right = 0;
			val = val & 0x02;
		}
		break;
		
		default:
		break;
	}
	return state;
}

int J_tick(int state) {
	switch (state){
		case J_Begin:
		state = grabDirection;
		break;
		
		case grabDirection:
		state = grabDirection;
		break;
		
		default:
		state = J_Begin;
		break;
	}
	
	switch (state){
		case J_Begin:
		break;
		
		case grabDirection:
		x = (unsigned char) readadc(0);
		y = (unsigned char)readadc(1);
		if(x > 200 && y < 200 && y > 27)
		{
			direction = 0;//right
		}
		else if (x < 27 && y < 200 && y > 27)
		{
			direction = 1;//left
		}
		else if (y > 200 && x < 150 && x > 30)
		{
			direction = 2;//up
		}
		else if (y < 27 && x < 150 && x > 30)
		{
			direction = 3;//down
		}
		/*else if (x < 400)
		{
			direction = 0x02;//left
		}
		else if (x < 400)
		{
			direction = 0x02;//left
		}
		else if (x < 400)
		{
			direction = 0x02;//left
		}
		else if (x < 400)
		{
			direction = 0x02;//left
		}*/
		else
		{
			direction = 5;
		}
		break;
		
		default:
		break;
	}
	return state;
}

int W_tick(int state) {
	switch (state){
		case W_Begin:
		state = grabWeight;
		break;
		
		case grabWeight:
		state = grabWeight;
		break;
		
		default:
		state = W_Begin;
		break;
	}
	
	switch (state){
		case W_Begin:
		break;
		
		case grabWeight:
		ADMUX = 0x04;
		currW = (char)ADC;
		break;
		
		default:
		break;
	}
	return state;
}

int K_tick(int state) {
	switch (state){
		case K_Begin:
		state = grabKey;
		break;
		
		case grabKey:
		state = grabKey;
		break;
		
		default:
		state = K_Begin;
		break;
	}
	
	switch (state){
		case K_Begin:
		break;
		
		case grabKey:
		keyPressed = GetKeypadKey();
		break;
		
		default:
		break;
	}
	return state;
}

int H_tick(int state) {
	switch (state){
		case H_Begin:
		if(gameStart == 1)
		{
			gameStart = 0;
			InitADC();
			index = 0;
			T = 0;
			num = 200;
			state = H_inGame;
		}
		else
		{
			state = H_Begin;
		}
		break;
		
		case H_inGame:
		if(gameStart == 1)
		{
			state = H_Begin;
		}
		else
		{
			state = H_inGame;
		}
		break;
		
		default:
		state = H_Begin;
		break;
	}
	
	switch (state){
		case H_Begin:
		break;
		
		case H_inGame:
		hButtonPressed = ~PINA & 0x80;
		if((hButtonPressed == 0x80) &&	index == 3)
		{
			hCompleted = 1;
			hToggle=1;
		}
		else if((hButtonPressed == 0x80) &&	index != 3)
		{
			hToggle = 1;
			Hkilled = 1;
		}
		
		if(hToggle == 1)
		{
			
		}
		else
		{
			if(num >= 0)
			{
				if(index == 0)
				{
					transmit_data1(bigX[T]);
					transmit_data2(bigY[T]);
				}
				else if(index == 1)
				{
					transmit_data1(bigMX[T]);
					transmit_data2(bigMY[T]);
				}
				else if(index == 2)
				{
					transmit_data1(smallMX[T]);
					transmit_data2(smallMY[T]);
				}
				else if(index == 3)
				{
					transmit_data1(smallX[T]);
					transmit_data2(smallY[T]);
				}
				num--;
			}
			else
			{
				if(index == 3)
				{
					num = 200;
					index = 0;
				}
				else if(index == 2)
				{
					num = 200;
					index++;
				}
				else if(index == 1)
				{
					num = 200;
					index++;
				}
				else
				{
					num = 50;
					index++;
				}
			}
			
			if(T >= 7)
			{
				T = 0;
			}
			else
			{
				T++;
			}
		}
		
		
		break;
		
		default:
		break;
	}
	return state;
}

int A_tick(int state) {
	switch (state){
		case A_Begin:
		if(gameStart == 1)
		{
			killedCnt = 20;
			asteroidsPassed = 6;
			gameStart = 0;
			aCount = 100;
			InitADC();
			
			myShip.leftLeg = 3;
			myShip.rightLeg = 5;
			myShip.front = 4;
			resetShip();
			positionShip();
			
			myAsteroid1.X = rand() % 8;
			myAsteroid1.Y = 12;
			myAsteroid2.X = rand() % 8;
			while(myAsteroid1.X == myAsteroid2.X)
			{
				myAsteroid2.X = rand() % 8;
			}
			myAsteroid2.Y = 15;
			positionAsteroid();
			state = A_inGame;
		}
		else
		{
			state = A_Begin;
		}
		break;
		
		case A_inGame:
		if(gameStart == 1)
		{
			state = A_Begin;
		}
		else
		{
			state = A_inGame;
		}
		break;
		
		default:
		state = A_Begin;
		break;
	}
	
	switch (state){
		case A_Begin:
		break;
		
		case A_inGame:
		if(aCount <= 0)
		{
			x = (unsigned char) readadc(0);
			y = (unsigned char)readadc(1);
			if(x > 210)
			{
				if(myShip.rightLeg < 7)
				{
					shipMoved = 1;
					myShip.front++;
					myShip.rightLeg++;
					myShip.leftLeg++;
					resetShip();
					positionShip();
				}
				//right
			}
			else if (x < 40)
			{
				if(myShip.leftLeg > 0)
				{
					shipMoved = 1;
					myShip.front--;
					myShip.rightLeg--;
					myShip.leftLeg--;
					resetShip();
					positionShip();
				}
			}
			if(myAsteroid1.Y < -1)
			{
				if(asteroidsPassed <= 2)
				{
					myAsteroid1.Y = 50;
					myAsteroid1.X = 8;
					asteroidsPassed--;
				}
				else
				{
					myAsteroid1.Y = 10;
					myAsteroid1.X = rand() % 8;
					asteroidsPassed--;
				}
			}
			else
			{
				myAsteroid1.Y--;
			}
			
			if(myAsteroid2.Y < -1)
			{
				if(asteroidsPassed <= 1)
				{
					myAsteroid2.Y = 50;
					myAsteroid2.X = 8;
					asteroidsPassed--;
				}
				else
				{
					myAsteroid2.Y = 10;
					while(myAsteroid1.X == myAsteroid2.X)
					{
						myAsteroid2.X = rand() % 8;
					}
					asteroidsPassed--;
				}
			}
			else
			{
				myAsteroid2.Y--;
			}
			positionAsteroid();
			if(myAsteroid1.X == myShip.leftLeg)
			{
				if(myAsteroid1.Y == 0)
				{
					killed = 1;
					gameStart = 0;
				}
			}
			if(myAsteroid1.X == myShip.front)
			{
				if(myAsteroid1.Y == 1)
				{
					killed = 1;
					gameStart = 0;
				}
			}
			if(myAsteroid1.X == myShip.rightLeg)
			{
				if(myAsteroid1.Y == 0)
				{
					killed = 1;
					gameStart = 0;
				}
			}
			
			if(myAsteroid2.X == myShip.leftLeg)
			{
				if(myAsteroid2.Y == 0)
				{
					killed = 1;
					gameStart = 0;
				}
			}
			if(myAsteroid2.X == myShip.front)
			{
				if(myAsteroid2.Y == 1)
				{
					killed = 1;
					gameStart = 0;
				}
			}
			if(myAsteroid2.X == myShip.rightLeg)
			{
				if(myAsteroid2.Y == 0)
				{
					killed = 1;
					state = A_Begin;
					gameStart = 0;
				}
			}
			aCount = 100 - speed;//4
		}
		else
		{
			aCount--;
		}
		drawSpace();
		
		if(asteroidsPassed <= 0)
		{
			aCompleted = 1;
		}
		
		
		if(killed == 1)
		{
			transmit_data1(0xFF);
			transmit_data2(0);
		}
		break;
		
		default:
		break;
	}
	return state;
}

int M_tick(int state){
	switch (state) {
		case M_Begin:
		transmit_data1(0);
		transmit_data2(0);
		LCD_DisplayString(1,"Space Team!");
		startButtonPressed = ~PINA & 0x80;
		if((startButtonPressed == 0x80) && firstTime == 0)
		{
			gameTimer = 1000000;
			ogameTime = gameTimer;
			gameOn = 1;
			firstTime++;
			numInstruc = 3;
			onumInstruct = numInstruc;
			srand(seed);
			instruction = 0;
			keyPadIndex = 0;
			
			randomKeyCode();
			clearKeyCode();
			
			char longword[50] = "Enter code:";
			LCD_DisplayString(1,longword);
			
			LCD_Cursor(12);
			LCD_WriteData(randkeyPadCode[0] + '0');
			LCD_WriteData(randkeyPadCode[1] + '0');
			LCD_WriteData(randkeyPadCode[2] + '0');
			LCD_WriteData(randkeyPadCode[3] + '0');
			
			LCD_Cursor(1);
			LCD_WriteData(keyPadEntry[0] + '0');
			LCD_WriteData(keyPadEntry[1] + '0');
			LCD_WriteData(keyPadEntry[2] + '0');
			LCD_WriteData(keyPadEntry[3] + '0');
			
			LCD_Cursor(30);
			LCD_WriteData(numInstruc + '0');
			
			state = M_Sta;
		}
		else
		{
			state = M_Begin;
		}
		break;
		
		case M_Sta:
		if(gameTimer <= 0)
		{
			GAMEOVER = 1;
			gameOn = 0;
			state = M_gameOver;
		}
		else
		{
			if(numInstruc <=0)
			{
				instruction = 4;
				state = M_asteroid;
				gameStart = 1;
			}
			else
			{
				gameTimer--;
				state = M_Sta;
			}
		}
		break;
		
		case M_asteroid:
		LCD_DisplayString(1,"Dodge the asteroids!");
		LCD_Cursor(1);
		if(aCompleted == 1)
		{
			killedCnt--;
			if(killedCnt <= 0)
			{
				aCompleted = 0;
				multiplier++;
				gameOn = 1;
				gameStart = 1;
				instruction = 5;
				state = M_hyperDrive;
				break;
			}
		}
		
		if(killed == 1)
		{
			GAMEOVER = 1;
			gameOn = 0;
			state = M_gameOver;
		}
		else
		{
			state = M_asteroid;
		}
		break;
		
		case M_hyperDrive:
		LCD_DisplayString(1,"Hit the hyperdrive!");
		LCD_Cursor(1);
		
		if(1)//did not end up using this state
		{
			Hkilled= 0;
			hToggle = 0;
			hCompleted = 0;
			multiplier++;
			gameOn = 0;
			state = M_level;
			break;
		}
		
		if(hCompleted == 1)
		{
			Hkilled= 0;
			hToggle = 0;
				hCompleted = 0;
				multiplier++;
				gameOn = 0;
				state = M_level;
				break;
		}
		
		if(Hkilled == 1)
		{
			Hkilled= 0;
			hToggle = 0;
			hCompleted = 0;
			GAMEOVER = 1;
			gameOn = 0;
			state = M_gameOver;
		}
		else
		{
			state = M_hyperDrive;
		}
		break;
		
		case M_level:
		if(cnt <= 0)
		{
			cnt = 40;
			if(numIADD <= 5)
			{
				numIADD++;
			}
			numInstruc = onumInstruct + (rand() % numIADD);
			onumInstruct = numInstruc;
			uPoints++;
			state = M_Sta;
			if(speed >=50)
			{
				speed = speed - 10;
			}
			gameTimer = ogameTime + (rand() % gameTADD);
			ogameTime = gameTimer;
			gameOn = 1;
				instruction = rand() % 4;
				/*while(instruction == 1)
				{
					instruction = rand() % 4;
				}*/
				if(instruction == 0)
				{
					clearKeyCode();
					keyPadIndex = 0;
					randomKeyCode();
					char longword[50] = "Enter code:";
					LCD_DisplayString(1,longword);
					
					LCD_Cursor(12);
					LCD_WriteData(randkeyPadCode[0] + '0');
					LCD_WriteData(randkeyPadCode[1] + '0');
					LCD_WriteData(randkeyPadCode[2] + '0');
					LCD_WriteData(randkeyPadCode[3] + '0');
				}
				else if(instruction == 1)
				{
					ADC_init();
					wTick = 0;
					weightToggle = rand() % 2;
					if(weightToggle == 0)
					{
						char longword[50] = "Press lightly on the weight. ";
						LCD_DisplayString(1,longword);
					}
					else
					{
						char longword[50] = "Press hard on the weight.";
						LCD_DisplayString(1,longword);
					}
				}
				else if(instruction == 2)
				{
					InitADC();
					jTick = 0;
					directionToggle = rand() % 4;
					if(directionToggle == 0)
					{
						char longword[50] = "Steer Right.";
						LCD_DisplayString(1,longword);
					}
					else if(directionToggle == 1)
					{
						char longword[50] = "Steer Left.";
						LCD_DisplayString(1,longword);
					}
					else if(directionToggle == 2)
					{
						char longword[50] = "Steer Up.";
						LCD_DisplayString(1,longword);
					}
					else if(directionToggle == 3)
					{
						char longword[50] = "Steer Down.";
						LCD_DisplayString(1,longword);
					}
				}
				else if(instruction == 3)
				{
					ADC_init();
					rTick = 0;
					MAX = 100;
					randPosIndex = rand() % 3;
					if(randPosIndex == 0)
					{
						randPos = 0;
					}
					else if(randPosIndex == 1)
					{
						randPos = 5;
					}
					else if(randPosIndex == 2)
					{
						randPos = 10;
					}
					
					while(randPos == rotPos)
					{
						randPosIndex = rand() % 3;
						if(randPosIndex == 0)
						{
							randPos = 0;
						}
						else if(randPosIndex == 1)
						{
							randPos = 5;
						}
						else if(randPosIndex == 2)
						{
							randPos = 10;
						}
					}
					
					if(randPosIndex == 0)
					{
						char longword[50] = "Turn knob to 1.";
						LCD_DisplayString(1,longword);
					}
					else if(randPos == 5)
					{
						char longword[50] = "Turn knob to 2.";
						LCD_DisplayString(1,longword);
					}
					else if(randPos == 10)
					{
						char longword[50] = "Turn knob to 3.";
						LCD_DisplayString(1,longword);
					}
				}
		}
		else
		{
			if(cnt <= 20)
			{
				LCD_DisplayString(1,"LEVEL COMPLETE!");
			}
			cnt--;
			state = M_level;
		}
		break;
		
		case M_gameOver:
			LCD_DisplayString(1,"GAME OVER. Your score:");
			LCD_Cursor(28);
			LCD_WriteData(uPoints +'0');
			startButtonPressed = ~PINA & 0x80;
			if(startButtonPressed == 0x80)
			{
				uPoints = 0;
				seed = 0;
				multiplier = 1;
				GAMEOVER = 0;
				firstTime = 0;
				killed = 0;
				int i  = 1000;
				while(i >= 0)
				{
					i--;
					while(!TimerFlag);
					TimerFlag = 0;
				}
				state = M_Begin;
				break;
			}
			state = M_gameOver;
		break;
		
		default:
		state = M_Begin;
		break;
	}
	
	switch (state) 
	{
		case M_Begin:
		break;
		
		case M_Sta:
		if(instruction == 0) // THIS IS FOR THE KEYPAD INSTRUCTION
		{
			if((keyPadEntry[0] == randkeyPadCode[0])&&(keyPadEntry[1] == randkeyPadCode[1])&&(keyPadEntry[2] == randkeyPadCode[2])&&(keyPadEntry[3] == randkeyPadCode[3]))
			{
				iCompleted = 1;
			}
			else if(keyPressed == '*')
			{
				clearKeyCode();
				keyPadIndex = 0;
			}
			else if (keyPressed == '\0')
			{
			}
			else
			{
				if(keyPressed == '0')
				{
					keyPadEntry[keyPadIndex] = 0;
				}
				else if(keyPressed == '1')
				{
					keyPadEntry[keyPadIndex] = 1;
				}
				else if(keyPressed == '2')
				{
					keyPadEntry[keyPadIndex] = 2;
				}
				else if(keyPressed == '3')
				{
					keyPadEntry[keyPadIndex] = 3;
				}
				else if(keyPressed == '4')
				{
					keyPadEntry[keyPadIndex] = 4;
				}
				else if(keyPressed == '5')
				{
					keyPadEntry[keyPadIndex] = 5;
				}
				else if(keyPressed == '6')
				{
					keyPadEntry[keyPadIndex] = 6;
				}
				else if(keyPressed == '7')
				{
					keyPadEntry[keyPadIndex] = 7;
				}
				else if(keyPressed == '8')
				{
					keyPadEntry[keyPadIndex] = 8;
				}
				else if(keyPressed == '9')
				{
					keyPadEntry[keyPadIndex] = 9;
				}
				if(keyPadIndex < 3)
				{
					keyPadIndex++;
				}
				else
				{
					keyPadIndex = 3;
				}
			}
		}
		else if(instruction == 1) // THIS IS FOR THE WEIGHT SENSOR INSTRUCTION
		{
			if((currW < fullW) && (weightToggle == 0))
			{
				wTick++;
				if((wTick >= 5))
				{
					iCompleted = 1;
				}
			}
			else if((currW < halfW)&& (weightToggle == 1) && (currW >= fullW))
			{
				wTick++;
				if(wTick >= 5)
				{
					iCompleted = 1;
				}
			}
			else
			{
				wTick = 0;
			}
		}
		else if(instruction == 2) // THIS IS FOR THE DIRECTION INSTRUCTION
		{
			if(directionToggle == direction)
			{
				jTick++;
				if(jTick >= 5)
				{
					iCompleted = 1;
				}
			}
			else
			{
				jTick = 0;
			}
		}
		else if(instruction == 3) // THIS IS FOR THE ROTARY INSTRUCTION
		{
			if(rotPos == randPos)
			{
				rTick++;
					iCompleted = 1;
			}
			else
			{
				rTick = 0;
			}
		}
		
		
		if(iCompleted == 1) // if instruction was completed
		{
			numInstruc--;
			iCompleted = 0;
			instruction = rand() % 4;
			/*while(instruction == 1)
			{
				instruction = rand() % 4;
			}*/
			if(instruction == 0)
			{
				clearKeyCode();
				keyPadIndex = 0;
				randomKeyCode();
				char longword[50] = "Enter code:";
				LCD_DisplayString(1,longword);
				
				LCD_Cursor(12);
				LCD_WriteData(randkeyPadCode[0] + '0');
				LCD_WriteData(randkeyPadCode[1] + '0');
				LCD_WriteData(randkeyPadCode[2] + '0');
				LCD_WriteData(randkeyPadCode[3] + '0');
				
			}
			else if(instruction == 1)
			{
				ADC_init();
				wTick = 0;
				weightToggle = rand() % 2;
				if(weightToggle == 0)
				{
					char longword[50] = "Press acceleration hard. ";
					LCD_DisplayString(1,longword);
				}
				else
				{
					char longword[50] = "Press acceleration moderately.";
					LCD_DisplayString(1,longword);
				}
			}
			else if(instruction == 2)
			{
				InitADC();
				jTick = 0;
				directionToggle = rand() % 4;
				if(directionToggle == 0)
				{
					char longword[50] = "Steer Right.";
					LCD_DisplayString(1,longword);
				}
				else if(directionToggle == 1)
				{
					char longword[50] = "Steer Left.";
					LCD_DisplayString(1,longword);
				}
				else if(directionToggle == 2)
				{
					char longword[50] = "Steer Up.";
					LCD_DisplayString(1,longword);
				}
				else if(directionToggle == 3)
				{
					char longword[50] = "Steer Down.";
					LCD_DisplayString(1,longword);
				}
			}
			else if(instruction == 3)
			{
				ADC_init();
				rTick = 0;
				MAX = 100;
				randPosIndex = rand() % 3;
				if(randPosIndex == 0)
				{
					randPos = 0;
				}
				else if(randPosIndex == 1)
				{
					randPos = 5;
				}
				else if(randPosIndex == 2)
				{
					randPos = 10;
				}
				
				while(randPos == rotPos)
				{
					randPosIndex = rand() % 3;
					if(randPosIndex == 0)
					{
						randPos = 0;
					}
					else if(randPosIndex == 1)
					{
						randPos = 5;
					}
					else if(randPosIndex == 2)
					{
						randPos = 10;
					}
				}
				
				if(randPosIndex == 0)
				{
					char longword[50] = "Shield generator to 1.";
					LCD_DisplayString(1,longword);
				}
				else if(randPos == 5)
				{
					char longword[50] = "Shield generator to 2.";
					LCD_DisplayString(1,longword);
				}
				else if(randPos == 10)
				{
					char longword[50] = "Shield generator to 3.";
					LCD_DisplayString(1,longword);
				}
			}
		}
		else
		{
			
			if(instruction == 0)
			{
				char longword[50] = "Enter code:";
				LCD_DisplayString(1,longword);
				
				LCD_Cursor(12);
				LCD_WriteData(randkeyPadCode[0] + '0');
				LCD_WriteData(randkeyPadCode[1] + '0');
				LCD_WriteData(randkeyPadCode[2] + '0');
				LCD_WriteData(randkeyPadCode[3] + '0');
			}
			else if(instruction == 1)
			{
				if(weightToggle == 0)
				{
					char longword[50] = "Press acceleration hard";
					LCD_DisplayString(1,longword);
				}
				else
				{
					char longword[50] = "Press acceleration moderately";
					LCD_DisplayString(1,longword);
				}
			}
			else if(instruction == 2)
			{
				if(directionToggle == 0)
				{
					char longword[50] = "Steer Right.";
					LCD_DisplayString(1,longword);
				}
				else if(directionToggle == 1)
				{
					char longword[50] = "Steer Left.";
					LCD_DisplayString(1,longword);
				}
				else if(directionToggle == 2)
				{
					char longword[50] = "Steer Up.";
					LCD_DisplayString(1,longword);
				}
				else if(directionToggle == 3)
				{
					char longword[50] = "Steer Down.";
					LCD_DisplayString(1,longword);
				}
			}
			else if(instruction == 3)
			{
				if(randPos == 0)
				{
					char longword[50] = "Shield generator to 1.";
					LCD_DisplayString(1,longword);
				}
				else if(randPos == 5)
				{
					char longword[50] = "Shield generator to 2.";
					LCD_DisplayString(1,longword);
				}
				else if(randPos == 10)
				{
					char longword[50] = "Shield generator to 3.";
					LCD_DisplayString(1,longword);
				}
			}
		}
		break;
		
		case M_asteroid:
		break;
		
		default:
		break;
	}
	return state;
}

int main(void)
{
	DDRA = 0x0C; PORTA = 0xF3;
	DDRC = 0xF0; PORTC = 0x0F;
	DDRD = 0xFF; PORTD = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	
	static task master,keyPad,weight,joyStick,rotary,asteroidG,hyperDrive;
	task *tasks[] = {&master,&keyPad,&weight,&joyStick,&rotary,&asteroidG,&hyperDrive};
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);
	
	master.state = M_Begin;
	master.period = 100;
	master.elapsedTime = 100;
	master.TaskFct = &M_tick;
	
	keyPad.state = K_Begin;
	keyPad.period = 1;
	keyPad.elapsedTime = 1;
	keyPad.TaskFct = &K_tick;
	
	weight.state = W_Begin;
	weight.period = 100;
	weight.elapsedTime = 100;
	weight.TaskFct = &W_tick;
	
	joyStick.state = J_Begin;
	joyStick.period = 100;
	joyStick.elapsedTime = 100;
	joyStick.TaskFct = &J_tick;
	
	rotary.state = R_Begin;
	rotary.period = 1;
	rotary.elapsedTime = 1;
	rotary.TaskFct = &R_tick;
	
	asteroidG.state = A_Begin;
	asteroidG.period = 1;
	asteroidG.elapsedTime = 1;
	asteroidG.TaskFct = &A_tick;
	
	hyperDrive.state = H_Begin;
	hyperDrive.period = 1;
	hyperDrive.elapsedTime = 1;
	hyperDrive.TaskFct = &H_tick;
	
	ADC_init();
	LCD_init();
	TimerSet(1);
	TimerOn();
	
	unsigned short i = 0;
    while (1)
    {	
		
		if(gameOn == 1)
		{
			//this is the other sm being run
			if (tasks[instruction + 1]->elapsedTime == tasks[instruction + 1]->period)
			{
				tasks[instruction + 1]->state = tasks[instruction + 1]->TaskFct(tasks[instruction + 1]->state);
				tasks[instruction + 1]->elapsedTime = 0;
			}
			tasks[instruction + 1]->elapsedTime += 1;
		}
		else
		{
			seed++;
		}
		
		if (tasks[0]->elapsedTime == tasks[0]->period)
		{
			tasks[0]->state = tasks[0]->TaskFct(tasks[0]->state);
			tasks[0]->elapsedTime = 0;
		}
		tasks[0]->elapsedTime += 1;
		
		if(gameOn == 1)
		{
			LCD_Cursor(30);
			LCD_WriteData(numInstruc + '0');
		}
		
		while(!TimerFlag);
		TimerFlag = 0;
    }
}
