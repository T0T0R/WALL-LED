WallLed: 
	g++ -Wall -Wextra -O3 -s -std=c++11 -o WallLed main.cpp ./FFT/*.cpp -lwiringPi -lpthread -lncurses -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system

clean:
	rm -f ./WallLed

n: clean
	g++ -Wall -Wextra -g -std=c++11 -o WallLed main.cpp ./FFT/*.cpp -lwiringPi -lpthread -lncurses -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system

renew: n
	./WallLed