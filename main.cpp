#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

#include <ncurses.h>
#include <wiringPi.h>
#include <wiringShift.h>

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
//std::vector<int> convertImageToLED();
std::vector<std::vector<int>> convertImageToLED()
int convertValuePWM(int const& value, int const& color);
void initDATAS();
int M_displayPatterns();
int M_spectrum();
int M_pong();
int play_pong(int const& screenMode, std::vector<int> const& fgColor, std::vector<int> const& HUDcolor);
int pongMovePlayer(int const& player, int const& direction, std::vector<int> & playerPos, int const& mode);
int M_calibrate();

std::vector<int> test(16);


PI_THREAD(deamonLED){
	int nbScreens = 0;	//Used to count fps
	int dTime = 0;
	int prevTime = 0;

	while(1==1){
		/*
		test[0]=3;
		test[1]=0;
		test[2]=3;
		test[3]=0;
		test[4]=3;
		test[5]=0;
		test[6]=3;
		test[7]=0;
		test[8]=2;
		test[9]=2;
		test[10]=2;
		test[11]=2;
		test[12]=0;
		test[13]=0;
		test[14]=0;
		test[15]=0;
		*/
		dTime = 0;	//Reset of deltaTime
		nbScreens = 0;
		prevTime = millis();
		while(dTime<=1000){	//Prints nb of screens displayed
					//	after one second
			nbScreens +=1;
			drawScreen();	//Draw a frame in several PWM cycles
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

//	for(;;){}	//Infinite loop


	while (true){
		std::cout<<FPS<<" fps."<<std::endl;
		delay(1000);
	}


/*
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
*/
	return EXIT_SUCCESS;
}







int sendPacket(std::vector<int> & rawDatas) {
/* PWM ratio are stored with values V between 0 and 3.
	When a value V is not 0, it sets on an output, and this value is decreased.
	Therefore, after V passes in the loop, the output is turned off, simulating PWM */

	resetPins();
	unsigned int size = rawDatas.size();
	int i8bitPacket (0);
	int bitPos (0);

	for (unsigned int i(0); i<size; i++) {	//LSB First, so datas are sent from the end of the table...

		bitPos = i%8;	//Build the packet:
		switch (bitPos){
		case 0:
			if(rawDatas[size-i-1]!=0){	//1st bit = 2^0 = 1
				i8bitPacket ++;
				rawDatas[size-i-1] = rawDatas[size-i-1]-1;	//Decrease the value, as explained in description of the function
			}
			break;
		case 1:
			if(rawDatas[size-i-1]!=0){	//2nd bit = 2^1 = 2
				i8bitPacket = i8bitPacket+2;
				rawDatas[size-i-1] = rawDatas[size-i-1]-1;
			}
			break;
		case 2:
			if(rawDatas[size-i-1]!=0){	//3nd bit = 2^2 = 4
				i8bitPacket = i8bitPacket+4;
				rawDatas[size-i-1] = rawDatas[size-i-1]-1;
			}
			break;
		case 3:
			if(rawDatas[size-i-1]!=0){	//3rd bit = 2^3 = 8
				i8bitPacket = i8bitPacket+8;
				rawDatas[size-i-1] = rawDatas[size-i-1]-1;
			}
			break;
		case 4:
			if(rawDatas[size-i-1]!=0){	//4th bit = 2^4 = 16
				i8bitPacket = i8bitPacket+16;
				rawDatas[size-i-1] = rawDatas[size-i-1]-1;
			}
			break;
		case 5:
			if(rawDatas[size-i-1]!=0){	//5th bit = 2^5 = 32
				i8bitPacket = i8bitPacket+32;
				rawDatas[size-i-1] = rawDatas[size-i-1]-1;
			}
			break;
		case 6:
			if(rawDatas[size-i-1]!=0){	//6th bit = 2^6 = 64
				i8bitPacket = i8bitPacket+64;
				rawDatas[size-i-1] = rawDatas[size-i-1]-1;
			}
			break;
		case 7:
			if(rawDatas[size-i-1]!=0){	//7th bit = 2^7 = 128
				i8bitPacket = i8bitPacket+128;
				rawDatas[size-i-1] = rawDatas[size-i-1]-1;
			}
			//Correct size for a packet ; can be sent
			shiftOut(PINS[0], PINS[1], LSBFIRST, i8bitPacket);
			i8bitPacket = 0;	//Reset the packet after being sent
			break;
		default:
			break;
		}

	}

	digitalWrite(PINS[2], HIGH);	//Outputs transmission
//	delayMicroseconds(1);
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

	//std::vector<int> rawDATAS (convertImageToLED());
	std::vector<std::vector<int>> rawDATAS (convertImageToLED());
	//std::vector<int> rawDATAS (test);

	for (unsigned int i(0); i<4; i++){
		sendPacket(rawDATAS);
	}

	return EXIT_SUCCESS;
}


/*
std::vector<int> convertImageToLED() {
	std::vector<int> procImL0 (SIZE[0]*SIZE[1]*64*3);	//Processed image

	std::vector<int> tempBWpixel {};

	piLock(DATAS_KEY);
	for (unsigned int noLine(0); noLine<SIZE[1]; noLine++) {	//Line of the picture
		for (unsigned int cell(0); cell<SIZE[0]; cell++) {	//Cell in the line (one cell is composed of 8 pixels)
			for (unsigned int noPixel(0); noPixel<8; noPixel++) {	//Pixel in the cell

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
*/



std::vector<std::vector<int>> convertImageToLED() {	
	/* Eache lineSet is build by getting the values for the pixels in this very line set (SIZE[0]*SIZE[1]*8*3 bits)
		and also the byte read by the shift register (row byte) that chooses the row/line to display (+8 bits)
		*/

	int rowByteSize (8);	//Not so hard-coded to not forget to leave space at the beginning of
					//a lineset to store there the data for the shift register that drives the rows.

	std::vector<std::vector<int>> procImage (8);	//Processed image, composed of 8 line sets
	std::vector<int> procLineSet (SIZE[0]*SIZE[1]*8*3 + 8);	//Processed line set of an image

	std::vector<int> tempBWpixel {};

	piLock(DATAS_KEY);

	for (unsigned int lineSet(0); lineSet<8; lineSet++){	//Line set of the picture

		switch(lineSet){	//Row byte
			case 0:
				procLineSet[0] = 9; procLineSet[1] = 0; procLineSet[2] = 0; procLineSet[3] = 0;	//We put 9, because it can't be erased that easily when changing PWM cycle
				procLineSet[4] = 0; procLineSet[5] = 0; procLineSet[6] = 0; procLineSet[7] = 0;
				break;
			case 1:
				procLineSet[0] = 0; procLineSet[1] = 9; procLineSet[2] = 0; procLineSet[3] = 0;
				procLineSet[4] = 0; procLineSet[5] = 0; procLineSet[6] = 0; procLineSet[7] = 0;
				break;
			case 2:
				procLineSet[0] = 0; procLineSet[1] = 0; procLineSet[2] = 9; procLineSet[3] = 0;
				procLineSet[4] = 0; procLineSet[5] = 0; procLineSet[6] = 0; procLineSet[7] = 0;
				break;
			case 3:
				procLineSet[0] = 0; procLineSet[1] = 0; procLineSet[2] = 0; procLineSet[3] = 9;
				procLineSet[4] = 0; procLineSet[5] = 0; procLineSet[6] = 0; procLineSet[7] = 0;
				break;
			case 4:
				procLineSet[0] = 0; procLineSet[1] = 0; procLineSet[2] = 0; procLineSet[3] = 0;
				procLineSet[4] = 9; procLineSet[5] = 0; procLineSet[6] = 0; procLineSet[7] = 0;
				break;
			case 5:
				procLineSet[0] = 0; procLineSet[1] = 0; procLineSet[2] = 0; procLineSet[3] = 0;
				procLineSet[4] = 0; procLineSet[5] = 9; procLineSet[6] = 0; procLineSet[7] = 0;
				break;
			case 6:
				procLineSet[0] = 0; procLineSet[1] = 0; procLineSet[2] = 0; procLineSet[3] = 0;
				procLineSet[4] = 0; procLineSet[5] = 0; procLineSet[6] = 9; procLineSet[7] = 0;
				break;
			case 7:
				procLineSet[0] = 0; procLineSet[1] = 0; procLineSet[2] = 0; procLineSet[3] = 0;
				procLineSet[4] = 0; procLineSet[5] = 0; procLineSet[6] = 0; procLineSet[7] = 9;
				break;
			default:
				break;
		}

		for (unsigned int cellLine(0); cellLine<SIZE[1]; cellLine++){	//Yeah, we got the line set, but which line of the image in this line set ? cellLine*8 + lineSet !
			for (unsigned int cell(0); cell<SIZE[0]; cell++) {	//Cell in the cellLine (one cell is composed of 8 pixels)
				for (unsigned int noPixel(0); noPixel<8; noPixel++) {	//Pixel in the cell

					tempBWpixel = convertPixelBW(DATAS[cellLine*8 + lineSet][cell*8 + noPixel]);
					//in red:
					procLineSet[rowByteSize + cellLine*SIZE[0]*8*3 + cell*8*3 + noPixel] = tempBWpixel[0];

					//in green:
					procLineSet[rowByteSize + cellLine*SIZE[0]*8*3 + cell*8*3 + noPixel+8] = tempBWpixel[1];

					//in blue:
					procLineSet[rowByteSize + cellLine*SIZE[0]*8*3 + cell*8*3 + noPixel+16] = tempBWpixel[2];
				}
			}
		}

		procImage.push_back(procLineSet);	//Add the lineSet to the image
		procLineSet.clear();	//Empty the temporary lineset, so it can be used again
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
