#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <string>
#include <complex>
#include <valarray>
#include <math.h>

const double PI = 3.141592653589793238460 ;

/*using namespace std ;
using namespace sf ;*/

typedef std::complex<double> Complex;
typedef std::valarray<Complex> CArray;

class FFT
{
public:
	FFT(std::string const& _path, int const& _bufferSize);

	bool hammingWindow();
	void fft(CArray &x);
	bool update();

	void bars(float const& max);

	void draw(sf::RenderWindow &window);

private:
	std::string path;
	sf::SoundBuffer buffer;
	sf::Sound sound;

	std::vector<Complex> sample;
	std::vector<float> window;
	CArray bin;

	sf::VertexArray VA1 ;
	sf::VertexArray VA2 ;

	std::vector<sf::Vertex> cascade ;
	
	int sampleRate ;
	int sampleCount ;
	int bufferSize ;
	int mark ;
};

