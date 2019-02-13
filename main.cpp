#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

/*
#include <wiringPi.h>
#include <wiringShift.h>
*/

int DATAS = 85;	//Global variables = pure evil !
int FPS = 0;
std::vector<unsigned int> SIZE {8, 8};
std::vector<unsigned int> PINS {7, 0, 2, 10};	
/*PINS:
	- Datas pin
	- shift pin
	- memory pin
	- allow output
*/



int testLed(int const red, int const blue, int const green) {
	/*
	digitalWrite(red, HIGH);
	digitalWrite(green, HIGH);
	digitalWrite(blue, HIGH);
	delayMicroseconds(1);
	digitalWrite(red, LOW);
	digitalWrite(green, LOW);
	digitalWrite(blue, LOW);
	delayMicroseconds(100);
	*/
	return EXIT_SUCCESS;
}



int askUser() {
	int number;
	std::cout<<"\nEnter a number: ";
	std::cin>> number;
	return number;
}



int sendPackets(int const nb) {
	/*
	for (int i(0); i<nb; i++) {
		shiftOut(2, 7, MSBFIRST, DATAS);
	}
	*/
	return EXIT_SUCCESS;
}



int drawScreen(int const nbCycles) {
	/* One frame composed of several cycles of PWM */

	int nbCells = SIZE[0]*SIZE[1];
	for (int i(0); i<nbCycles; i++) {
		sendPackets(nbCells*4);
	}
	return EXIT_SUCCESS;
}


/*
PI_THREAD(deamonLED){
	int nbScreens = 0;	//Used to count fps
	int dTime = 0;
	int prevTime = 0;

	while(1==1){
		dTime = 0;	//Reset of deltaTime
		nbScreens = 0;
		prevTime = millis();
		while(dTime<=1000){	//Prints nb of screens displayed
					//	after one second
			nbScreens +=1;
			drawScreen(10);
			dTime = millis()-prevTime;
		}
		FPS = nbScreens;	//Updates FPS variable
	}
}
*/


int main(){
/*
	if (wiringPiSetup()==-1){
		std::cout<<"Thread initialisation failed"<<std::endl;
		return EXIT_FAILURE;
	}

	int Width (0);
	int Height (0);
	int Bpp = (0);

	int red = (7);
	int blue = (0);
	int green = (2);

	int x = piThreadCreate(deamonLED);
	if (x!=0){
		std::cout<<"Thread didnt start"<<std::endl;
	}
*/	

	return EXIT_SUCCESS;
}
