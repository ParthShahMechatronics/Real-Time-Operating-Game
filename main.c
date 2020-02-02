/*****************
MTE 241 - LAB 5 - CATAPULT
PHSHAH & SMTAMBOL
THURSDAY GROUP 46
*****************/

// Required imports

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <lpc17xx.h>
#include <time.h>
#include "cmsis_os2.h"
#include "glcd_scroll.h"
#include "uart.h"
#include "stdbool.h"
#include "GLCD.h"
#include "catapult.c"


#define BG White
#define FG Blue

//Global variables 
int life = 0;
int score_old = 0;
int score = 0;
bool miss = 0;
bool hit = 0;
int power = 0, y_bullet = 120, x_bullet = 20, x_sling = 1, x_target = 133, y_target = 95;
bool up = 0;
bool down = 0;
bool shoot = 0;
char *Power = "Power: ", *Score = "Score: ";

// helper function to randomly generate coordinates
int randomCoord(int lBound, int uBound) {
    return (rand() % (uBound-lBound+1))+lBound;
}

//User input thread
void inputs(void *arg) {
	
	// potentiometer

	LPC_PINCON -> PINSEL1 &= ~(0x03 << 18);
	LPC_PINCON -> PINSEL1 |= (0x01 << 18);
	LPC_SC -> PCONP |= (1 << 12);
	LPC_ADC -> ADCR = (1 << 2) | // select AD0.2 pin
				            (4 << 8) | // ADC clock is 25MHz/5
				        	  (1 << 21); // enable ADC
	while(1){
	//get code
		LPC_ADC -> ADCR |= (1 << 24);
		while(!(LPC_ADC -> ADGDR & (1U << 31))) {}
		power = LPC_ADC -> ADGDR & 0xFFF0 >> 4;
		up = 0;
		down = 0;
		shoot = 0;
		
		// Joystick
		if(	!(LPC_GPIO1->FIOPIN & (1<<23))){
			up =1;
		}
		if(!(LPC_GPIO1->FIOPIN &(1<<25))){
			down = 1;
		}
		//pushbutton 
		if (!(LPC_GPIO2 -> FIOPIN & (1 << 10))){
			shoot = 1;
		}	
	}
		osThreadYield();
}

//graphics thread 
void graphics(void *arg) {

	char pwr[3];
	char scr[2];
	
	while(life<3) {
		// Power and Score Texts
		GLCD_DisplayString(1, 1, 0, (unsigned char *)Power);
		GLCD_DisplayString(1, 40, 0, (unsigned char *)Score);

		//Configuration of force
		sprintf(pwr, "%d", power/80 );
		GLCD_DisplayString(1, 8, 0, (unsigned char *)pwr);
		sprintf(scr, "%d", score );
		GLCD_DisplayString(1, 48, 0, (unsigned char *)scr);	
			
		//initial graphics
		GLCD_Bitmap(x_sling, y_bullet, 16,20, (unsigned char*) sling);
		GLCD_Bitmap(x_bullet, y_bullet, 10, 14, (unsigned char *) bullet);
		GLCD_Bitmap(x_target, y_target, 20, 20, (unsigned char *) target);

		
		//to move up and down
		if (up == 1 && y_bullet >25) {
			y_bullet -= 1;
			GLCD_Bitmap(x_sling, y_bullet, 16,20, (unsigned char*) sling);
			GLCD_Bitmap(x_bullet, y_bullet, 10, 14, (unsigned char *) bullet);
		}
		else if (down == 1 && y_bullet <215 ) {
			y_bullet += 1;
			GLCD_Bitmap(x_sling, y_bullet, 16, 20, (unsigned char*) sling);
			GLCD_Bitmap(x_bullet, y_bullet, 10, 14, (unsigned char *) bullet);
		}
		
		if(shoot == 1) {
		  
			while(x_bullet<300 && !hit) {
				GLCD_Bitmap(x_bullet, y_bullet, 10, 14, (unsigned char *) bullet);
				x_bullet++;
				
				if(x_bullet == x_target && ((y_bullet-3<=y_target+5 && y_bullet-3 >= y_target-5) || (y_bullet+3>= y_target-5 && y_bullet +3 <= y_target+5))){
					score++;
					hit = 1;
				}
			}
			// if you miss
			if(score_old == score) {
				miss = 1;
			}
			else {
				score_old = score;
			}
			// clearing screen
			GLCD_Clear(White);
			// resetting positions of sling and bullet for next iteration
			// resetting hit for next iteration
			y_bullet = 120;
			x_bullet = 20;
			x_target = randomCoord(35+life,300-life);
			y_target = randomCoord(25+life,215-life);
			hit = 0;
		}
		
		osThreadYield();
	}
	GLCD_Clear(White);
	GLCD_DisplayString(4,6,1, (unsigned char *)"GAME OVER!");
}

//LED Thread
void LED(void *arg) {

	while(1) {
		//setting the required LEDs
		LPC_GPIO2->FIOSET |= 0x00000040; 
		LPC_GPIO2->FIOSET |= 0x00000020;
		LPC_GPIO2->FIOSET |= 0x00000010;
	

		//programming the LEDs to turn one led off each time a life is lost
		while(1){
			if(miss == 1){
				if(life == 0){
					LPC_GPIO2->FIOCLR |= 0x00000040;
					miss = 0;
				}
				if(life == 1){
					LPC_GPIO2->FIOCLR |= 0x00000020;
					miss = 0;
				}
				if(life == 2){
					LPC_GPIO2->FIOCLR |= 0x00000010;
				}
				//else return error
				life++;
			}
		}
	}
	osThreadYield();
}



int main(void){
	printf("GROUP TH46\n");
	
	
	//Game Startup/Instructions
	GLCD_Init();
	GLCD_Clear(Blue);
	GLCD_SetBackColor(White);
	GLCD_SetTextColor(Black);
	GLCD_DisplayString(2, 2, 1, (unsigned char *) "Catapult");
	GLCD_DisplayString(10, 5, 0, (unsigned char *)  "Up and Down joystick moves the Slingshot");
	GLCD_DisplayString(12, 5, 0, (unsigned char *)  "Use pushbutton to shoot");
	GLCD_DisplayString(14, 5, 0, (unsigned char *)  "The LEDs Display your lives left");
	GLCD_DisplayString(25, 5, 0, (unsigned char *)  "Press push button to start!");	
	
	while ((LPC_GPIO2 -> FIOPIN & (1 << 10))) {};
	GLCD_Clear(White);
		
	osKernelInitialize(); // initializing kernel
	LPC_GPIO1->FIODIR |= 0xB0000000;
	LPC_GPIO2->FIODIR |= 0x0000007C;
	
	// Thread Execution
	osThreadNew(graphics, NULL, NULL);
	osThreadNew(inputs, NULL, NULL);
	osThreadNew(LED, NULL, NULL);
	osKernelStart();
}
