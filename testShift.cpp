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

std::vector<int> test(8);




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
        delayMicroseconds(1);           //...delay...
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
    test[0]=7;
    test[1]=2;
    test[2]=1;
    test[3]=0;
    test[4]=0;
    test[5]=1;
    test[6]=2;
    test[7]=7;

    for (unsigned int i(0); i<8; i++){
        sendPacket(test);
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
        
    }

    return EXIT_SUCCESS;

}