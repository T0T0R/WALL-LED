#include <iostream>
#include <vector>
/*
#include <wiringPi.h>
#include <softPwm.h>
#include <wiringShift.h>
*/
#include "ledPanel.hpp"





ledPanel::ledPanel(std::vector<unsigned int> size,
				   std::vector<unsigned int> pins):
	m_size(size),
	serialPin(pins[0]), shiftClockPin(pins[1]), memoryClockPin(pins[2]), resetPin(pins[3])
{}


ledPanel::~ledPanel() {
}


int ledPanel::testLed(int red, int blue, int green) {
	/*
	digitalWrite(red, HIGH);
	digitalWrite(green, HIGH);
	digitalWrite(blue, HIGH);
	delayMicroseconds(1);
	digitalWrite(red, LOW);
	digitalWrite(green, LOW);
	digitalWrite(blue, LOW);
	delayMicroseconds(100);
	*/
	return EXIT_SUCCESS;
}



int ledPanel::askUser() {
	int number;
	std::cout<<"\nEnter a number: ";
	std::cin>> number;
	return number;
}



int ledPanel::sendPackets(int nb) {
	/*
	for (int i(0); i<nb; i++) {
		shiftOut(2, 7, MSBFIRST, DATAS);
	}
	*/
	return EXIT_SUCCESS;
}



int ledPanel::drawScreen(int nbCycles, int nbCells) {
	for (int i(0); i<nbCycles; i++) {
		sendPackets(nbCells*4);
	}
	return EXIT_SUCCESS;
}
