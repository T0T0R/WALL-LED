#pragma once

class ledPanel
{
public:
	ledPanel(std::vector<unsigned int> size, std::vector<unsigned int> pins);
	~ledPanel();

	int testLed(int red, int green, int blue);
	int askUser();
	int sendPackets(int nb);
	int drawScreen(int nbCycles, int nbCells);

protected:
	std::vector<unsigned int> m_size;
	unsigned int serialPin;
	unsigned int shiftClockPin;
	unsigned int memoryClockPin;
	unsigned int resetPin;
};

