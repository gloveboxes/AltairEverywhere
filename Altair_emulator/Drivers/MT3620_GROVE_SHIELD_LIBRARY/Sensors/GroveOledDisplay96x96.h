//GROVE_NAME        "Grove - OLED Display 1.12'' V2"
//SKU               101020132
//WIKI_URL          http://wiki.seeedstudio.com/Grove-OLED_Display_1.12inch/
#pragma once

#include "../applibs_versions.h"
#include <applibs/gpio.h>

#define SH1107G  1
#define SSD1327  2

void GroveOledDisplay_Init(int i2cFd, uint8_t IC);

void setNormalDisplay(void);
void setInverseDisplay(void);

void setGrayLevel(unsigned char grayLevel);

void setVerticalMode(void);
void setHorizontalMode(void);

void setTextXY(unsigned char Row, unsigned char Column);
void clearDisplay(void);
void setContrastLevel(unsigned char ContrastLevel);
void putChar(unsigned char c);
void putString(const char *String);
unsigned char putNumber(long n);

void drawBitmap(const unsigned char *bitmaparray, int bytes);

void setHorizontalScrollProperties(bool direction, unsigned char startRow, unsigned char endRow, unsigned char startColumn, unsigned char endColumn, unsigned char scrollSpeed);
void activateScroll(void);
void deactivateScroll(void);