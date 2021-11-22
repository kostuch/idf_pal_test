#include "Graphics.h"
#include "TriangleTree.h"
#include <stdlib.h>

Graphics::Graphics(int w, int h, int initialTrinagleBufferSize)
	: xres(w),
	  yres(h)
{
	font = 0;
	cursorX = cursorY = cursorBaseX = 0;
	trinagleBufferSize = initialTrinagleBufferSize;
	triangleCount = 0;
	frontColor = 0xffff;
	backColor = 0;
}

void Graphics::setTextColor(int front, int back)
{
	//-1 = transparent back;
	frontColor = front;
	backColor = back;
}

void Graphics::init()
{
	frame = (char **)malloc(yres * sizeof(char *));
	backbuffer = (char **)malloc(yres * sizeof(char *));

	//not enough memory for z-buffer implementation
	//zbuffer = (char**)malloc(yres * sizeof(char*));
	for (int y = 0; y < yres; y++)
	{
		frame[y] = (char *)malloc(xres);
		backbuffer[y] = (char *)malloc(xres);
		//zbuffer[y] = (char*)malloc(xres);
	}

	triangleBuffer = (TriangleTree *)malloc(sizeof(TriangleTree) * trinagleBufferSize);
}

void Graphics::setFont(Font &font)
{
	this->font = &font;
}

void Graphics::setCursor(int x, int y)
{
	cursorX = cursorBaseX = x;
	cursorY = y;
}

void Graphics::out_txt(const char *str)
{
	if (!font) return;

	while (*str)
	{
		if (*str >= 32 && *str < 128)
			drawChar(cursorX, cursorY, *str, frontColor, backColor);

		cursorX += font->xres + 1;

		if (cursorX + font->xres > xres || *str == '\n')
		{
			cursorX = cursorBaseX;
			cursorY += font->yres;
		}

		str++;
	}
}

void Graphics::begin(int clear)
{
	if (clear > -1)
		for (int y = 0; y < yres; y++)
			for (int x = 0; x < xres; x++)
				dotFast(x, y, clear);

	triangleCount = 0;
	triangleRoot = 0;
}

void Graphics::flush()
{
	if (triangleRoot)
		triangleRoot->draw(*this);
}

void Graphics::end()
{
	char **b = backbuffer;
	backbuffer = frame;
	frame = b;
}

void Graphics::enqueueTriangle(short *v0, short *v1, short *v2, unsigned int color)
{
	if (triangleCount >= trinagleBufferSize) return;

	TriangleTree &t = triangleBuffer[triangleCount++];
	t.set(v0, v1, v2, color);

	if (triangleRoot)
		triangleRoot->add(&triangleRoot, t);
	else
		triangleRoot = &t;
}

void Graphics::triangle(short *v0, short *v1, short *v2, unsigned int color)
{
	short *v[3] = {v0, v1, v2};

	if (v[1][1] < v[0][1])
	{
		short *vb = v[0]; v[0] = v[1]; v[1] = vb;
	}

	if (v[2][1] < v[1][1])
	{
		short *vb = v[1]; v[1] = v[2]; v[2] = vb;
	}

	if (v[1][1] < v[0][1])
	{
		short *vb = v[0]; v[0] = v[1]; v[1] = vb;
	}

	int y = v[0][1];
	int xac = v[0][0] << 16;
	int xab = v[0][0] << 16;
	int xbc = v[1][0] << 16;
	int xaci = 0;
	int xabi = 0;
	int xbci = 0;

	if (v[1][1] != v[0][1])
		xabi = ((v[1][0] - v[0][0]) << 16) / (v[1][1] - v[0][1]);

	if (v[2][1] != v[0][1])
		xaci = ((v[2][0] - v[0][0]) << 16) / (v[2][1] - v[0][1]);

	if (v[2][1] != v[1][1])
		xbci = ((v[2][0] - v[1][0]) << 16) / (v[2][1] - v[1][1]);

	for (; y < v[1][1] && y < yres; y++)
	{
		if (y >= 0)
			xLine(xab >> 16, xac >> 16, y, color);

		xab += xabi;
		xac += xaci;
	}

	for (; y < v[2][1] && y < yres; y++)
	{
		if (y >= 0)
			xLine(xbc >> 16, xac >> 16, y, color);

		xbc += xbci;
		xac += xaci;
	}
}

void Graphics::line(int x1, int y1, int x2, int y2, unsigned int color)
{
	int x, y, xe, ye;
	int dx = x2 - x1;
	int dy = y2 - y1;
	int dx1 = labs(dx);
	int dy1 = labs(dy);
	int px = 2 * dy1 - dx1;
	int py = 2 * dx1 - dy1;

	if (dy1 <= dx1)
	{
		if (dx >= 0)
		{
			x = x1;
			y = y1;
			xe = x2;
		}
		else
		{
			x = x2;
			y = y2;
			xe = x1;
		}

		dot(x, y, color);

		for (int i = 0; x < xe; i++)
		{
			x = x + 1;

			if (px < 0)
				px = px + 2 * dy1;
			else
			{
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
					y = y + 1;
				else
					y = y - 1;

				px = px + 2 * (dy1 - dx1);
			}

			dot(x, y, color);
		}
	}
	else
	{
		if (dy >= 0)
		{
			x = x1;
			y = y1;
			ye = y2;
		}
		else
		{
			x = x2;
			y = y2;
			ye = y1;
		}

		dot(x, y, color);

		for (int i = 0; y < ye; i++)
		{
			y = y + 1;

			if (py <= 0)
				py = py + 2 * dx1;
			else
			{
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
					x = x + 1;
				else
					x = x - 1;

				py = py + 2 * (dx1 - dy1);
			}

			dot(x, y, color);
		}
	}
}

void Graphics::fillRect(int x, int y, int w, int h, unsigned int color)
{
	if (x < 0)
	{
		w += x;
		x = 0;
	}

	if (y < 0)
	{
		h += y;
		y = 0;
	}

	if (x + w > xres)
		w = xres - x;

	if (y + h > yres)
		h = yres - y;

	for (int j = y; j < y + h; j++)
		for (int i = x; i < x + w; i++)
			dotFast(i, j, color);
}

void Graphics::rect(int x, int y, int w, int h, unsigned int color)
{
	fillRect(x, y, w, 1, color);
	fillRect(x, y, 1, h, color);
	fillRect(x, y + h - 1, w, 1, color);
	fillRect(x + w - 1, y, 1, h, color);
}

/* Draw 1-bit bitmap with given foreground and background color */
void Graphics::drawMonoBMP(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *bitmap, uint16_t fg, uint16_t bg)
{
	uint16_t byte_width = (width + 7) / 8;
	uint8_t pattern = *bitmap;

	for (uint16_t rows = 0; rows < height; rows++)							// For every row in bitmap pattern
	{
		for (uint16_t cols = 0; cols < width + 1; cols++)					// For every column in bitmap pattern
		{
			if (pattern & 0x80) dot(cols + x, rows + y, fg);				// If bit in pattern is set, pixel in foreground color
			else if (bg != TRANSPARENT) dot(cols + x, rows + y, bg);

			if (cols & 7) pattern <<= 1;									// Eight bits to draw
			else pattern = *(bitmap + rows * byte_width + cols / 8); 		// Next byte of pattern
		}
	}
}

void Graphics::drawChar(int x, int y, char ch, int frontColor, int backColor)
{
	if (font->xres == 5)
	{
		for (uint8_t cols = 0; cols < 5; cols++ )							// 5 columns
		{
			uint8_t line = *(font->pixels + ( (ch - ' ') * 5) + cols);		// Characters start with code 32 (space)

			for (uint8_t rows = 0; rows < 8; rows++)						// 8 rows
			{
				if (line & 0x1) dotFast(x + cols, y + rows, frontColor);		// If bit in pattern set
				else if (backColor != TRANSPARENT) dotFast(x + cols, y + rows, backColor);

				line >>= 1;													// Next bit in pattern
			}
		}
	}
	else
	{
		const uint8_t *adr = font->pixels + ((ch - ' ') * ((font->xres + 7) / 8) * font->yres);
		drawMonoBMP(x, y, font->xres, font->yres, adr, frontColor, backColor);
		//if (backColor != TRANSPARENT) draw_mono_bmp(x, y, font->xres, font->yres, adr, frontColor, backColor);
		//else draw_mono_trans_bmp(x, y, font->xres, font->yres, adr, frontColor);
	}
}

size_t Graphics::write(uint8_t data)
{
	drawChar(cursorX, cursorY, data, frontColor, backColor);
	cursorX += font->xres;

	if (cursorX + font->xres > xres || data == '\n')
	{
		cursorX = cursorBaseX;
		cursorY += font->yres;
	}

	return cursorX;
}