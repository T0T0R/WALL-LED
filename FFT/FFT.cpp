#include "FFT.h"

FFT::FFT(std::string const& _path, int const& _bufferSize):
	path(_path), buffer()
{
	if(!buffer.loadFromFile(path)){	//Load file in buffer
		std::cout<<"Unable to load buffer"<<std::endl;
	}

	sound.setBuffer(buffer);
	sound.setLoop(false);
	sound.play();

	VA1.setPrimitiveType(sf::LinesStrip);
	VA2.setPrimitiveType(sf::Lines);

	sampleRate = buffer.getSampleRate()*buffer.getChannelCount();
	sampleCount = buffer.getSampleCount();

	if(_bufferSize < sampleCount){
		bufferSize = _bufferSize;
	}else{
		bufferSize = sampleCount;
	}
	mark = 0 ;

	for(int i(0) ; i < bufferSize ; i++){
		window.push_back(0.54-0.46*cos(2*PI*i/(float)bufferSize));
	}

	sample.resize(bufferSize);
	VA1.resize(bufferSize);
}



bool FFT::hammingWindow(){
	mark = sound.getPlayingOffset().asSeconds()*sampleRate;
	if (sound.getStatus()==sf::Sound::Stopped){	//If done playing
		return true;
	}

	if(mark+bufferSize < sampleCount){
		for(int i(mark) ; i < bufferSize+mark ; i++){
			sample[i-mark] = Complex(buffer.getSamples()[i] * window[i-mark], 0);
			VA1[i-mark] = sf::Vertex(sf::Vector2f(20,250) + sf::Vector2f((i-mark)/(float)bufferSize*700,sample[i-mark].real()*0.005), sf::Color(255,0,0,50));
		}
	}
	return false;
}




void FFT::fft(CArray &x){
	const int N = x.size();
	if(N <= 1){	return;	}

	CArray even = x[std::slice(0,N/2,2)];
	CArray  odd = x[std::slice(1,N/2,2)];

	fft(even);
	fft(odd);

	for(int k = 0 ; k < N/2 ; k++){
		Complex t = std::polar(1.0,-2 * PI * k / N) * odd[k];
		x[k] = even[k] + t;
		x[k+N/2] = even[k] - t;
	}
}




bool FFT::update(){
	bool isDone (false);
	isDone = hammingWindow();

	VA2.clear();

	bin = CArray(sample.data(), bufferSize);
	fft(bin);
	float max = 100000000;

	bars(max);
	return isDone;
}




void FFT::bars(float const& max){
	float MAX(20000.f);
	VA2.setPrimitiveType(sf::Lines);
	sf::Vector2f position(0,800);
	for(float i(3) ; i < std::min(bufferSize/2.f,MAX) ; i*=1.01){
		sf::Vector2f samplePosition(log(i)/log(std::min(bufferSize/2.f,MAX)),abs(bin[(int)i]));
		VA2.append(sf::Vertex(position+sf::Vector2f(samplePosition.x*800,-samplePosition.y/max*500), sf::Color::White));
		VA2.append(sf::Vertex(position+sf::Vector2f(samplePosition.x*800,0), sf::Color::White));
		VA2.append(sf::Vertex(position+sf::Vector2f(samplePosition.x*800,0), sf::Color(255,255,255,100)));
		VA2.append(sf::Vertex(position+sf::Vector2f(samplePosition.x*800,samplePosition.y/max*500/2.f), sf::Color(255,255,255,0)));
	}
}



void FFT::draw(sf::RenderWindow &window){
	//window.draw(VA1) ;
	//window.draw(VA2) ;
}
