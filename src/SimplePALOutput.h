#pragma once
#include "driver/i2s.h"
#include "math.h"

const int lineSamples = 854; // 854*75ns=64.050us 75ns to okres zegara 13.333333MHz
const int imageSamples = 640;
const int syncLevel = 0;
const int blankLevel = 23;
const int frameStart = 170; // Im wieksze frameStart, tym bardziej obraz przesuniety w prawo. Musi byc parzyste
const int syncSamples = 64; // 64*75ns=4.8us
const int burstSamples = 38; // 64*75ns=2.85us
const int burstStart = 70; // 70*75ns=5.25us
const int burstAmp = 12;
const int maxLevel = 34; //34
const float burstPerSample = (2 * M_PI) / (13333333 / 4433618.75);// 2.089232047699551
const float burstPhase = M_PI / 4 * 3;//2.355 (M_PI/4 to 45stopni)

class SimplePALOutput
{
public:
	unsigned short longSync[lineSamples];
	unsigned short shortSync[lineSamples];
	unsigned short mixSync[lineSamples];
	unsigned short line[2][lineSamples];
	unsigned short blank[lineSamples];
	short SIN[imageSamples];
	short COS[imageSamples];
	short YLUT[16];
	short UVLUT[16];
	static const i2s_port_t I2S_PORT = (i2s_port_t)I2S_NUM_0;

	SimplePALOutput()
	{
		
		for (int i = 0; i < syncSamples; i++)                                   // Synchronizacja pozioma 4.8us
		{
			shortSync[i / 2] = syncLevel;										// SYNC na poczatku linii
			shortSync[(lineSamples / 2) + (i / 2)] = syncLevel;					// SYNC w polowie linii
			longSync[((lineSamples / 2) - syncSamples + i)] = blankLevel << 8;	// Pierwszy stan BLANK
			longSync[(lineSamples - syncSamples + i)] = blankLevel << 8;		// Drugi stan BLANK
			mixSync[((lineSamples / 2) - syncSamples + i)] = blankLevel << 8;	// Poczatej jak LONG
			//line[0][i] = syncLevel;											// I tak jest zero
			//line[1][i] = syncLevel;											// I tak jest zero
			//blank[i] = syncLevel;												// I tak jest zero
		}

		for (int i = 0; i < lineSamples - syncSamples; i++) // i < 790
		{
			shortSync[(i / 2) + (syncSamples / 2)] = blankLevel << 8;			// Pierwszy stan BLANK
			shortSync[((i/2) + (syncSamples / 2) + (lineSamples / 2))] = blankLevel << 8;	// Drugi stan BLANK
			mixSync[((i/2) + (syncSamples / 2) + (lineSamples / 2))] = blankLevel << 8; // Koniec jak SHORT
			line[0][(i + syncSamples)] = blankLevel << 8 ;
			line[1][(i + syncSamples)] = blankLevel << 8 ;
			blank[(i + syncSamples)] = blankLevel << 8;
		}

		for (int i = 0; i < burstSamples; i++)
		{
			int p = burstStart + i;
			unsigned short b0 = ((short)(blankLevel + sin(i * burstPerSample + burstPhase) * burstAmp)) << 8;
			unsigned short b1 = ((short)(blankLevel + sin(i * burstPerSample - burstPhase) * burstAmp)) << 8;
			line[0][p ^ 1] = b0;
			line[1][p ^ 1] = b1;
			//blank[p] = b1;
		}

		for (int i = 0; i < imageSamples; i++)
		{
			int p = frameStart + i; //170
			int c = p - burstStart; // 30
			SIN[i] = 0.436798 * sin(c * burstPerSample) * 256;
			COS[i] = 0.614777 * cos(c * burstPerSample) * 256;
		}

		for (int i = 0; i < 16; i++)
		{
			YLUT[i] = (blankLevel << 8) + i / 15. * 256 * maxLevel;
			UVLUT[i] = (i - 8) / 7. * maxLevel;
		}
	}

	void init()
	{
		i2s_config_t i2s_config =
		{
			.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
			.sample_rate = 1000000,  //not really used
			.bits_per_sample = (i2s_bits_per_sample_t)I2S_BITS_PER_SAMPLE_16BIT,
			.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
			.communication_format = I2S_COMM_FORMAT_STAND_I2S,
			.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
			.dma_buf_count = 4,
			.dma_buf_len = lineSamples,   //a buffer per line
			.use_apll = false,
			.tx_desc_auto_clear = false,
			.fixed_mclk = 0
		};
		i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);    //start i2s driver
		i2s_set_pin(I2S_PORT, NULL);                           //use internal DAC
		i2s_set_sample_rates(I2S_PORT, 1000000);               //dummy sample rate, since the function fails at high values
		//this is the hack that enables the highest sampling rate possible ~13MHz, have fun
		SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_A_V, 1, I2S_CLKM_DIV_A_S);
		SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_B_V, 1, I2S_CLKM_DIV_B_S);
		SET_PERI_REG_BITS(I2S_CLKM_CONF_REG(0), I2S_CLKM_DIV_NUM_V, 2, I2S_CLKM_DIV_NUM_S);
		SET_PERI_REG_BITS(I2S_SAMPLE_RATE_CONF_REG(0), I2S_TX_BCK_DIV_NUM_V, 2, I2S_TX_BCK_DIV_NUM_S);
		//untie DACs
		SET_PERI_REG_BITS(I2S_CONF_CHAN_REG(0), I2S_TX_CHAN_MOD_V, 3, I2S_TX_CHAN_MOD_S);
		SET_PERI_REG_BITS(I2S_FIFO_CONF_REG(0), I2S_TX_FIFO_MOD_V, 1, I2S_TX_FIFO_MOD_S);
	}

	void sendLine(unsigned short *l)
	{
		size_t i2s_bytes_write;
		i2s_write(I2S_PORT, (char *)l, lineSamples * sizeof(unsigned short), &i2s_bytes_write, portMAX_DELAY);
	}

	void sendFrame(char ***frame)
	{
		// Bez przeplotu
		for (int i = 0; i < 2; i++) sendLine(longSync);							// long sync

		sendLine(mixSync);														// long-short sync

		for (int i = 0; i < 2; i++)	sendLine(shortSync);						// short sync
		
		for (int i = 0; i < 35; i++) sendLine(blank);							// blank lines
		// <35 obraz przesuniety w gore ,35 - ok, 36 - brak synchro, 37 - negatyw kolorow

		for (int i = 0; i < 240; i += 2)
		{
			char *pixels0 = (*frame)[i];
			char *pixels1 = (*frame)[i + 1];
			int j = frameStart;
 
			for (int x = 0; x < imageSamples; x += 2)
			{
				unsigned short p0 = *(pixels0++);
				unsigned short p1 = *(pixels1++);
				short y0 = YLUT[p0 & 15];
				short y1 = YLUT[p1 & 15];
				unsigned char p04 = p0 >> 4;
				unsigned char p14 = p1 >> 4;
				short u0 = (SIN[x] * UVLUT[p04]);
				short u1 = (SIN[x + 1] * UVLUT[p04]);
				short v0 = (COS[x] * UVLUT[p14]);
				short v1 = (COS[x + 1] * UVLUT[p14]);
				//word order is swapped for I2S packing (j + 1) comes first then j
				line[0][j] = y0 + u1 + v1;
				line[1][j] = y1 + u1 - v1;
				line[0][j + 1] = y0 + u0 + v0;
				line[1][j + 1] = y1 + u0 - v0;
				j += 2;
			}

			sendLine(line[0]);
			sendLine(line[1]);
		}

		for (int i = 0; i < 25; i++) sendLine(blank);

		for (int i = 0; i < 3; i++)	sendLine(shortSync);
	}
};
