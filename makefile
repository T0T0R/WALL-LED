WallLed: 
	g++ -Wall -Wextra -g -std=c++11 -o WallLed main.cpp ./FFT/*.cpp -lwiringPi -lpthread -lncurses -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system
