/**************************************************************************************************
		Platform: Fire Bird V P89V51RD2
		Title: Adaptive Highbeam Assist
		Author: E-Yantra Team, ERTS Lab, IIT Bombay.
		Compiled with: uVision3 V3.90; C Compiler: C51.Exe, V8.18
**************************************************************************************************/

/********************************************************************************

   Copyright (c) 2010, ERTS Lab, IIT Bombay.                       -*- c -*-
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

   * Source code can be used for academic purpose. 
	 For commercial use permission form the author needs to be taken.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. 

  Software released under Creative Commence cc by-nc-sa licence.
  For legal information refer to: 
  http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode

********************************************************************************/
#include <intrins.h>
#include "p89v51rx2.h"
#include "lcd_array.h"

//direction control bits

sbit LB=P1^0;  //LEFT BACK Pin
sbit LF=P1^1;  //LEFT FRONT Pin
sbit RF=P1^2;  //RIGHT FRONT Pin
sbit RB=P3^4;  //RIGHT BACK Pin

sbit start_conv = P3^6;		 //P3^6 is connected to Start and ALE pin of ADC0808
sbit output_enable = P3^7;		 //P3^7 is connected to Output Enable Pin of ADC0808

sbit upper_light = P1^7;
sbit lower_light = P1^6;


unsigned char data_array[8];//initialize an array of 8 elements to display onto the LCD screen
unsigned char i=0;  //used in main function during ADC conversion 
unsigned char left_whiteline = 0;
unsigned char center_whiteline = 0;
unsigned char right_whiteline = 0;
unsigned char flag = 0; //used in white line following logic

unsigned char front_sharp_sensor = 0; //used in stop n go ACC
unsigned int reqd_count=20;

/********************************************************************************
	Function: ADC_conversion(unsigned char ADC_channel_number)

	Description: This function performs ADC conversion of a channel number 
				 that is passed to it. The return value of this function contains 
 				 digital data of the selected channel.
		   
				 Analog Sensor Pin Mapping
  
            	 IN0-----> Optional
				 IN1-----> Batt. Voltage
				 IN2-----> Front Sharp IR Range sensor
				 IN3-----> Left Proximity
				 IN4-----> Left Whiteline
		 		 IN5-----> Center Whiteline
				 IN6-----> Right Whiteline
				 IN7-----> Right Proximity
********************************************************************************/
unsigned char ADC_conversion(unsigned char ADC_ch_no)
{
unsigned char ADC_data;

P0 = ADC_ch_no;		  // to select channel no. send address on P0

start_conv = 1; // ADC0808 will latch the address on L-H transition on the ALE pin
start_conv = 0;	// ADC0808 will start conversion on H-L transition on the start pin
delay_ms(2);	// conversion time for ADC0808

P0 =0xFF;	        // output enable and read
output_enable = 1;	//Enabling o/p of ADC0808 for to read the converted data
ADC_data = P0;		//storing adc_data
output_enable = 0;	//disabling ADC 0808 o/p
return ADC_data;	//returning adc_data
}

//Set left motor with velocity vel(compare value). 0x00 will give full (100% duty cycle) velocity, while 0xFF will give zero (0% duty cycle) velocity. Any value in between 0x00 and 0xFF will give intermediate velocity.
void left_motor_velocity(unsigned char vel)
{
CCAP0H=vel;
}

//Set right motor with velocity vel(compare value). 0x00 will give full (100% duty cycle) velocity, while 0xFF will give zero (0% duty cycle) velocity. Any value in between 0x00 and 0xFF will give intermediate velocity.
void right_motor_velocity(unsigned char vel)
{
CCAP1H=vel;
}

// motor direction control subroutines
void forward(void)
{
RF=1;
LF=1;
RB=0;
LB=0;
}

void stop(void)
{
RF=0;
LF=0;
RB=0;
LB=0;
}

//initialise programmable counter array module to generate pwm
//The CCAPnH register holds the reload value. CCAPnL register increments,and is loaded with this value everytime it overflows
//Thus the value in CCAPnH can be changed to change the motor velocity
void pca_init(void)
{
 CMOD=0x80;	   //80 sets PCA counter to run at Fosc/6; 
 CCON=0x40;	   //start PCA counter
 						
 CCAP0L=0x00;   
 CCAP0H=0xFF;  //Left motor duty cycle register
  
 CCAP1L=0x00;				  
 CCAP1H=0xFF;  //Right motor duty cycle register

 CCAPM0=0x42;  //enable PWM mode and ECOM bits for left motor
 CCAPM1=0x42;  //enable PWM mode and ECOM bits for right motor
}

void linefollower()
{
   unsigned int count=0;
   left_motor_velocity(0x4F);  //left and right motor is at same velocity
   right_motor_velocity(0x4F);
   forward();


  while(count<reqd_count)
  {
   for(i=0;i<8;i++)	 //Doing ADC conversion
   {
   data_array[i] = ADC_conversion(i);
   }

  left_whiteline = data_array[4];   
  center_whiteline = data_array[5];
  right_whiteline = data_array[6];
  front_sharp_sensor = data_array[2];

  flag = 0; //reset the flag to 0
     
   //check if robot's center white line sensor is on the white line
  if(center_whiteline < 25)
  {
   flag = 1;  //set the flag to 0 so that further white line sensor comparision is disabled
   left_motor_velocity(0x4F);  //left and right motor is at same velocity
   right_motor_velocity(0x4F);
  }

  //robot is drifting towards left side, increase velocity of the left wheel and decrease velocity of the right wheel
  if ((left_whiteline > 25) && (flag == 0))
  {
   flag = 1;  //set the flag to 0 so that further white line sensor comparision is disabled
   left_motor_velocity(0x3F);   //increase left motor velocity
   right_motor_velocity(0x5F);	//decrease  right motor velocity
  }

  //robot is drifting towards right side, decrease velocity of the left wheel and increase velocity of the right wheel
  if ((right_whiteline > 25) && (flag == 0))
  {
   flag = 1;  //set the flag to 0 so that further white line sensor comparision is disabled
   left_motor_velocity(0x5F);	//decrease  right motor velocity
   right_motor_velocity(0x3F);  //increase left motor velocity
  }
  
  if((left_whiteline < 25) && (center_whiteline < 25) && (right_whiteline < 25)) // no whiteline is detected, stop (buzzer will not turn on)
  {
   forward();
  }

  LCD_PRINT(data_array);//call this function to print the array onto the screen
  delay_ms(250);
 
  count++;
 }

}

void main()
{
 output_enable = 0;
 start_conv = 0;    //de-assert all control signals to ADC
 upper_light = 0;
 lower_light = 1;


 buzzer = 1;  //buzzer off
 LCD_INIT(); //Initialize LCD
 pca_init(); //Initialize PCA to genarate PWM

 forward();
 left_motor_velocity(0x4F);  //0x00 will give full (100% duty cycle) velocity, while 0xFF will give zero (0% duty cycle) velocity. Any value in between 0x00 and 0xFF will give intermediate velocity.			
 right_motor_velocity(0x4F); //0x00 will give full (100% duty cycle) velocity, while 0xFF will give zero (0% duty cycle) velocity. Any value in between 0x00 and 0xFF will give intermediate velocity.


 while(1)
 {
  for(i=0;i<8;i++)	 //Doing ADC conversion
  {
  data_array[i] = ADC_conversion(i);
  }

  left_whiteline = data_array[4];   
  center_whiteline = data_array[5];
  right_whiteline = data_array[6];
  front_sharp_sensor = data_array[2];

  flag = 0; //reset the flag to 0
     
   //check if robot's center white line sensor is on the white line
  if(center_whiteline < 25)
  {
   flag = 1;  //set the flag to 0 so that further white line sensor comparision is disabled
   left_motor_velocity(0x4F);  //left and right motor is at same velocity
   right_motor_velocity(0x4F);
  }

  //robot is drifting towards left side, increase velocity of the left wheel and decrease velocity of the right wheel
  if ((left_whiteline > 25) && (flag == 0))
  {
   flag = 1;  //set the flag to 0 so that further white line sensor comparision is disabled
   left_motor_velocity(0x3F);   //increase left motor velocity
   right_motor_velocity(0x5F);	//decrease  right motor velocity
  }

  //robot is drifting towards right side, decrease velocity of the left wheel and increase velocity of the right wheel
  if ((right_whiteline > 25) && (flag == 0))
  {
   flag = 1;  //set the flag to 0 so that further white line sensor comparision is disabled
   left_motor_velocity(0x5F);	//decrease  right motor velocity
   right_motor_velocity(0x3F);  //increase left motor velocity
  }

  if(front_sharp_sensor >= 30) // obstacle is near the robot, stop and turn on the buzzer
  {
   buzzer = 0;  //buzzer on
   upper_light = 1;
   lower_light = 0;
   reqd_count=0;
  }
  
  else
  {
   buzzer = 1;  //buzzer off
   if(reqd_count<8)
   {
    upper_light = 1;
    lower_light = 0;
   }
   else
   {
    upper_light = 0;
    lower_light = 1;
   }
   reqd_count++;
  }

  if((left_whiteline > 25) && (center_whiteline > 25) && (right_whiteline > 25)) // no whiteline is detected, stop (buzzer will not turn on)
  {
   stop();
  }
  
  LCD_PRINT(data_array);//call this function to print the array onto the screen
  delay_ms(250);
 }
}