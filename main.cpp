#include <stdlib.h>
#include <stdio.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>

#include <cstdio>
#include <unistd.h>				//Needed for I2C port
#include <fcntl.h>				//Needed for I2C port
#include <sys/ioctl.h>			//Needed for I2C port
#include <linux/i2c-dev.h>		//Needed for I2C port

#include <ncurses.h>

#include <SFML/Network.hpp>
#include <wiringPi.h>

#define DATAS_KEY	0
#define FPS		60
#define PI 3.14159265
#define I2C_ADDR 0x04	//I2C address of the slave

//Global variables = pure evil !
bool deamonDisplay (true);
std::vector<std::vector<std::vector<unsigned char>>> DATAS {};
int fps (0);
unsigned int SIZE_X (3);
unsigned int SIZE_Y (2);
unsigned int nbOfPWMcycles (8);
int file_i2c;	//Output of I2C
unsigned int nbOfBands (23);

std::vector<unsigned char> RED_VALUES {};// {64, 128, 192, 255};
std::vector<unsigned char> GREEN_VALUES {};// {64, 128, 192, 255};
std::vector<unsigned char> BLUE_VALUES {};// {64, 128, 192, 255};



/******** PROTOTYPES ********/
int askUser();
int sendLine(unsigned char const& color, unsigned char const& row, unsigned char const& cell);
int drawScreen();
void initDATAS();
int sendSpectrumBand(std::vector<unsigned char> const& data, unsigned char const& cellColumn);
int i2cSetup();
float map(float const& value, float const& a1, float const& a2, float const& b1, float const& b2);

int M_displayPatterns();

int M_spectrum();

int M_pong();
int play_pong(int const& screenMode, std::vector<unsigned char> const& fgColor, std::vector<unsigned char> const& HUDcolor);
int pongMovePlayer(int const& player, int const& direction, std::vector<int> & playerPos, int const& mode);
int pongInitBall(std::vector<double> & ballPosAngle, int const& screenMode);
int pongMoveBall(std::vector<double> & ballPosAngle, std::vector<int> const& playerPos, int const& screenMode, int const& dTime);
int pongDisplay(std::vector<double> const& ballPosAngle, std::vector<int> const& playerPos, std::vector<int> const& score,
				int const& screenMode, std::vector<unsigned char> const& fgColor, std::vector<unsigned char> const& HUDcolor);
bool calcScore(std::vector<double> const& ballPosAngle, std::vector<int> & score, int const& screenMode);
int drawScore(std::vector<int> const& score, std::vector<unsigned char> const& HUDcolor);

int M_settings();



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
		prevTimeFrame = prevTimeSec;

		while(dTimeSec <= 1000 && deamonDisplay){
			while(dTimeFrame <= (int)(1000/FPS)){	//Forces non-constant update of the screen by introducing delay.
				delay((int)(100/FPS));
				dTimeFrame = millis() - prevTimeFrame;	//Updates duration of the frame.
			}

			nbScreens +=1;
			if(drawScreen()==EXIT_FAILURE){	//Draws a frame in several PWM cycles.
				deamonDisplay = false;
			}

			dTimeFrame = 0;	//Resets duration of the frame.
			prevTimeFrame = millis();
			dTimeSec = millis() - prevTimeSec;	//Updates duration of a second.
		}

		fps = nbScreens;	//Updates FPS variable.
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

	i2cSetup();

	piLock(DATAS_KEY);
	initDATAS();	//Fills the DATAS table with default pattern
	piUnlock(DATAS_KEY);

/*
	int x = piThreadCreate(deamonLED);	//Starts the display
	if (x!=0){
		std::cout<<"Thread didnt start"<<std::endl;
	}
*/	

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

	deamonDisplay = false;
	delay(100);
	return EXIT_SUCCESS;
}




int i2cSetup(){
	//----- OPEN THE I2C BUS -----
	char *filename = (char*)"/dev/i2c-1";
	if ((file_i2c = open(filename, O_RDWR)) < 0)
	{
		//ERROR HANDLING: you can check errno to see what went wrong
		std::cout<<"Failed to open the i2c bus"<<std::endl;
		return EXIT_FAILURE;
	}
	
	if (ioctl(file_i2c, I2C_SLAVE, I2C_ADDR) < 0)
	{
		std::cout<<"Failed to acquire bus access and/or talk to slave.\n"<<std::endl;
		//ERROR HANDLING; you can check errno to see what went wrong
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}



int sendLine(unsigned char const& color, unsigned char const& row, unsigned char const& cell){
	/* Send a line of pixels through I2C to the STM32
	   [ COLOR , ROW , CELL , 0-255 , 0-255 , 0-255 , 0-255 , 0-255 , 0-255 , 0-255 , 0-255]
	*/

	ssize_t length = 11;
	unsigned char buffer[11] = {0};

	buffer[0] = color; buffer[1] = row; buffer[2] = cell;	// First 3 bytes required by the code on the STM32 to know which pixels to update

	for (unsigned int column (0); column<8; column++){
		buffer[3+column] = DATAS[row][cell*8 + column][color];	// The next 8 bytes of a row in one cell
	}

	if (write(file_i2c, buffer, length) != length){		//write() returns the number of bytes actually written
		/* ERROR HANDLING: i2c transaction failed */
		printf("Failed to write to the i2c bus.\n");
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}



int sendSpectrumBand(std::vector<unsigned char> const& data, unsigned char const& cellColumn){
	/* Send a portion of audio spectrum through I2C to the STM32
	   [ IS_SPECTRUM_DATA , *empty_byte* , CELL_COLUMN , 0-255 , 0-255 , 0-255 , 0-255 , 0-255 , 0-255 , 0-255 , 0-255]
	*/
	ssize_t length = 11;
	unsigned char buffer[11] = {0};

	buffer[0] = 7;	buffer[1] = 0;	buffer[2] = cellColumn;	// Must be between |[ 0 ; SIZE_X ]|
	for(unsigned int i(0); i < 8; i++){
		buffer[3+i] = data[i];
		//std::cout<<(int)data[i]<<std::endl;
	}

	if (write(file_i2c, buffer, length) != length){
		/* ERROR HANDLING: i2c transaction failed */
		std::cout<<"Failed to write to the i2c bus.\n"<<std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}



int drawScreen() {
	/* Send red, green and blue lines through I2C to the STM32 */

	for (unsigned int row (0); row<DATAS.size(); row++){
		for (unsigned int cell (0); cell<SIZE_X; cell++){
			if (sendLine(0, row, cell)==EXIT_FAILURE){ return EXIT_FAILURE;	}
			if (sendLine(1, row, cell)==EXIT_FAILURE){ return EXIT_FAILURE;	}
			if (sendLine(2, row, cell)==EXIT_FAILURE){ return EXIT_FAILURE;	}
		}
 	}
	return EXIT_SUCCESS;
}



void initDATAS(){
	/* Set a test pattern as the frame (bayer mosaic)*/

	std::vector<unsigned char> red {255,0,0};
	std::vector<unsigned char> green{0,255,0};
	std::vector<unsigned char> blue {0,0,255};
	std::vector<std::vector<unsigned char>> lineA {};
	std::vector<std::vector<unsigned char>> lineB {};
	for (unsigned int pixel (0); pixel < SIZE_X*8; pixel += 2) {
		lineA.push_back(red);
		lineA.push_back(green);
	}
	for (unsigned int pixel (0); pixel < SIZE_X*8; pixel += 2) {
		lineB.push_back(green);
		lineB.push_back(blue);
	}

	for (unsigned int line (0); line < SIZE_Y*8; line+=2) {
		DATAS.push_back(lineA);
		DATAS.push_back(lineB);
	}

	/* Initialization of red, green and blue informations */ 
	for (unsigned int PWMlevel (0); PWMlevel<nbOfPWMcycles; PWMlevel++){
		RED_VALUES.push_back((255/nbOfPWMcycles)*(PWMlevel+1));
		GREEN_VALUES.push_back((255/nbOfPWMcycles)*(PWMlevel+1));
		BLUE_VALUES.push_back((255/nbOfPWMcycles)*(PWMlevel+1));
	}
	RED_VALUES = std::vector<unsigned char> {31, 63, 95, 127, 159, 191, 223, 255};
	GREEN_VALUES = std::vector<unsigned char> {63, 95, 127, 159, 191, 223, 255, 255};
	BLUE_VALUES = std::vector<unsigned char> {95, 127, 159, 191, 223, 255, 255, 255};
}



std::vector<unsigned char> HSVtoRGB(std::vector<float> const& HSV){
	float H = HSV[0];	float S = HSV[1];	float V = HSV[2];
    if(H>360 || H<0 || S>100 || S<0 || V>100 || V<0){
        std::cout<<"The givem HSV values are not in valid range"<<std::endl;
        return std::vector<unsigned char> {};
    }
    float s = S/100;
    float v = V/100;
    float C = s*v;
    float X = C*( 1-abs(fmod(H/60.0, 2)-1) );
    float m = v-C;
    float r, g, b;

    if(H >= 0 && H < 60){			r = C, g = X, b = 0;
    }else if(H >= 60 && H < 120){	r = X, g = C, b = 0;
    }else if(H >= 120 && H < 180){	r = 0, g = C, b = X;
    }else if(H >= 180 && H < 240){	r = 0, g = X, b = C;
    }else if(H >= 240 && H < 300){	r = X, g = 0, b = C;
    }else{							r = C, g = 0, b = X;
    }

    unsigned char R = (r+m)*255;
    unsigned char G = (g+m)*255;
    unsigned char B = (b+m)*255;

	return std::vector<unsigned char> {R,G,B};
}



float map(float const& value, float const& a1, float const& a2, float const& b1, float const& b2){
	float start_ratio = (value - a1)/(a2 - a1);
	return (start_ratio * (b2-b1) + b1);
}



int M_displayPatterns() {
	/* NOT DONE YET */
	return EXIT_SUCCESS;
}



int M_spectrum() {

	sf::UdpSocket socket;
	socket.setBlocking(false);

	if (socket.bind(54000) != sf::Socket::Done) {
		std::cout<<"***** Can't open UDP socket on port 54000"<<std::endl;
		return EXIT_FAILURE;
	}

	char data[24];
	std::size_t received;
	std::vector<float> HSV {0,0,0};

	sf::IpAddress sender("127.0.0.1");
	unsigned short sender_port (36000);

	initscr();
	clear();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	cbreak();


	printw("Receiving...\n");
	printw("[q]:exit\n");
	refresh();

	bool keepReceiving (true);
	int c (0);
	std::vector<unsigned char> RGB (3);
	std::vector<unsigned char> datas_per_cell (8);
	while(keepReceiving){
		c = getch();
		if(c==113) keepReceiving = false; // key q pressed : quit


		if (socket.receive(data, 24, received, sender, sender_port) == sf::Socket::Done) {
			//piLock(DATAS_KEY);
			
			for (unsigned char cell (0); cell < SIZE_X; cell++){
				for (unsigned char i(0); i<8; i++){
					datas_per_cell[i] = data[cell*8+i];
				}
				sendSpectrumBand(datas_per_cell, cell);
			}
		}

	}
	endwin();
	return EXIT_SUCCESS;
}



int M_pong() {
	std::cout<<"\n*** Pong game ***"<<std::endl;

	unsigned int choiceDisp (0);
	int screenMode (0);
	while (choiceDisp<1 || choiceDisp>3) {
		choiceDisp = 0;
		std::cout<<" * Choose display"<<std::endl;
		std::cout<<"1 - 3*2 cells"<<std::endl;

		if (SIZE_Y==4){	//Display the other choice only if available
			std::cout<<"2 - 4*4 cells"<<std::endl;
		}

		std::cout<<"3 - EXIT"<<std::endl;
		std::cout<<"> ";
		//std::cin>>choiceDisp;	choiceDisp = (unsigned int)choiceDisp;
		choiceDisp=1;
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
	unsigned int temp (0);
	std::cout<<" * Choose foreground color\n"<<std::endl;
	std::cout<<"RED : 0-255 > ";
	unsigned char redValue  (255);
	//std::cin>>temp;	redValue = (unsigned char)temp;
	std::cout<<"GREEN : 0-255 > ";
	unsigned char greenValue (255);
	//std::cin>>temp;	greenValue = (unsigned char)temp;
	std::cout<<"BLUE : 0-255 > ";
	unsigned char blueValue (255);
	//std::cin>>temp;	blueValue = (unsigned char)temp;
	std::vector<unsigned char> fgColor {redValue, greenValue, blueValue};

	std::cout<<" * Choose HUD color\n"<<std::endl;
	std::cout<<"RED : 0-255 > ";
	//std::cin>>temp;	redValue = (unsigned char)temp;
	std::cout<<"GREEN : 0-255 > ";
	//std::cin>>temp;	greenValue = (unsigned char)temp;
	std::cout<<"BLUE : 0-255 > ";
	//std::cin>>temp;	blueValue = (unsigned char)temp;
	std::vector<unsigned char> HUDcolor {redValue, greenValue, blueValue};

	play_pong(screenMode, fgColor, HUDcolor);	//screenMode : =0 (4*4 cells), =1 (3*2 cells)
	return EXIT_SUCCESS;
}



int play_pong(int const& screenMode, std::vector<unsigned char> const& fgColor, std::vector<unsigned char> const& HUDcolor){
	//screenMode : =0 (4*4 cells), =1 (3*2 cells)

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
		case 1:	//3*2 cells
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

		if(calcScore(ballPosAngle, score, screenMode)){	//If a new point has been made...
			delay(1500);
			flushinp();
			pongInitBall(ballPosAngle, screenMode);	//New ball

			switch (screenMode){	//Reset player positions
				case 0:	//4*4 cells
					playerPos[0]=13;	playerPos[1]=13;
					break;
				case 1:	//3*2 cells
					playerPos[0]=6;	playerPos[1]=6;
					break;
				default:
					std::cout<<"Could not initialize players : screenMode = "<<screenMode<<std::endl;
					break;
			}

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
			}else{			//3*2 cells
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
	
			}else{			//3*2 cells
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
	std::cout<<"*new ball!"<<std::endl;
	switch (screenMode){
		case 0:	//4*4 cells
			ballPosAngle[0] = 15+(rd()%2);	//X-Axis
			ballPosAngle[1] = 15+(rd()%2);	//Y-Axis
			break;
		case 1:	//3*2 cells
			ballPosAngle[0] = 11+(rd()%2);
			ballPosAngle[1] = 7+(rd()%2);
			break;
		default:
			std::cout<<"Could not initialize ball : screeMode = "<<screenMode<<std::endl;
			break;
	}

	//Angle in degrees, easier to generate than in rad:
	int angle = rd()%360;
	while ((angle>80&&angle<100)||(angle>170&&angle<190)||(angle>260&&angle<280)||(angle>350&&angle<10)){	//We do not want a vertical ball nor horizontal,
												//	so please find an angle != vertical && != horizontal
		angle = rd()%360;
	}
	ballPosAngle[2] = angle*(PI/180.0);	//Now, angle in rad!


	return EXIT_SUCCESS;
}



int pongMoveBall(std::vector<double> & ballPosAngle, std::vector<int> const& playerPos, int const& screenMode, int const& dTime){
	double SPEED (60.0);	//Arbitrary fixed speed value of 10 pixels per second. Because I can.

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
			case 1:	//3*2 cells
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
	if (futurePosX <= 1 && ballPosAngle[0]>=2){		//Left paddle

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
			case 1:	//3*2 cells, paddles are 4px wide:
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



		
	}else{//	If it didn't hit the left paddle, maybe the right one ?
		if(futurePosX >= 30 and ballPosAngle[0]<=30 and screenMode==0){	//4*4 cells, paddles are 6px wide:
			for (int y(0); y<6; y++){
				if (y+playerPos[0] == futurePosY){	//If a pixel of the paddle meet the ball : collision and bounce
					ballPosAngle[0] = 30-(futureX-futurePosX);
					ballPosAngle[2] = PI - ballPosAngle[2];
					std::cout<<"bounce right"<<std::endl;
					hitPaddle = true;
					break;
				}
			}

		}else if(futurePosX >= 22 and ballPosAngle[0]<=22 and screenMode==1){	//3*2 cells, paddles are 4px wide:
			for (int y(0); y<4; y++){
				if (y+playerPos[0] == futurePosY){	//If a pixel of the paddle meet the ball : collision and bounce
					ballPosAngle[0] = 30-(futureX-futurePosX);
					ballPosAngle[2] = PI - ballPosAngle[2];
					std::cout<<"bounce right"<<std::endl;
					hitPaddle = true;
					break;
				}
			}
		}else{	//Not in the zone of paddles : can not bounce
			ballPosAngle[0]	= futureX;
		}

		if (!hitPaddle){
			ballPosAngle[0]	= futureX;
		}
	}

	//Do not exceed capacity of the display ! (out of range, etc.)
	if (ballPosAngle[0]<0){	ballPosAngle[0]=0;	}	//On the left edge...

	if (ballPosAngle[0]>31 && screenMode==0){	ballPosAngle[0]=31;	// ... and on the right edge
	}else if(ballPosAngle[0]>23 && screenMode==1){	ballPosAngle[0]=23;	}

	return EXIT_SUCCESS;
}



int pongDisplay(std::vector<double> const& ballPosAngle, std::vector<int> const& playerPos, std::vector<int> const& score,
				int const& screenMode, std::vector<unsigned char> const& fgColor, std::vector<unsigned char> const& HUDcolor){
	std::vector<unsigned char> BLACK {0,0,0};

	//piLock(DATAS_KEY);

	for (unsigned int line (0); line<DATAS.size(); line++){	//Reset blank screen
		for (unsigned int pixel (0); pixel<DATAS[0].size(); pixel++){
			DATAS[line][pixel] = BLACK;
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
			for (unsigned int i (0); i<6; i++){	//Draw players (6px long)
				DATAS[ playerPos[0]+i ][1] = fgColor;
				DATAS[ playerPos[1]+i ][30] = fgColor;
			}

			DATAS[ (int)floor(ballPosAngle[1]) ][ (int)floor(ballPosAngle[0]) ] = fgColor;	//Draw ball

			break;

		case 1:	//3*2 cells
			for (unsigned int i(0); i<SIZE_X*8; i++){	//draw top and bottom borders
				DATAS[0][i] = fgColor;
				DATAS[15][i] = fgColor;
			}

			for (int i (0); i<score[0]; i++){	DATAS[0][9-i] = HUDcolor;	}	//Draw scores on the top border
			for (int i (0); i<score[1]; i++){	DATAS[0][14+i] = HUDcolor;	}
			//DATAS[0][4] = HUDcolor;	DATAS[0][27] = HUDcolor;	//Draw pixel indicating the limit score

			for (unsigned int i (0); i<15; i+=2){	//Draw net
				DATAS[i][12] = fgColor;
				DATAS[i+1][11] = fgColor;
			}
			for (unsigned int i (0); i<4; i++){	//Draw players (4px long)
				DATAS[ playerPos[0]+i ][1] = fgColor;
				DATAS[ playerPos[1]+i ][22] = fgColor;
			}
			DATAS[ (int)floor(ballPosAngle[1]) ][ (int)floor(ballPosAngle[0]) ] = fgColor;	//Draw ball
			break;
		default:
			break;
	}
	//piUnlock(DATAS_KEY);
	drawScreen();
	return EXIT_SUCCESS;
}



bool calcScore(std::vector<double> const& ballPosAngle, std::vector<int> & score, int const& screenMode){
/*Returns true if a new point has been made*/
	bool newPoint = false;

	if (ballPosAngle[0]<1 && score[1]<10){	//Point for the left player
		score[1]=score[1]+1;
		newPoint = true;
	}

	switch(screenMode){	//Point for the right player
		case 0:	//4*4 cells
			if (ballPosAngle[0]>30 && score[0]<10){
				score[0]=score[0]+1;
				newPoint = true;
			}
		break;
		case 1:	//3*4 cells
			if (ballPosAngle[0]>22 && score[0]<10){
				score[0]=score[0]+1;
				newPoint = true;
			}
		break;
		default:
		break;
	}

	return newPoint;
}



int drawScore(std::vector<int> const& scores, std::vector<unsigned char> const& HUDcolor){
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
