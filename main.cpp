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
std::vector<std::vector<std::vector<int>>> DATAS {};
int FPS = 0;
std::vector<unsigned int> SIZE {4, 4};
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
int M_displayPatterns();
int M_spectrum();
int M_pong();
int play_pong(int const& screenMode, std::vector<int> const& fgColor, std::vector<int> const& HUDcolor);
int pongMovePlayer(int const& player, int const& direction, std::vector<int> & playerPos, int const& mode);
int M_calibrate();






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


	unsigned int choice (0);
	while (choice<1 || choice>4) {
		choice = 0;
		std::cout<<"\t===== WALL-LED =====\n"<<std::endl;
		std::cout<<"1 - Display patterns"<<std::endl;
		std::cout<<"2 - Spectrum"<<std::endl;
		std::cout<<"3 - Pong"<<std::endl;
		std::cout<<"4 - Color Calibration"<<std::endl;
		std::cout<<"5 - EXIT"<<std::endl;
		std::cout<<"> ";
		std::cin>>choice;	choice = (unsigned int)choice;

		switch (choice) {
			case 1:
				M_displayPatterns();
				choice = 0;
				break;
			case 2:
				M_spectrum();
				choice = 0;
				break;
			case 3:
				M_pong();
				choice = 0;
				break;
			case 4:
				M_calibrate();
				choice = 0;
				break;
			case 5:
				return EXIT_SUCCESS;
			default:
				break;
		}
	}
	return EXIT_SUCCESS;
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
	}
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



int M_displayPatterns() {
	return EXIT_SUCCESS;
}



int M_spectrum() {
	return EXIT_SUCCESS;
}



int M_pong() {
	std::vector<int> WHITE {255,255,255};
	play_pong(1, WHITE, WHITE);
	return EXIT_SUCCESS;
}

int play_pong(int const& screenMode, std::vector<int> const& fgColor, std::vector<int> const& HUDcolor){
	//screenMode : =0 (4*4 cells), =1 (4*2 cells)
	initscr();
	clear();
	noecho();
	keypad(stdscr, TRUE);
	cbreak();
	printw("*** Pong game ***\n");
	printw("- Left player:\t[e]:up\t\t- Right player:\t[UP]:up\n");
	printw("\t\t[c]:down\t\t \t[DOWN]:down\n");
	printw("- Other:\t[q]:exit");
	refresh();
	bool gameRunning (true);
	int c (0);

	std::vector<int> playerPos (2);
	std::vector<int> ballPos (2);

	std::vector<int> score {0,0};
	if (screenMode==0){	//4*4 cells
		playerPos[0]=13;	playerPos[1]=13;
		ballPos[0]=15;	 ballPos[0]=15;
	}else{	//4*2 cells
		playerPos[0]=6;	playerPos[1]=6;
		ballPos[0]=15;	 ballPos[0]=8;
	}

	while (gameRunning){
		c = getch();

		switch (c){
			case 113:	// key q pressed : quit
				gameRunning = false;
				break;
			case 101:		// key e pressed: left player, up
				pongMovePlayer(0,1,playerPos, screenMode);
				refresh();
				break;
			case 99:		// key c pressed: left player, down
				pongMovePlayer(0,-1,playerPos, screenMode);
				refresh();
				break;
			case KEY_UP:		// key UP pressed: right player, up
				pongMovePlayer(1,1,playerPos, screenMode);
				refresh();
				break;
			case KEY_DOWN:		// key DOWN pressed: right player, down
				pongMovePlayer(1,-1,playerPos, screenMode);
				refresh();
				break;
			default:
				break;
			
		}
		
	}
	endwin();
	return EXIT_SUCCESS;
}



int pongMovePlayer(int const& player, int const& direction, std::vector<int> & playerPos, int const& mode){
	if(player==0){	//Left player
		if (direction==1){	//up
			if(playerPos[0]<=3){	playerPos[0]=3;
			}else{	playerPos[0] = playerPos[0]-1;
			}
		}else{	//down
			if (mode==0){	//4*4 cells
				if(playerPos[0]>=23){	playerPos[0]=23;
				}else{	playerPos[0] = playerPos[0]+1;
				}
			}else{	//4*2 cells
				if(playerPos[0]>=9){	playerPos[0]=9;
				}else{	playerPos[0] = playerPos[0]+1;
				}
			}
		}

	}else{	//Right player
		if (direction==1){
			if(playerPos[1]<=3){	playerPos[1]=3;
			}else{	playerPos[1] = playerPos[1]-1;
			}

		}else{
			if (mode==0){	//4*4 cells
				if(playerPos[1]>=23){	playerPos[1]=23;
				}else{	playerPos[1] = playerPos[1]+1;
				}
			}else{	//4*2 cells
				if(playerPos[1]>=9){	playerPos[1]=9;
				}else{	playerPos[1] = playerPos[1]+1;
				}
			}
		}
	}
	printw("LP : %d\tRP : %d\n", playerPos[0], playerPos[1]);
	return EXIT_SUCCESS;
}



int M_calibrate(){
	unsigned int choice (0);
	while(choice<1 || choice>4){
		choice=0;
		std::cout<<"\t*** CALIBRATION MENU ***\n"<<std::endl;
		std::cout<<"PWM duty\t"<<"0%"<<"\t"<<"25%"<<"\t"<<"50%"<<"\t"<<"75%"<<std::endl;
		std::cout<<"RED: \t[0]\t["<<RED_VALUES[0]<<"]\t["<<RED_VALUES[1]<<"]\t["<<RED_VALUES[2]<<"]"<<std::endl;
		std::cout<<"GREEN: \t[0]\t["<<GREEN_VALUES[0]<<"]\t["<<GREEN_VALUES[1]<<"]\t["<<GREEN_VALUES[2]<<"]"<<std::endl;
		std::cout<<"BLUE: \t[0]\t["<<BLUE_VALUES[0]<<"]\t["<<BLUE_VALUES[1]<<"]\t["<<BLUE_VALUES[2]<<"]\n"<<std::endl;
		std::cout<<"1 - Calibrate RED"<<std::endl;
		std::cout<<"2 - Calibrate GREEN"<<std::endl;
		std::cout<<"3 - Calibrate BLUE"<<std::endl;
		std::cout<<"4 - EXIT"<<std::endl;
		std::cout<<"> ";
		std::cin>>choice;	choice = (unsigned int)choice;

		switch (choice) {
			case 1:
				std::cout<<"\n*RED: \t["<<RED_VALUES[0]<<"]\t["<<RED_VALUES[1]<<"]\t["<<RED_VALUES[2]<<"]"<<std::endl;
				std::cin>>RED_VALUES[0];
				std::cin>>RED_VALUES[1];
				std::cin>>RED_VALUES[2];
				choice=0;
				break;
			case 2:
				std::cout<<"\n*GREEN: \t["<<GREEN_VALUES[0]<<"]\t["<<GREEN_VALUES[1]<<"]\t["<<GREEN_VALUES[2]<<"]"<<std::endl;
				std::cin>>GREEN_VALUES[0];
				std::cin>>GREEN_VALUES[1];
				std::cin>>GREEN_VALUES[2];
				choice=0;
				break;
			case 3:
				std::cout<<"\n*BLUE: t["<<BLUE_VALUES[0]<<"]\t["<<BLUE_VALUES[1]<<"]\t["<<BLUE_VALUES[2]<<"]\n"<<std::endl;
				std::cin>>BLUE_VALUES[0];
				std::cin>>BLUE_VALUES[1];
				std::cin>>BLUE_VALUES[2];
				choice=0;
				break;
			case 4:
				return EXIT_SUCCESS;
			default:
				break;
		}
	}
	return EXIT_SUCCESS;
}
