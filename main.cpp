#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

/*
#include <wiringPi.h>
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


int askUser();
int sendPacket(std::vector<bool> const& rawDatas);
int resetPins();
int drawScreen(int const nbCycles);




int askUser() {
	int number;
	std::cout<<"\nEnter a number: ";
	std::cin>> number;
	return number;
}



int sendPacket(std::vector<bool> const& rawDatas) {
	resetPins();
	for (bool isOn: rawDatas) {
		if (isOn){	digitalWrite(PINS[0], HIGH);	}else{ digitalWrite(PINS[0], LOW);	}
		digitalWrite(PINS[1], HIGH);	//Register transmission

		digitalWrite(PINS[0], LOW);	//Reset of data pin
		digitalWrite(PINS[1], LOW);	//Reset of shift pin
	}

	digitalWrite(PINS[2], HIGH);	//Outputs transmission
	digitalWrite(PINS[2], LOW);
	return EXIT_SUCCESS;
}



int resetPins(){	//All output pins at LOW level
	for (unsigned int pin: PINS){
		digitalWrite(pin, LOW);
	}
	return EXIT_SUCCESS
}



int drawScreen(int const nbCycles) {
	/* One frame composed of several cycles of PWM */

	std::vector<bool> rawDATAS {true, false, false, false, true, true, false, false, false};

	//int nbCells = SIZE[0]*SIZE[1];
	
	for (int i(0); i<nbCycles; i++) {
		sendPacket(rawDATAS);
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


	int x = piThreadCreate(deamonLED);
	if (x!=0){
		std::cout<<"Thread didnt start"<<std::endl;
	}
*/

	return EXIT_SUCCESS;
}
