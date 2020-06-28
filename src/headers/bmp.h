#ifndef BMP_H
#define BMP_H

#include <stdint.h>
#include <stdbool.h>

#define longestFileName 100
#define bmpSignature 0x4d42


typedef uint8_t BYTE;

typedef struct BMPHEADER {
    //Should be 0x4d42 to identify bitmap file
    uint16_t signiture;

    // File size in bytes
    uint32_t fileSize;
    
    // Not to be touched
    uint16_t reserved1;
    uint16_t reserved2;

    // The byte where the image data begins
    uint32_t offset; 
} BMP_HEAD;

// MODELED OFF OF BITMAPINFOHEADER DIB
typedef struct BMPDIBHEADER {
    
    // Should be 40 for BITMAPINFOHEADER
    uint32_t headerSize;

    /*
        For some reason the specifications for DIB headers have signed values for width and height?
        Apperently the world is not ready for negative bitmap area.
        Luckily the specification is ready for when we have advanced
        society to the point of needing negative image dimentions.
    */
    int32_t bmpWidth;
    int32_t bmpHeight;

    // Must be 1 according to spec
    uint16_t colorPlanes;

    // Can be either 1, 4, 8, 16, 24, or 32
    // This program only aims to be compatiable with 24 or 32 bpp bitmap
    // 32-bit color is 24 bit color with an extra 8 bit alpha channel
    uint16_t bitsPerPixel;

    // MUST be 0, or image is compressed and regular operation will likely fail
    uint32_t compression;

    // Size of bitmap image data 
    // (DO NOT TRUST, can randomly be zero for uncompressed bitmaps)
    uint32_t imageSize;

    /*
        These next two elements are the width and height of the image
        in pixels per meter. 
        Not only is this the single worst way to measure an image,
        it changes with the monitor resolution and size.
        Good thing it can also be negative, just to be compatible with
        the negative bmpWidth and bmpHeight.
    */
    int32_t resWidthPPM;
    int32_t resHeightPPM;

    // Number of colors in color palette
    uint32_t colorPalette;
    
    // Number of important colors
    // Set to zero when every color is important
    // "Generally ignored" - Wikipedia
    uint32_t importantColors;

} BMP_DIB;

typedef struct BMPPIXEL {
    int red;
    int blue;
    int green;
    int alpha;

} PIXEL;

typedef struct BMPDATA {
    int width;
    int height;
    int area;
    int bitDepth;
    int bitsPerChannel;
    bool hasAlpha;
    int bitsForAlpha;

    PIXEL* data;

} BMP_DATA;

typedef struct RAWBMP {
    long long size;
    BYTE* byteArray;
} BMP_RAW;

typedef struct BMPFILE {
    BMP_RAW rawBMP;

    BMP_HEAD head;
    BMP_DIB dib;
    BMP_DATA data;

} BMP;

// Displays error message for a function
void errMsg(char func[],char err[]);

// Frees a BMP struct and all subelements
void freeBMP();

// Creates a BMP struct
BMP* newBMP();

// Takes an array of bytes in little endian and return a uint_32t of those bytes
uint32_t bytesToUINT32(BYTE* byteArray, int numBytes);

// Takes an array of bytes in little endian and return a uint_32t of those bytes
uint16_t bytesToUINT16(BYTE* byteArray, int numBytes);
// Reads in a BMP file and returns a BMP struct as a pointer
BMP* readBMP(char fileName[]);

#endif