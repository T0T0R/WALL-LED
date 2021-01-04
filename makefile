WallLed: 
	g++ -Wall -Wextra -O3 -s -std=c++11 -o WallLed main.cpp -lwiringPi -lpthread -lncurses -lsfml-network

clean:
	rm -f ./WallLed

n: clean
	g++ -Wall -Wextra -g -std=c++11 -o WallLed main.cpp -lwiringPi -lpthread -lncurses -lsfml-network

renew: n
	./WallLed