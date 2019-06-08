#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>

#include <ncurses.h>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "./FFT/FFT.h"

#include <wiringPi.h>
#include <wiringShift.h>

#define DATAS_KEY	0
#define FPS		400
#define PI 3.14159265

//Global variables = pure evil !
bool deamonDisplay (true);
std::vector<std::vector<std::vector<int>>> DATAS {};
int fps (0);
unsigned int SIZE_X (4);
unsigned int SIZE_Y (4);
std::vector<int> PINS {0, 2, 3};
/*PINS:
	- Datas pin
	- clock pin
	- latch pin
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
std::vector<std::vector<int>> convertImageToLED();
int convertValuePWM(int const& value, int const& color);
void initDATAS();
int M_displayPatterns();
int M_spectrum();
int M_pong();
int play_pong(int const& screenMode, std::vector<int> const& fgColor, std::vector<int> const& HUDcolor);
int pongMovePlayer(int const& player, int const& direction, std::vector<int> & playerPos, int const& mode);
int pongInitBall(std::vector<double> & ballPosAngle, int const& screenMode);
int pongMoveBall(std::vector<double> & ballPosAngle, std::vector<int> const& playerPos, int const& screenMode, int const& dTime);
int pongDisplay(std::vector<double> const& ballPosAngle, std::vector<int> const& playerPos, std::vector<int> const& score,
				int const& screenMode, std::vector<int> const& fgColor, std::vector<int> const& HUDcolor);
bool calcScore(std::vector<double> const& ballPosAngle, std::vector<int> & score);
int drawScore(std::vector<int> const& score, std::vector<int> const& HUDcolor);
int M_settings();

std::vector<int> test(16);


PI_THREAD(deamonLED){
	std::ofstream myFile;
	myFile.open ("fps.txt");

	int nbScreens (0);	//Used to count fps
	int dTimeSec (0);
	int prevTimeSec (0);

	int dTimeFrame (0);	//Used to force fps
	int prevTimeFrame (0);

	while(deamonDisplay){

		dTimeSec = 0;	//Reset of deltaTime
		nbScreens = 0;
		prevTimeSec = millis();

		dTimeFrame = 0;
		prevTimeFrame = millis();

		while(dTimeSec<=1000){
			while(dTimeFrame<=(int)(1000/FPS)){	//Force non-constant update of the screen by introducing delay
				delay((int)(100/FPS));
				dTimeFrame = millis() - prevTimeFrame;	//update duration of the frame
			}

			nbScreens +=1;
			drawScreen();	//Draw a frame in several PWM cycles

			dTimeFrame = 0;	//reset duration of the frame
			prevTimeFrame = millis();
			dTimeSec = millis() - prevTimeSec;	//update duration of a second
		}

		fps = nbScreens;	//Updates FPS variable
		myFile << fps<<" fps."<< std::endl;
	}
	myFile.close();
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

	piLock(DATAS_KEY);
	initDATAS();
	piUnlock(DATAS_KEY);


	int x = piThreadCreate(deamonLED);	//Starts the display
	if (x!=0){
		std::cout<<"Thread didnt start"<<std::endl;
	}

//	for(;;){}	//Infinite loop

/*
	while (true){
		std::cout<<fps<<" fps."<<std::endl;
		piLock(DATAS_KEY);
		DATAS[0][1][0] = 127;
		piUnlock(DATAS_KEY);
		delay(1000);
	}
*/


	unsigned int choice (0);
	while (choice<1 || choice>4) {
		choice = 0;
		std::cout<<"\t===== WALL-LED =====\n"<<std::endl;
		std::cout<<"1 - Display patterns"<<std::endl;
		std::cout<<"2 - Spectrum"<<std::endl;
		std::cout<<"3 - Pong"<<std::endl;
		std::cout<<"4 - Settings"<<std::endl;
		std::cout<<"5 - EXIT"<<std::endl;
		std::cout<<"> ";
		std::cin>>choice;	choice = (unsigned int)choice;
		//choice=2;


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
				M_settings();
				choice = 0;
				break;
			case 5:
				return EXIT_SUCCESS;
			default:
				break;
		}
	}


	resetPins();
	deamonDisplay = false;
	delay(100);
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
	/* One frame composed of several (4) cycles of PWM */

	std::vector<std::vector<int>> rawDATAS (convertImageToLED());
	//std::vector<int> rawDATAS (test);
	for (auto lineSet: rawDATAS){
		for (unsigned int i(0); i<4; i++){
			sendPacket(lineSet);
		}
	}

	return EXIT_SUCCESS;
}



std::vector<std::vector<int>> convertImageToLED() {
	/* Each lineSet is build by getting the values for the pixels in this very line set (SIZE_X*SIZE_Y*8*3 bits)
		and also the byte read by the shift register (row byte) that chooses the row/line to display (+8 bits)
	*/

	int rowByteSize (8);	//Not so hard-coded to not forget to leave space at the beginning of
					//a lineset to store there the data for the shift register that drives the rows.

	std::vector<std::vector<int>> procImage (8);	//Processed image, composed of 8 line sets
	std::vector<int> procLineSet (SIZE_X*SIZE_Y*8*3 + 8);	//Processed line set of an image

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

		for (unsigned int cellLine(0); cellLine<SIZE_Y; cellLine++){	//Yeah, we got the line set, but which line of the image in this line set ? cellLine*8 + lineSet !
			for (unsigned int cell(0); cell<SIZE_X; cell++) {	//Cell in the cellLine (one cell is composed of 8 pixels)
				for (unsigned int noPixel(0); noPixel<8; noPixel++) {	//Pixel in the cell

					tempBWpixel = convertPixelBW(DATAS[cellLine*8 + lineSet][cell*8 + noPixel]);
					//in red:
					procLineSet[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel] = tempBWpixel[0];

					//in green:
					procLineSet[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+8] = tempBWpixel[1];

					//in blue:
					procLineSet[rowByteSize + cellLine*SIZE_X*8*3 + cell*8*3 + noPixel+16] = tempBWpixel[2];
				}
			}
		}

		procImage.push_back(procLineSet);	//Add the lineSet to the image
		procLineSet.clear();	//Empty the temporary lineset, so it can be used again
	}
	piUnlock(DATAS_KEY);
	return procImage;
}



std::vector<int> convertPixelBW(std::vector<int> const& pixel){
/* Converts hue from 3*(0-255) to 3*(0-3) */
	int Red = convertValuePWM(pixel[0], 0);
	int Green = convertValuePWM(pixel[1], 1);
	int Blue = convertValuePWM(pixel[2], 2);

	std::vector<int> result {Red, Green, Blue};
	return result;
}



int convertValuePWM(int const& value, int const& color){
	/* Basically, converts values from 0-255 to 0-3.
		Conversion can be non-linear if confugured by user in
		the "Calibration" menu.
		Linear by default.
	*/
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
	for (unsigned int pixel (0); pixel<SIZE_X*8; pixel += 2) {
		lineA.push_back(red);
		lineA.push_back(green);
	}
	for (unsigned int pixel (0); pixel<SIZE_X*8; pixel += 2) {
		lineB.push_back(green);
		lineB.push_back(blue);
	}

	for (unsigned int line (0); line<SIZE_Y*8; line+=2) {
		DATAS.push_back(lineA);
		DATAS.push_back(lineB);
	}
}



int M_displayPatterns() {
	/* NOT DONE YET */
	return EXIT_SUCCESS;
}



int M_spectrum() {
	/* NOT DONE YET */

	sf::RenderWindow window(sf::VideoMode(900,900,32),"Window");

	std::string path;
	int bufferSize (0);
	std::cout<<"Put an audio file in the Ressources folder (.wav will work, mp3 wont)"<<std::endl;
	std::cout<<"Enter the file name (example.wav) : ";
	//std::cin>>path;
	path="test.flac";
	std::cout<<"Enter the buffer size treated by the fft (powers of two works best like 16384)"<<std::endl;
	//std::cin>>bufferSize;
	bufferSize=8192;

	FFT fft("Ressources/"+path,bufferSize);

	int dTime(0);	//Used to regulate the framerate (FPS constant)
	int prevTime = millis();

	bool isDone (false);

	while(window.isOpen() && !isDone ){

		while(dTime <= (int)(1000/FPS)){
			delay((int)(100/FPS));
			dTime = millis()-prevTime;
		}

		//std::cout<<dTime<<std::endl;
		isDone = fft.update() ;

		window.clear() ;
		fft.draw(window) ;
		window.display() ;

		dTime=0;
		prevTime=millis();
	}


	return EXIT_SUCCESS;
}



int M_pong() {
	std::cout<<"\n*** Pong game ***"<<std::endl;

	unsigned int choiceDisp (0);
	int screenMode (0);
	while (choiceDisp<1 || choiceDisp>3) {
		choiceDisp = 0;
		std::cout<<" * Choose display"<<std::endl;
		std::cout<<"1 - 4*2 cells"<<std::endl;

		if (SIZE_Y==4){	//Display the other choice only if available
			std::cout<<"2 - 4*4 cells"<<std::endl;
		}

		std::cout<<"3 - EXIT"<<std::endl;
		std::cout<<"> ";
		std::cin>>choiceDisp;	choiceDisp = (unsigned int)choiceDisp;
		
		switch (choiceDisp) {
			case 1:
				screenMode = 1;
				break;
			case 2:
				if (SIZE_Y!=4){
					std::cout<<"Screen too small !\n"<<std::endl;
					choiceDisp = 0;	//go back in the choice loop
					break;
				}else{
					screenMode = 0;
				}
				break;
			case 3:
				return EXIT_SUCCESS;
			default:
				break;
		}
	}

	std::cout<<" * Choose foreground color\n"<<std::endl;
	std::cout<<"RED : 0-255 > ";
	int redValue  (255);	std::cin>>redValue;
	std::cout<<"GREEN : 0-255 > ";
	int greenValue (255);	std::cin>>greenValue;
	std::cout<<"BLUE : 0-255 > ";
	int blueValue (255);	std::cin>>blueValue;
	std::vector<int> fgColor {redValue, greenValue, blueValue};

	std::cout<<" * Choose HUD color\n"<<std::endl;
	std::cout<<"RED : 0-255 > ";
	std::cin>>redValue;
	std::cout<<"GREEN : 0-255 > ";
	std::cin>>greenValue;
	std::cout<<"BLUE : 0-255 > ";
	std::cin>>blueValue;
	std::vector<int> HUDcolor {redValue, greenValue, blueValue};

	play_pong(screenMode, fgColor, HUDcolor);	//screenMode : =0 (4*4 cells), =1 (4*2 cells)
	return EXIT_SUCCESS;
}



int play_pong(int const& screenMode, std::vector<int> const& fgColor, std::vector<int> const& HUDcolor){
	//screenMode : =0 (4*4 cells), =1 (4*2 cells)

	std::ofstream myFile;
	myFile.open ("ball.txt");

	initscr();
	clear();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	cbreak();


	printw("*** Pong game ***\n");
	printw("- Left player:\t[e]:up\t\t- Right player:\t[UP]:up\n");
	printw("\t\t[c]:down\t\t \t[DOWN]:down\n");
	printw("- Other:\t[q]:exit\n");
	refresh();
	bool gameRunning (true);
	int c (0);


	/* Initializing positions */
	std::vector<int> playerPos (2);
	std::vector<double> ballPosAngle (3);	//Contains the pos(x;y) and the angle with the X-Axis
	std::vector<int> score {0,0};
	switch (screenMode){
		case 0:	//4*4 cells
			playerPos[0]=13;	playerPos[1]=13;
			break;
		case 1:	//4*2 cells
			playerPos[0]=6;	playerPos[1]=6;
			break;
		default:
			std::cout<<"Could not initialize players : screenMode = "<<screenMode<<std::endl;
			break;
	}
	pongInitBall(ballPosAngle, screenMode);
	std::cout<<ballPosAngle[0]<<";"<<ballPosAngle[1]<<";"<<ballPosAngle[2]*180/PI<<"."<<std::endl;


	int dTime(0);	//Used to regulate the framerate (FPS constant)
	int prevTime = millis();

	while (gameRunning){
		/*Regulate frame rate*/
		while(dTime <= (int)(1000/FPS)){
			delay((int)(100/FPS));
			dTime = millis()-prevTime;
		}

		/*Inputs from players*/
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

		/*Move the ball*/
		pongMoveBall(ballPosAngle, playerPos, screenMode, dTime);	//Move the ball according to the delay between two frames
		myFile<<floor(ballPosAngle[0])<<"\t"<<floor(ballPosAngle[1])<<"\t"<<ballPosAngle[2]*180/PI<<std::endl;

		if(calcScore(ballPosAngle, score)){	//If a new point has been made...
			delay(500);
			pongInitBall(ballPosAngle, screenMode);	//New ball

			if (score[0]==10||score[1]==10){	//If someone won, stop the game
				gameRunning = false;
			}
		}

		pongDisplay(ballPosAngle, playerPos, score, screenMode, fgColor, HUDcolor);

		dTime = 0;
		prevTime = millis();

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
		}else{				//down
			
			if (mode==0){	//4*4 cells
				if(playerPos[0]>=23){	playerPos[0]=23;
				}else{	playerPos[0] = playerPos[0]+1;
				}
			}else{			//4*2 cells
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
	
			}else{			//4*2 cells
				if(playerPos[1]>=9){	playerPos[1]=9;
				}else{	playerPos[1] = playerPos[1]+1;
				}
			}
		}
	}
	printw("LP : %d\tRP : %d\n", playerPos[0], playerPos[1]);	//Debug message displaying coordinates of the players
	return EXIT_SUCCESS;
}



int pongInitBall(std::vector<double> & ballPosAngle, int const& screenMode){
	std::random_device rd{};

	ballPosAngle[0] = 15+(rd()%2);	//X-Axis

	switch (screenMode){			//Y-Axis
		case 0:	//4*4 cells
			ballPosAngle[1] = 15+(rd()%2);
			break;
		case 1:	//4*2 cells
			ballPosAngle[1] = 7+(rd()%2);
			break;
		default:
			std::cout<<"Could not initialize ball : screeMode = "<<screenMode<<std::endl;
			break;
	}

	//Angle in degrees, easier to generate than in rad:
	int angle = rd()%360;
	while (angle==90||angle==180||angle==270){	//We do not want a vertical ball nor horizontal,
												//	so please find an angle != vertical && != horizontal
		angle = rd()%360;
	}
	ballPosAngle[2] = angle*(PI/180.0);	//Now, angle in rad!


	return EXIT_SUCCESS;
}



int pongMoveBall(std::vector<double> & ballPosAngle, std::vector<int> const& playerPos, int const& screenMode, int const& dTime){
	double SPEED (50.0);	//Arbitrary fixed speed value of 20 pixels per second. Because I can.

	double futureX = SPEED*((double)(dTime)/1000) * cos(ballPosAngle[2]) + ballPosAngle[0];
	double futureY = SPEED*((double)(dTime)/1000) * sin(ballPosAngle[2]) + ballPosAngle[1];

	//Future positions ON THE SCREEN, so interger values !
	int futurePosX = (int) floor(futureX);
	int futurePosY = (int) floor(futureY);


	/* Collision on the Y-Axis */
	if (futurePosY<=0){
							//Bounce on the upper limit : y+1
		ballPosAngle[1] = ballPosAngle[1];	//Reflect
		ballPosAngle[2] = -1.0* ballPosAngle[2];

	}else if(futurePosY >= 15){	//>=15 or >= 31

		switch(screenMode){	//Bounce on the lower limit : y-1
			case 0:	//4*4 cells
				if (futurePosY >= 31){
					ballPosAngle[1] = ballPosAngle[1];	//Reflect
					ballPosAngle[2] = -1.0* ballPosAngle[2];
				}else{ballPosAngle[1]	= futureY;}
				break;
			case 1:	//4*2 cells
				if (futurePosY >= 15){
					ballPosAngle[1] = ballPosAngle[1];	//Reflect
					ballPosAngle[2] = -1.0* ballPosAngle[2];
				}else{ballPosAngle[1]	= futureY;}
				break;
		}
	}else{//Not on borders: cannot bounce:
		ballPosAngle[1]	= futureY;
	}


	bool hitPaddle(false);
	/* Collision on the X-Axis */
	if (futurePosX <= 1 && ballPosAngle[0]>2){		//Left paddle

		switch(screenMode){
			case 0:	//4*4 cells, paddles are 6px wide:
				for (int y(0); y<6; y++){
					if (y+playerPos[0] == futurePosY){	//If a pixel of the paddle meet the ball : collision and bounce
						ballPosAngle[0] = 2+(futureX-futurePosX);	//Reflect position
						ballPosAngle[2] = PI - ballPosAngle[2];
						std::cout<<"bounce left"<<std::endl;
						hitPaddle = true;
						break;
					}
				}
				break;
			case 1:	//4*2 cells, paddles are 4px wide:
				for (int y(0); y<4; y++){
					if (y+playerPos[0] == futurePosY){	//If a pixel of the paddle meet the ball : collision and bounce
						ballPosAngle[0] = 2+(futureX-futurePosX);	//Reflect position
						ballPosAngle[2] = PI - ballPosAngle[2];
						std::cout<<"bounce left"<<std::endl;
						hitPaddle = true;
						break;
					}
				}
				break;
			default:
				break;
		}

		if (!hitPaddle){
			ballPosAngle[0]	= futureX;
		}



	}else if (futurePosX >= 30 && ballPosAngle[0]<30){		//Right paddle

		switch(screenMode){
			case 0:	//4*4 cells, paddles are 6px wide:
				for (int y(0); y<6; y++){
					if (y+playerPos[0] == futurePosY){	//If a pixel of the paddle meet the ball : collision and bounce
						ballPosAngle[0] = 30-(futureX-futurePosX);
						ballPosAngle[2] = PI - ballPosAngle[2];
						std::cout<<"bounce right"<<std::endl;
						hitPaddle = true;
						break;
					}
				}
				break;
			case 1:	//4*2 cells, paddles are 4px wide:
				for (int y(0); y<4; y++){
					if (y+playerPos[0] == futurePosY){	//If a pixel of the paddle meet the ball : collision and bounce
						ballPosAngle[0] = 30-(futureX-futurePosX);
						ballPosAngle[2] = PI - ballPosAngle[2];
						std::cout<<"bounce right"<<std::endl;
						hitPaddle = true;
						break;
					}
				}
				break;
			default:
				break;
		}
		if (!hitPaddle){
			ballPosAngle[0]	= futureX;
		}

	}else{	//Not in the zone of paddles : can not bounce
		ballPosAngle[0]	= futureX;
	}

	if (ballPosAngle[0]<0){	ballPosAngle[0]=0;	}	//Do not exceed capacity of the display ! (out of range, etc.)
	if (ballPosAngle[0]>31){	ballPosAngle[0]=31;	}


	return EXIT_SUCCESS;
}



int pongDisplay(std::vector<double> const& ballPosAngle, std::vector<int> const& playerPos, std::vector<int> const& score,
				int const& screenMode, std::vector<int> const& fgColor, std::vector<int> const& HUDcolor){
	std::vector<int> BLACK {0,0,0};

	piLock(DATAS_KEY);

	for (auto line: DATAS){	//Reset blank screen
		for (auto pixel: line){
			pixel = BLACK;
		}
	}

	switch (screenMode){
		case 0:	//4*4 cells
			drawScore(score, HUDcolor);	//Draw score BELOW the ball
			for (unsigned int i (0); i<SIZE_X*8; i++){	//Draw top and bottom borders
				DATAS[0][i] = fgColor;
				DATAS[31][i] = fgColor;
			}
			for (unsigned int i (0); i<32; i+=2){	//Draw net
				DATAS[i][16] = fgColor;
				DATAS[i+1][15] = fgColor;
			}
			for (unsigned int i (0); i<5; i++){	//Draw players
				DATAS[ playerPos[0]+i ][1] = fgColor;
				DATAS[ playerPos[1]+i ][30] = fgColor;
			}

			DATAS[ (int)floor(ballPosAngle[1]) ][ (int)floor(ballPosAngle[0]) ] = fgColor;	//Draw ball

			break;

		case 1:	//4*2 cells
			for (unsigned int i(0); i<SIZE_X*8; i++){	//draw top and bottom borders
				DATAS[0][i] = fgColor;
				DATAS[15][i] = fgColor;
			}

			for (int i (0); i<score[0]; i++){	DATAS[0][14-i] = HUDcolor;	}	//Draw scores on the top border
			for (int i (0); i<score[1]; i++){	DATAS[0][17+i] = HUDcolor;	}
			DATAS[0][4] = HUDcolor;	DATAS[0][27] = HUDcolor;	//Draw pixel indicating the limit score

			for (unsigned int i (0); i<15; i+=2){	//Draw net
				DATAS[i][16] = fgColor;
				DATAS[i+1][15] = fgColor;
			}
			for (unsigned int i (0); i<5; i++){	//Draw players
				DATAS[ playerPos[0]+i ][1] = fgColor;
				DATAS[ playerPos[1]+i ][30] = fgColor;
			}
			DATAS[ (int)floor(ballPosAngle[1]) ][ (int)floor(ballPosAngle[0]) ] = fgColor;	//Draw ball
			break;
		default:
			break;
	}
	piUnlock(DATAS_KEY);
	return EXIT_SUCCESS;
}



bool calcScore(std::vector<double> const& ballPosAngle, std::vector<int> & score){
/*Returns true if a new point has been made*/
	bool newPoint = false;
	if (ballPosAngle[0]<1 && score[1]<10){
		score[1]=score[1]+1;
		newPoint = true;
	}
	if (ballPosAngle[0]>30 && score[0]<10){
		score[0]=score[0]+1;
		newPoint = true;
	}

	return newPoint;
}



int drawScore(std::vector<int> const& scores, std::vector<int> const& HUDcolor){
	/* NOT DONE YET */
	int x (10);	//sPX : Start Position on the X-Axis
	int gap(0);

	for (unsigned int i(0); i<scores.size(); i++){	//Draw for both players
		gap = i*9;
		switch(scores[i]){
			case 0:	//Draw 0
				DATAS[x+gap][2] = HUDcolor;	DATAS[x+1+gap][2] = HUDcolor;	DATAS[x+2+gap][2] = HUDcolor;
				DATAS[x+gap][3] = HUDcolor;									DATAS[x+2+gap][3] = HUDcolor;
				DATAS[x+gap][4] = HUDcolor;									DATAS[x+2+gap][4] = HUDcolor;
				DATAS[x+gap][5] = HUDcolor;									DATAS[x+2+gap][5] = HUDcolor;
				DATAS[x+gap][6] = HUDcolor;	DATAS[x+1+gap][6] = HUDcolor;	DATAS[x+2+gap][6] = HUDcolor;
				break;
			case 1:	//Draw 1
																			DATAS[x+2+gap][2] = HUDcolor;
																			DATAS[x+2+gap][3] = HUDcolor;
																			DATAS[x+2+gap][4] = HUDcolor;
																			DATAS[x+2+gap][5] = HUDcolor;
																			DATAS[x+2+gap][6] = HUDcolor;
				break;
			case 2:	//Draw 2
				DATAS[x+gap][2] = HUDcolor;	DATAS[x+1+gap][2] = HUDcolor;	DATAS[x+2+gap][2] = HUDcolor;
																			DATAS[x+2+gap][3] = HUDcolor;
				DATAS[x+gap][4] = HUDcolor;	DATAS[x+1+gap][4] = HUDcolor;	DATAS[x+2+gap][4] = HUDcolor;
				DATAS[x+gap][5] = HUDcolor;									DATAS[x+2+gap][5] = HUDcolor;
				DATAS[x+gap][6] = HUDcolor;	DATAS[x+1+gap][6] = HUDcolor;	DATAS[x+2+gap][6] = HUDcolor;
				break;
			case 3:	//Draw 3
				DATAS[x+gap][2] = HUDcolor;	DATAS[x+1+gap][2] = HUDcolor;	DATAS[x+2+gap][2] = HUDcolor;
																			DATAS[x+2+gap][3] = HUDcolor;
											DATAS[x+1+gap][4] = HUDcolor;	DATAS[x+2+gap][4] = HUDcolor;
																			DATAS[x+2+gap][5] = HUDcolor;
				DATAS[x+gap][6] = HUDcolor;	DATAS[x+1+gap][6] = HUDcolor;	DATAS[x+2+gap][6] = HUDcolor;
				break;
			case 4:	//Draw 4
				DATAS[x+gap][2] = HUDcolor;									DATAS[x+2+gap][2] = HUDcolor;
				DATAS[x+gap][3] = HUDcolor;									DATAS[x+2+gap][3] = HUDcolor;
				DATAS[x+gap][4] = HUDcolor;	DATAS[x+1+gap][4] = HUDcolor;	DATAS[x+2+gap][4] = HUDcolor;
																			DATAS[x+2+gap][5] = HUDcolor;
																			DATAS[x+2+gap][6] = HUDcolor;
				break;
			case 5:	//Draw 5
				DATAS[x+gap][2] = HUDcolor;	DATAS[x+1+gap][2] = HUDcolor;	DATAS[x+2+gap][2] = HUDcolor;
				DATAS[x+gap][3] = HUDcolor;
				DATAS[x+gap][4] = HUDcolor;	DATAS[x+1+gap][4] = HUDcolor;	DATAS[x+2+gap][4] = HUDcolor;
																			DATAS[x+2+gap][5] = HUDcolor;
				DATAS[x+gap][6] = HUDcolor;	DATAS[x+1+gap][6] = HUDcolor;	DATAS[x+2+gap][6] = HUDcolor;
				break;
			case 6:	//Draw 6
				DATAS[x+gap][2] = HUDcolor;	DATAS[x+1+gap][2] = HUDcolor;	DATAS[x+2+gap][2] = HUDcolor;
				DATAS[x+gap][3] = HUDcolor;
				DATAS[x+gap][4] = HUDcolor;	DATAS[x+1+gap][4] = HUDcolor;	DATAS[x+2+gap][4] = HUDcolor;
				DATAS[x+gap][5] = HUDcolor;									DATAS[x+2+gap][5] = HUDcolor;
				DATAS[x+gap][6] = HUDcolor;	DATAS[x+1+gap][6] = HUDcolor;	DATAS[x+2+gap][6] = HUDcolor;
				break;
			case 7:	//Draw 7
				DATAS[x+gap][2] = HUDcolor;	DATAS[x+1+gap][2] = HUDcolor;	DATAS[x+2+gap][2] = HUDcolor;
																			DATAS[x+2+gap][3] = HUDcolor;
																			DATAS[x+2+gap][4] = HUDcolor;
																			DATAS[x+2+gap][5] = HUDcolor;
																			DATAS[x+2+gap][6] = HUDcolor;
				break;
			case 8:	//Draw 8
				DATAS[x+gap][2] = HUDcolor;	DATAS[x+1+gap][2] = HUDcolor;	DATAS[x+2+gap][2] = HUDcolor;
				DATAS[x+gap][3] = HUDcolor;									DATAS[x+2+gap][3] = HUDcolor;
				DATAS[x+gap][4] = HUDcolor;	DATAS[x+1+gap][4] = HUDcolor;	DATAS[x+2+gap][4] = HUDcolor;
				DATAS[x+gap][5] = HUDcolor;									DATAS[x+2+gap][5] = HUDcolor;
				DATAS[x+gap][6] = HUDcolor;	DATAS[x+1+gap][6] = HUDcolor;	DATAS[x+2+gap][6] = HUDcolor;
				break;
			case 9:	//Draw 9
				DATAS[x+gap][2] = HUDcolor;	DATAS[x+1+gap][2] = HUDcolor;	DATAS[x+2+gap][2] = HUDcolor;
				DATAS[x+gap][3] = HUDcolor;									DATAS[x+2+gap][3] = HUDcolor;
				DATAS[x+gap][4] = HUDcolor;	DATAS[x+1+gap][4] = HUDcolor;	DATAS[x+2+gap][4] = HUDcolor;
																			DATAS[x+2+gap][5] = HUDcolor;
																			DATAS[x+2+gap][6] = HUDcolor;
				break;
			default:
				break;
		}
	}
	return EXIT_SUCCESS;
}



int M_settings(){
	unsigned int choice (0);
	while(choice<1 || choice>5){
		choice=0;
		std::cout<<"\t*** CALIBRATION MENU ***\n"<<std::endl;
		std::cout<<"PWM duty\t"<<"0%"<<"\t"<<"25%"<<"\t"<<"50%"<<"\t"<<"75%"<<std::endl;
		std::cout<<"RED: \t[0]\t["<<RED_VALUES[0]<<"]\t["<<RED_VALUES[1]<<"]\t["<<RED_VALUES[2]<<"]"<<std::endl;
		std::cout<<"GREEN: \t[0]\t["<<GREEN_VALUES[0]<<"]\t["<<GREEN_VALUES[1]<<"]\t["<<GREEN_VALUES[2]<<"]"<<std::endl;
		std::cout<<"BLUE: \t[0]\t["<<BLUE_VALUES[0]<<"]\t["<<BLUE_VALUES[1]<<"]\t["<<BLUE_VALUES[2]<<"]"<<std::endl;
		std::cout<<"SCREEN DIMENSIONS : "<<SIZE_X<<" * "<<SIZE_Y<<" cells of 8*8 pixels\n"<<std::endl;
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
				std::cout<<"\n*BLUE: \t["<<BLUE_VALUES[0]<<"]\t["<<BLUE_VALUES[1]<<"]\t["<<BLUE_VALUES[2]<<"]\n"<<std::endl;
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
