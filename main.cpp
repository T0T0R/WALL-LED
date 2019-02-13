#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <wiringPi.h>

#define DATAS_KEY	0

//Global variables = pure evil !
std::vector<std::vector<std::vector<int>>> DATAS {};
int FPS = 0;
std::vector<int> SIZE {4, 4};
std::vector<int> PINS {0, 2, 3};
/*PINS:
	- Datas pin
	- shift pin
	- memory pin
*/
std::vector<int> RED_VALUES {64, 128, 192, 255};
std::vector<int> GREEN_VALUES {64, 128, 192, 255};
std::vector<int> BLUE_VALUES {64, 128, 192, 255};



/******** PROTOTYPES ********/
int askUser();
int sendPacket(std::vector<int> & rawDatas);
int resetPins();
int drawScreen();
std::vector<int> convertPixelBW(std::vector<int> const& pixel);
std::vector<int> convertImageToLED();
int convertValuePWM(int const& value, int const& color);
void initDATAS();
int calibrate();






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
			drawScreen();	//Draw a frame in 8 PWM cycles
			dTime = millis()-prevTime;
		}
		FPS = nbScreens;	//Updates FPS variable
	}
}





int main(){
	if (wiringPiSetup()==-1){	//INIT of wiringPi
		std::cout<<"Thread initialisation failed"<<std::endl;
		return EXIT_FAILURE;
	}

	for (unsigned int pin: PINS){	//Enables outputs
		pinMode(pin, OUTPUT);
	}

	piLock(DATAS_KEY);
	initDATAS();
	piUnlock(DATAS_KEY);


	int x = piThreadCreate(deamonLED);	//Starts the display
	if (x!=0){
		std::cout<<"Thread didnt start"<<std::endl;
	}

/*
	while (true){
		std::cout<<FPS<<" fps."<<std::endl;
		delay(500);
	}
*/
	calibrate();


	return EXIT_SUCCESS;
}




int askUser() {
	int number;
	std::cout<<"\nEnter a number: ";
	std::cin>> number;
	return number;
}





int sendPacket(std::vector<int> & rawDatas) {
/* PWM ratio are stored with values V between 0 and 3.
	When a value V is not 0, it sets on an output, and this value is decreased.
	Therefore, after V passes in the loop, the output is turned off, simulating PWM */

	resetPins();
	for (unsigned int i(rawDatas.size()-1); i!=0; i--) {	//LSB First, so datas are sent from the end of the table...
		if (rawDatas[i] !=0 ) {
			digitalWrite(PINS[0], HIGH);
			rawDatas[i] = rawDatas[i]-1;	//Decrease the value, as explained in description of the function
		} else {
			digitalWrite(PINS[0], LOW);
		}
		digitalWrite(PINS[1], HIGH);	//Register transmission

		digitalWrite(PINS[0], LOW);	//Reset of data pin
		digitalWrite(PINS[1], LOW);	//Reset of shift pin
	}

	digitalWrite(PINS[2], HIGH);	//Outputs transmission
	digitalWrite(PINS[2], LOW);
	return EXIT_SUCCESS;
}





int resetPins() {	//All output pins at LOW level
	for (unsigned int pin : PINS) {
		digitalWrite(pin, LOW);
	}
	return EXIT_SUCCESS;
}



int drawScreen() {
	/* One frame composed of several cycles of PWM */

	std::vector<int> rawDATAS (convertImageToLED());

	for (unsigned int i(0); i<4; i++){
		sendPacket(rawDATAS);
	}

	return EXIT_SUCCESS;
}




std::vector<int> convertImageToLED(){
	std::vector<int> procImL0 (SIZE[0]*SIZE[1]*64*3);	//Processed image

	std::vector<int> tempBWpixel{};

	piLock(DATAS_KEY);
	for (unsigned int noLine(0); noLine<SIZE[1]; noLine++){	//Line of the picture
		for (unsigned int cell(0); cell<SIZE[0]; cell++){	//Cell in the line (one cell is composed of 8 pixels)
			for (unsigned int noPixel(0); noPixel<8; noPixel++){	//Pixel in the cell

				tempBWpixel = convertPixelBW(DATAS[noLine][cell*8 + noPixel]);
				//in red:
				procImL0[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel] = tempBWpixel[0];

				//in green:
				procImL0[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+8] = tempBWpixel[1];

				//in blue:
				procImL0[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+16] = tempBWpixel[2];
			}
		}
	}
	piUnlock(DATAS_KEY);
	return procImL0;
}



std::vector<int> convertPixelBW(std::vector<int> const& pixel){
/* Basically, converts level from 0-255 to 0-3 */
	int Red = convertValuePWM(pixel[0], 0);
	int Green = convertValuePWM(pixel[1], 1);
	int Blue = convertValuePWM(pixel[2], 2);

	std::vector<int> result {Red, Green, Blue};
	return result;
}



int convertValuePWM(int const& value, int const& color){
	switch (color){
		case 0:	//RED
			if (value<RED_VALUES[0]) {
				return 0;
			} else if (value<RED_VALUES[1]) {
				return 1;
			} else if (value<RED_VALUES[2]) {
				return 2;
			} else if (value<RED_VALUES[3]) {
				return 3;
			} else {
				return 4;
			}
			break;
		case 1:	//GREEN
				if (value<GREEN_VALUES[0]) {
					return 0;
				} else if (value<GREEN_VALUES[1]) {
					return 1;
				} else if (value<GREEN_VALUES[2]) {
					return 2;
				} else if (value<GREEN_VALUES[3]) {
					return 3;
				} else {
					return 4;
				}
				break;
		case 2:	//BLUE
			if (value<BLUE_VALUES[0]) {
				return 0;
			} else if (value<BLUE_VALUES[1]) {
				return 1;
			} else if (value<BLUE_VALUES[2]) {
				return 2;
			} else if (value<BLUE_VALUES[3]) {
				return 3;
			} else {
				return 4;
			}
			break;
		default:
			return 0;
			break;
		return 0;
}



void initDATAS(){
	//Set a test pattern as the frame
	std::vector<int> red {255,0,0};
	std::vector<int> green{0,255,0};
	std::vector<int> blue {0,0,255};

	std::vector<std::vector<int>> lineA {};
	std::vector<std::vector<int>> lineB {};

	for (unsigned int pixel (0); pixel<SIZE[0]*8; pixel += 2) {
		lineA.push_back(red);
		lineA.push_back(green);
	}
	for (unsigned int pixel (0); pixel<SIZE[0]*8; pixel += 2) {
		lineB.push_back(green);
		lineB.push_back(blue);
	}

	for (unsigned int line (0); line<SIZE[1]*8; line+=2) {
		DATAS.push_back(lineA);
		DATAS.push_back(lineB);
	}
}



int calibrate(){
	unsigned int answer (0);
	while(answer<1 || answer>4){
		std::cout<<"\t*** CALIBRATION MENU ***\n"<<std::endl;
		std::cout<<"\t\t"<<"0%"<<"\t"<<"25%"<<"\t"<<"50%"<<"\t"<<"75%"<<std::endl;
		std::cout<<"RED : \t["<<RED_VALUES[0]<<"]\t["<<RED_VALUES[1]<<"]\t["<<RED_VALUES[2]<<"]"<<std::endl;
		std::cout<<"GREEN : \t["<<GREEN_VALUES[0]<<"]\t["<<GREEN_VALUES[1]<<"]\t["<<GREEN_VALUES[2]<<"]"<<std::endl;
		std::cout<<"BLUE : \t["<<BLUE_VALUES[0]<<"]\t["<<BLUE_VALUES[1]<<"]\t["<<BLUE_VALUES[2]<<"]\n"<<std::endl;
		std::cout<<"1 - Calibrate RED"<<std::endl;
		std::cout<<"2 - Calibrate GREEN"<<std::endl;
		std::cout<<"3 - Calibrate BLUE"<<std::endl;
		std::cout<<"4 - Cancel"<<std::endl;
		std::cout<<"> ";
		std::cin>>answer;	answer = (unsigned int)answer;

		switch (answer) {
			case 1:
				std::cout<<"\n*RED : \t["<<RED_VALUES[0]<<"]\t["<<RED_VALUES[1]<<"]\t["<<RED_VALUES[2]<<"]"<<std::endl;
				std::cin>>RED_VALUES[0];
				std::cin>>RED_VALUES[1];
				std::cin>>RED_VALUES[2];
				break;
			case 2:
				std::cout<<"\n*GREEN : \t["<<GREEN_VALUES[0]<<"]\t["<<GREEN_VALUES[1]<<"]\t["<<GREEN_VALUES[2]<<"]"<<std::endl;
				std::cin>>GREEN_VALUES[0];
				std::cin>>GREEN_VALUES[1];
				std::cin>>GREEN_VALUES[2];
				break;
			case 3:
				std::cout<<"\n*BLUE : \t["<<BLUE_VALUES[0]<<"]\t["<<BLUE_VALUES[1]<<"]\t["<<BLUE_VALUES[2]<<"]\n"<<std::endl;
				std::cin>>BLUE_VALUES[0];
				std::cin>>BLUE_VALUES[1];
				std::cin>>BLUE_VALUES[2];
				break;
			case 4:
				return EXIT_SUCCESS;
			default:
				break;
		}
}
