#include <avr/pgmspace.h>
#include <stdlib.h>

#include "Adafruit_GFX.h"
#include "ST7920_GFX_Library.h"

uint8_t buff[2048] __attribute__ ((aligned (4)));		//This array serves as primitive "Video RAM" buffer.  Ensure it's 32-bit aligned.  Enough for 2 x ST7920s.

//This display is split into two halfs. Pages are 16bit long and pages are arranged in that way that are lied horizontaly instead of verticaly, unlike SSD1306 OLED, Nokia 5110 LCD, etc.
//After 8 horizonral page is written, it jumps to half of the screen (Y = 32) and continues until 16 lines of page have been written. After that, we have set cursor in new line.
void ST7920::drawPixel(int16_t x, int16_t y, uint16_t color) {
	if(x<0 || x>=ST7920_WIDTH || y<0 || y>=ST7920_HEIGHT) return;
  	uint8_t y0 = 0, x0 = 0;								//Define and initilize varilables for skiping rows
  	uint16_t data, n;										//Define variable for sending data itno buffer (basicly, that is one line of page)
  	if (y > 31) {											//If Y coordinate is bigger than 31, that means we have to skip into that row, but we have to do that by adding 
    	y -= 32;
    	y0 = 16;
  	}
  	x0 = x % 16;
  	x /= 16;
  	data = 0x8000 >> x0;
  	n = (x * 2) + (y0) + (32 * y);
  	if (!color) {
    	buff[n] &= (~data >> 8);
    	buff[n + 1] &= (~data & 0xFF);
  	}else{
    	buff[n] |= (data >> 8);
    	buff[n + 1] |= (data & 0xFF);
  	}
}

ST7920::ST7920(uint8_t CS, uint8_t width /* =ST7920_WIDTH */) : Adafruit_GFX(ST7920_HEIGHT, width) {
  	cs = CS;
    buf_len = 1024;
}

void ST7920::begin(void) {
	SPI.begin();
	pinMode(cs, OUTPUT);
  	digitalWrite(cs, HIGH);
	ST7920Command(0b00001100);
  	digitalWrite(cs, LOW);
}

void ST7920::clearDisplay() {
  	uint32_t* p = (uint32_t*)&buff;
  	for (uint16_t i = 0; i < (buf_len >> 2); i++) {
    	p[i] = 0;
  	}
}

void ST7920::display() {
  	uint8_t x = 0, y = 0;
    uint16_t n = 0;
  	digitalWrite(cs, HIGH);
  	ST7920Command(0b00100100); //EXTENDED INSTRUCTION SET
  	ST7920Command(0b00100110); //EXTENDED INSTRUCTION SET
  	for (y = 0; y < 32; y++) {
    	ST7920Command(0x80 | y);
    	ST7920Command(0x80);  // x always starts at 0.
    	for (x = 0; x < 16; x++) {
      		ST7920Data(buff[n]);
      		ST7920Data(buff[n + 1]);
      		n += 2;
    	}
  	}
  	digitalWrite(cs, LOW);
}

void ST7920::invertDisplay() {
  	uint32_t* p = (uint32_t*)&buff;
  	for(uint16_t i = 0; i < (buf_len >> 2); i++) {
    	p[i] = ~p[i];
  	}
}

void ST7920::ST7920Data(uint8_t data) { //RS = 1 RW = 0
  	SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE3));
  	SPI.transfer(0b11111010);
  	SPI.transfer((data & 0b11110000));
 	SPI.transfer((data & 0b00001111) << 4);
  	SPI.endTransaction();
  	delayMicroseconds(38);
}

void ST7920::ST7920Command(uint8_t data) { //RS = 0 RW = 0
  	SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE3));
  	SPI.transfer(0b11111000);
  	SPI.transfer((data & 0b11110000));
  	SPI.transfer((data & 0b00001111) << 4);
  	SPI.endTransaction();
  	delayMicroseconds(38);
}

// --------------- 192 x 64

//This display is split into two halfs. Pages are 16bit long and pages are arranged in that way that are lied horizontaly instead of verticaly, unlike SSD1306 OLED, Nokia 5110 LCD, etc.
//After 8 horizonral page is written, it jumps to half of the screen (Y = 32) and continues until 16 lines of page have been written. After that, we have set cursor in new line.

// Layout of buf[]: 
//  - each block of 16 bytes is one scan line of 128 pixels for a particular y value
//  - y rows are interleaved between top and bottom halves, so y=0, y=32, y=1, y=33 ...

void ST7920_192::drawPixel(int16_t x, int16_t y, uint16_t color) {
	if(x<0 || x>=ST7920_192_WIDTH || y<0 || y>=ST7920_HEIGHT) return;
  	uint8_t x0 = 0;
  	uint16_t data, n, offset = 0;							//Define variable for sending data itno buffer (basicly, that is one line of page)
  	if (y > 31) {											//If Y coordinate is bigger than 31, that means we have to skip into that row, but we have to do that by adding 
        // Second half needs to hit other controller.
    	y -= 32;
        offset = 1024;
  	}
  	x0 = x % 16;
  	x /= 16;
  	data = 0x8000 >> x0;
  	n = offset + (x * 2) + (32 * y);
  	if (!color) {
    	buff[n] &= (~data >> 8);
    	buff[n + 1] &= (~data & 0xFF);
  	}else{
    	buff[n] |= (data >> 8);
    	buff[n + 1] |= (data & 0xFF);
  	}
}

ST7920_192::ST7920_192(uint8_t CS, uint8_t CK1, uint8_t CK2) : ST7920(CS, ST7920_192_WIDTH) {
    ck1 = CK1;
    ck2 = CK2;
    buf_len = 2048;
}

void ST7920_192::set_chip(uint8_t chip) {
    uint8_t ck, not_ck;
    if (chip == 1) {
        ck = ck1;
        not_ck = ck2;
    } else {
        ck = ck2;
        not_ck = ck1;
    }
    gpio_set_function(not_ck, GPIO_FUNC_SIO);
    gpio_set_function(ck, GPIO_FUNC_SPI);
    SPI.setSCK(ck);
}

void ST7920_192::begin(void) {
    set_chip(1);
	SPI.begin();
	pinMode(cs, OUTPUT);
  	digitalWrite(cs, HIGH);
	ST7920Command(0b00001100);
  	digitalWrite(cs, LOW);
    // Initialize bottom-half chip.
    set_chip(2);
  	digitalWrite(cs, HIGH);
	ST7920Command(0b00001100);
  	digitalWrite(cs, LOW);
}

void ST7920_192::display() {
  	uint8_t c = 0, x = 0, y = 0;
    uint16_t n = 0;
    set_chip(1);  // Top rows.
    for (c = 0; c < 2; c++) {
      	digitalWrite(cs, HIGH);
        ST7920Command(0b00100100); //EXTENDED INSTRUCTION SET
  	    ST7920Command(0b00100110); //EXTENDED INSTRUCTION SET
  	    for (y = 0; y < 32; y++) {
    	    ST7920Command(0x80 | y);
    	    ST7920Command(0x80);  //  | x);  // Always start at x=0.
    	    for (x = 0; x < 12; x++) {  // 12 tiles of 16 pixels = 192 pixels.
      		    ST7920Data(buff[n]);
      		    ST7920Data(buff[n + 1]);
      		    n += 2;
    	    }
            // Skip over the last 64 pixels in this row for 192 col display.
            n += 8;
  	    }
      	digitalWrite(cs, LOW);
        set_chip(2);  // Bottom rows.
    }
}
