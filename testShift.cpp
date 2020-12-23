#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <ncurses.h>
#include <wiringPi.h>


#define DATAS_KEY	0

//Global variables = pure evil !


std::vector<int> PINS {0, 2, 3};
/*PINS:
	- Datas pin
	- clock pin
	- latch pin
*/


std::vector<int> test(32);




int resetPins() {	//All output pins at LOW level
	for (unsigned int pin : PINS) {
		digitalWrite(pin, LOW);
	}
	return EXIT_SUCCESS;
}



int sendPacket(std::vector<int> & rawDatas) {
/* PWM ratio are stored with values V between 0 and 3.
	When a value V is not 0, it sets on an output, and this value is decreased.
	Therefore, after V passes in the loop, the output is turned off, simulating PWM */

	resetPins();
	unsigned int size = rawDatas.size();

	for (unsigned int i(0); i<size; i++) {	//LSB First, so datas are sent from the end of the table...

		if(rawDatas[size-i-1]!=0){	//If (last-i) bit != 0, sent it
                digitalWrite(PINS[0], HIGH);
				rawDatas[size-i-1] = rawDatas[size-i-1]-1;	//Decrease the value, as explained in description of the function
		}

        digitalWrite(PINS[1], HIGH);    //Clock up...
        delayMicroseconds(1);           //...delay... (1us by default)
        digitalWrite(PINS[1], LOW);     //...Clock down.

        digitalWrite(PINS[0], LOW);     //Turn off the data line

	}

    digitalWrite(PINS[2], HIGH);	//Outputs transmission, and draw dat line !
    delayMicroseconds(1);
    digitalWrite(PINS[2], LOW);

	return EXIT_SUCCESS;
}


int drawScreen() {
	/* One frame composed of several (8) cycles of PWM */
    test[0]=7;  //Rows
    test[1]=6;
    test[2]=5;
    test[3]=4;
    test[4]=3;
    test[5]=2;
    test[6]=1;
    test[7]=0;

    test[8]=7;  //RED
    test[9]=6;
    test[10]=5;
    test[11]=4;
    test[12]=3;
    test[13]=2;
    test[14]=1;
    test[15]=0;

    test[16]=0; //GREEN
    test[17]=1;
    test[18]=2;
    test[19]=3;
    test[20]=4;
    test[21]=5;
    test[22]=6;
    test[23]=7;

    test[24]=4; //BLUE
    test[25]=1;
    test[26]=4;
    test[27]=1;
    test[28]=4;
    test[29]=1;
    test[30]=4;
    test[31]=1;

    for (unsigned int i(0); i<8; i++){  //Draws 8 frames to allow for PWM.
        sendPacket(test);
        delayMicroseconds(100);
        //delay(100);
    }
	return EXIT_SUCCESS;
}




int main(){

    if (wiringPiSetup()==-1){	//INIT of wiringPi
		std::cout<<"Thread initialisation failed"<<std::endl;
		return EXIT_FAILURE;
	}

	for (unsigned int pin: PINS){	//Enables outputs
		pinMode(pin, OUTPUT);
	}

    	std::cout<<"hello"<<std::endl;


    while (true){

        drawScreen();
        //delay(150); //60 fps <=> T=165 ms
        //delay(1);
        
    }

    return EXIT_SUCCESS;

}