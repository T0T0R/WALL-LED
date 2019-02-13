#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <wiringPi.h>

#define DATAS_KEY	0

std::vector<std::vector<std::vector<int>>> DATAS {};	//Global variables = pure evil !
int FPS = 0;
std::vector<unsigned int> SIZE {4, 4};
std::vector<unsigned int> PINS {0, 2, 3};
/*PINS:
	- Datas pin
	- shift pin
	- memory pin
	- allow output
*/



int askUser();
int sendPacket(std::vector<bool> const& rawDatas);
int resetPins();
int drawScreen();
std::vector<std::vector<bool>> convertPixelBW(std::vector<int> const& pixel);
std::vector<std::vector<bool>> convertImageToLED();
std::vector<bool> convertValuePWM(unsigned int const& value);
void initDATAS();






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

	initDATAS();
	std::cout<<DATAS.size()<<std::endl;
	std::cout<<DATAS[0].size()<<std::endl;
	std::cout<<DATAS[0][0].size()<<std::endl;

	int x = piThreadCreate(deamonLED);	//Starts the display
	if (x!=0){
		std::cout<<"Thread didnt start"<<std::endl;
	}


	while (true){
		std::cout<<FPS<<" fps."<<std::endl;
		delay(500);
	}


	return EXIT_SUCCESS;
}




int askUser() {
	int number;
	std::cout<<"\nEnter a number: ";
	std::cin>> number;
	return number;
}



int sendPacket(std::vector<bool> const& rawDatas) {
	resetPins();
	for (bool isOn : rawDatas) {
		if (isOn) { digitalWrite(PINS[0], HIGH); } else { digitalWrite(PINS[0], LOW); }
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

	std::vector<std::vector<bool>> rawDATAS (convertImageToLED());

	sendPacket(rawDATAS[0]);
	sendPacket(rawDATAS[1]);
	sendPacket(rawDATAS[2]);
	sendPacket(rawDATAS[3]);

	return EXIT_SUCCESS;
}


std::vector<std::vector<bool>> convertImageToLED(){
	std::vector<bool> procImL0 (SIZE[0]*SIZE[1]*64*3);	//Processed image, layer 0
	std::vector<bool> procImL1 (SIZE[0]*SIZE[1]*64*3);
	std::vector<bool> procImL2 (SIZE[0]*SIZE[1]*64*3);
	std::vector<bool> procImL3 (SIZE[0]*SIZE[1]*64*3);

	std::vector<std::vector<bool>> tempBWpixel{};

	piLock(DATAS_KEY);
	for (unsigned int noLine(0); noLine<SIZE[1]; noLine++){	//Line of the picture
		for (unsigned int cell(0); cell<SIZE[0]; cell++){	//Cell in the line (one cell is composed of 8 pixels)
			for (unsigned int noPixel(0); noPixel<8; noPixel++){	//Pixel in the cell

				tempBWpixel = convertPixelBW(DATAS[noLine][cell*8 + noPixel]);
				//in red:
				procImL0[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel] = tempBWpixel[0][0];
				procImL1[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel] = tempBWpixel[0][1];
				procImL2[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel] = tempBWpixel[0][2];
				procImL3[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel] = tempBWpixel[0][3];

				//in green:
				procImL0[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+8] = tempBWpixel[1][0];
				procImL1[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+8] = tempBWpixel[1][1];
				procImL2[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+8] = tempBWpixel[1][2];
				procImL3[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+8] = tempBWpixel[1][3];

				//in blue:
				procImL0[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+16] = tempBWpixel[2][0];
				procImL1[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+16] = tempBWpixel[2][1];
				procImL2[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+16] = tempBWpixel[2][2];
				procImL3[noLine*SIZE[0]*8*3 + cell*8*3 + noPixel+16] = tempBWpixel[2][3];
			}
		}
	}
	piUnlock(DATAS_KEY);
	std::vector<std::vector<bool>> result {procImL0, procImL1, procImL2, procImL3};
	return result;
}



std::vector<std::vector<bool>> convertPixelBW(std::vector<int> const& pixel){
	std::vector<bool> Red = convertValuePWM(pixel[0]);
	std::vector<bool> Green = convertValuePWM(pixel[1]);
	std::vector<bool> Blue = convertValuePWM(pixel[2]);

	std::vector<std::vector<bool>> result {Red, Green, Blue};
	return result;
}



std::vector<bool> convertValuePWM(unsigned int const& value){
	if(value<64){
		return std::vector<bool> {false, false, false, false};
	} else if (value<128) {
		return std::vector<bool> {true, false, false, false};
	} else if (value<192) {
		return std::vector<bool> {true, true, false, false};
	} else if (value<256) {
		return std::vector<bool> {true, true, true, false};
	} else {
		return std::vector<bool> {true, true, true, true};
	}
}



void initDATAS(){
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
