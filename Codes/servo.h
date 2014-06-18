float tmperiod;

/* Initialize the PCA and PWM mode */
/* Initialise the time period */

void SERVO_INIT(float timeperiod)
{
	unsigned char inner, outer;
	unsigned char a,b;
	float a1=timeperiod/256;
	tmperiod=timeperiod;
	a1=a1*1000;
	a1=a1/1.085;
	a=a1;
	b=256-a;

/* Initial Timer0 Generate Overflow PCA */
	
	TMOD = 0x02;  
	TH0  = b;     
	TR0=1;  


    CMOD=0x04;
    CCAPM0=0x42;
    CCAPM1=0x42;
    CCAPM2=0x42;
    CCAPM3=0x42;
    CCAPM4=0x42;
    CCAP0H=0x00;
    CCAP1H=0x00;
    CCAP2H=0x00;
    CCAP3H=0x00;
    CCAP4H=0x00;
    CCON=0x40;

    IEN0 = 0x00;
    P1 = 0xFF; /* Motor STOP */

    for (outer = 0x00; outer < 0x10; outer++) 
    { 
        for (inner = 0x00; inner < 0xFF; inner++);
    }


    IEN0 = 0x80; /* Start interrupt */
}
void RUN_SERVO(unsigned char pin, float rot_angle) //enter pulse width in degrees:- 0degrees to 180degrees
{
	float r1value, pulsewidth_tm;
	unsigned char rvalue;
	pulsewidth_tm=1+(rot_angle/180);
	r1value=256/tmperiod;
	r1value=r1value*pulsewidth_tm;
	rvalue=r1value;
	switch(pin)
	{
		case 0:	CCAP0H=-rvalue;
		           	break;
		case 1:	CCAP1H=-rvalue;
		           	break;
		case 2:	CCAP2H=-rvalue;
		           	break;
		case 3:	CCAP3H=-rvalue;
		           	break;
		case 4:	CCAP4H=-rvalue;
		           	break;
	}
}
