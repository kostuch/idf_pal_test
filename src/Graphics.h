#pragma once
#include "stdint.h"
#include "stddef.h"
#include "RGB2YUV.h"
#include "colors.h"

class TriangleTree;
class Font
{
public:
	int xres;
	int yres;
	const unsigned char *pixels;

	Font(int charWidth, int charHeight, const unsigned char *pixels_)
		: xres(charWidth),
		  yres(charHeight),
		  pixels(pixels_)
	{
	}
};

class Graphics
{
public:
	int xres;
	int yres;
	char **frame;
	char **backbuffer;
	char **zbuffer;
	int cursorX, cursorY, cursorBaseX;
	unsigned int frontColor, backColor;
	Font *font;

	TriangleTree *triangleBuffer;
	TriangleTree *triangleRoot;
	int trinagleBufferSize;
	int triangleCount;

	Graphics(int w, int h, int initialTrinagleBufferSize = 0);
	void setTextColor(int front, int back = 0);
	void init();

	size_t write(uint8_t data);
	void setFont(Font &font);
	void setCursor(int x, int y);
	void out_txt(const char *str);
	void begin(int clear = -1);
	void flush();
	void end();
	void drawMonoBMP(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *bitmap, uint16_t fg, uint16_t bg);
	void drawChar(int x, int y, char ch, int frontColor, int backColor);
	void enqueueTriangle(short *v0, short *v1, short *v2, unsigned int color);
	void triangle(short *v0, short *v1, short *v2, unsigned int color);
	void line(int x1, int y1, int x2, int y2, unsigned int color);
	void fillRect(int x, int y, int w, int h, unsigned int color);
	void rect(int x, int y, int w, int h, unsigned int color);

	inline void dotFast(int x, int y, unsigned int color)
	{
		int c = RGB2YUV[color & 0xfff];
		// To ponizej robi inwersje kolorow do "normalnosci"
		//int c = RGB2YUV[0xfff - (color & 0xfff)];

		if (y & 1)
		{
			backbuffer[y & ~1][x] = (backbuffer[y & ~1][x] & 0xf) | (c & 0xf0);
			backbuffer[y][x] = c >> 8;
		}
		else
		{
			backbuffer[y][x] = c;
			backbuffer[y | 1][x] = (backbuffer[y | 1][x] & 0xf) | ((c >> 8) & 0xf0);
		}
	}

	inline void dot(int x, int y, unsigned int color)
	{
		if ((unsigned int)x < xres && (unsigned int)y < yres)
		{
			int c = RGB2YUV[color & 0xfff];
			// To ponizej robi inwersje kolorow do "normalnosci"
			//int c = RGB2YUV[0xfff - (color & 0xfff)];

			if (y & 1)
			{
				backbuffer[y & ~1][x] = (backbuffer[y & ~1][x] & 0xf) | (c & 0xf0);
				backbuffer[y][x] = c >> 8;
			}
			else
			{
				backbuffer[y][x] = c;
				backbuffer[y | 1][x] = (backbuffer[y | 1][x] & 0xf) | ((c >> 8) & 0xf0);
			}
		}
	}

	inline void dotAdd(int x, int y, unsigned int color)
	{
		if ((unsigned int)x < xres && (unsigned int)y < yres)
			backbuffer[y][x] = (color + backbuffer[y][x]) > 54 ? 54 : color + backbuffer[y][x];
	}

	inline int get(int x, int y)
	{
		if ((unsigned int)x < xres && (unsigned int)y < yres)
			return backbuffer[y][x];

		return 0;
	}

	inline void xLine(int x0, int x1, int y, unsigned int color)
	{
		if (x0 > x1)
		{
			int xb = x0;
			x0 = x1;
			x1 = xb;
		}

		if (x0 < 0) x0 = 0;

		if (x1 > xres) x1 = xres;

		for (int x = x0; x < x1; x++)
			dotFast(x, y, color);
	}

	///converting from r8g8b8 to r4g4b4a4
	inline unsigned int rgb(int r, int g, int b)
	{
		return (r >> 4) + (g & 0xf0) + ((b & 0xf0) << 4) + 0xf000;
	}

	///converting from r8g8b8a8 to r4g4b4a4
	inline unsigned int rgba(int r, int g, int b, int a)
	{
		return (r >> 4) + (g & 0xf0) + ((b & 0xf0) << 4) + ((b & 0xf0) << 12);
	}
};

