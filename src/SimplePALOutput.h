#pragma once
#include "driver/i2s.h"
#include "math.h"

#define lineSamples 854 // 854*75ns=64.050us 75ns to okres zegara 13.333333MHz
#define imageSamples 640
#define syncLevel 0
#define blankLevel 23
#define frameStart 170 // Im wieksze frameStart, tym bardziej obraz przesuniety w prawo. Musi byc parzyste
#define syncSamples 64 // 64*75ns=4.8us
#define burstSamples 38 // 64*75ns=2.85us
#define burstStart 70 // 70*75ns=5.25us
#define burstAmp 12
#define maxLevel 34 //34
#define burstPerSample  ((2 * M_PI) / (13333333 / 4433618.75))// 2.089232047699551
#define burstPhase  (M_PI / 4 * 3)//2.355 (M_PI/4 to 45stopni)
class SimplePALOutput
{
public:
	SimplePALOutput();
	void init();
	void sendFrame(char ***frame);
private:
	const i2s_port_t I2S_PORT = (i2s_port_t)I2S_NUM_0;
	unsigned short longSync[lineSamples];
	unsigned short shortSync[lineSamples];
	unsigned short mixSync[lineSamples];
	unsigned short line[2][lineSamples];
	unsigned short blank[lineSamples];
	short SIN[imageSamples];
	short COS[imageSamples];
	short YLUT[16];
	short UVLUT[16];
	void sendLine(unsigned short *l);
};
