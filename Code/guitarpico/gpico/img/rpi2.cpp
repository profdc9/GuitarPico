#include "main.h"

// format: 2-bit pixel graphics
// image width: 32 pixels
// image height: 40 lines
// image pitch: 8 bytes
const u8 RPi2Img[320] __attribute__ ((aligned(4))) = {
	0xFF, 0xF0, 0x03, 0xFF, 0xFF, 0xC0, 0x0F, 0xFF, 0xF0, 0x01, 0x40, 0x0F, 0xF0, 0x01, 0x40, 0x0F, 
	0xF1, 0x55, 0x55, 0x43, 0xC1, 0x55, 0x55, 0x4F, 0xC1, 0x55, 0x55, 0x53, 0xC5, 0x55, 0x55, 0x43, 
	0xF1, 0x55, 0x55, 0x54, 0x15, 0x55, 0x55, 0x4F, 0xF1, 0x55, 0x41, 0x54, 0x15, 0x51, 0x55, 0x4F, 
	0xF0, 0x55, 0x50, 0x54, 0x15, 0x05, 0x55, 0x0F, 0xFC, 0x55, 0x55, 0x10, 0x04, 0x55, 0x55, 0x3F, 
	0xFC, 0x15, 0x55, 0x40, 0x01, 0x55, 0x54, 0x3F, 0xFF, 0x05, 0x55, 0x40, 0x01, 0x55, 0x50, 0xFF, 
	0xFF, 0xC0, 0x54, 0x00, 0x00, 0x15, 0x43, 0xFF, 0xFF, 0xF0, 0x00, 0x02, 0x80, 0x00, 0x0F, 0xFF, 
	0xFF, 0xC0, 0xA8, 0x2A, 0xA8, 0x28, 0x03, 0xFF, 0xFF, 0x0A, 0xA0, 0xAA, 0xAA, 0x0A, 0xA0, 0xFF, 
	0xFF, 0x0A, 0x80, 0xAA, 0xAA, 0x02, 0xA8, 0xFF, 0xFC, 0x2A, 0x00, 0x0A, 0xA0, 0x00, 0xA8, 0x3F, 
	0xFC, 0x28, 0x00, 0x00, 0x00, 0xA0, 0x28, 0x3F, 0xFC, 0x00, 0x2A, 0xA0, 0x0A, 0xAA, 0x00, 0x3F, 
	0xF0, 0x00, 0xAA, 0xA8, 0x2A, 0xAA, 0x80, 0x0F, 0xC2, 0x82, 0xAA, 0xA8, 0x2A, 0xAA, 0x82, 0x83, 
	0xC2, 0x82, 0xAA, 0xA8, 0x2A, 0xAA, 0x82, 0xA3, 0x0A, 0x82, 0xAA, 0xA8, 0x2A, 0xAA, 0xA2, 0xA0, 
	0x0A, 0x82, 0xAA, 0xA8, 0x0A, 0xAA, 0x82, 0xA0, 0x0A, 0x82, 0xAA, 0xA0, 0x02, 0xAA, 0x82, 0xA0, 
	0x0A, 0x00, 0xAA, 0x80, 0x00, 0xAA, 0x00, 0xA0, 0x0A, 0x00, 0x00, 0x0A, 0xA0, 0x00, 0x00, 0xA0, 
	0xC0, 0x00, 0x00, 0x2A, 0xAA, 0x00, 0x00, 0x03, 0xF0, 0x28, 0x00, 0xAA, 0xAA, 0x00, 0xAA, 0x0F, 
	0xF0, 0xAA, 0x00, 0xAA, 0xAA, 0x82, 0xAA, 0x0F, 0xF0, 0xAA, 0x80, 0xAA, 0xAA, 0x8A, 0xAA, 0x0F, 
	0xFC, 0xAA, 0xA0, 0xAA, 0xAA, 0x0A, 0xAA, 0x3F, 0xFC, 0x2A, 0xA0, 0x2A, 0xAA, 0x0A, 0xAA, 0x3F, 
	0xFC, 0x2A, 0xA8, 0x0A, 0xA0, 0x2A, 0xA8, 0x3F, 0xFF, 0x0A, 0xA8, 0x00, 0x00, 0x2A, 0xA0, 0xFF, 
	0xFF, 0xC0, 0x00, 0x00, 0x00, 0x0A, 0x03, 0xFF, 0xFF, 0xFC, 0x00, 0xAA, 0xAA, 0x00, 0x3F, 0xFF, 
	0xFF, 0xFF, 0x00, 0xAA, 0xAA, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0x2A, 0xA8, 0x0F, 0xFF, 0xFF, 
	0xFF, 0xFF, 0xFF, 0x02, 0x80, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC0, 0x03, 0xFF, 0xFF, 0xFF, 
};