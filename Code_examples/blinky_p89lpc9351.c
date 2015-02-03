#include <p89lpc9351.h>

void delay(void)
{
    int j, k;
    for(j=0; j<100; j++)
    {
        for(k=0; k<1000; k++);
    }
}

void main(void)
{
	P2M1=0;
	P2M2=0;
	
    while(1)
    {
        P2_1=0;
        delay();
        P2_1=1;
        delay();
    }
}
