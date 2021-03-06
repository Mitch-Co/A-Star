#ifndef BMP_H
#define BMP_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define longestFileName 100
#define bmpSignature 0x4d42

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
// ALL DATA STORED IN BIG ENDIAN
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

    // Number of colors in color table
    uint32_t colorPalette;
    
    // Number of important colors
    // Set to zero when every color is important
    // "Generally ignored" - Wikipedia
    uint32_t importantColors;

} BMP_DIB;

typedef struct BMPCOLORTABLE {
    uint32_t* entries;
    uint32_t length;
} COLOR_TABLE;

typedef struct BMPPIXEL {
    uint32_t value;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;

} PIXEL;

typedef struct BMPDATA {
    int width;
    int height;
    int area;
    int bitDepth;

    bool hasAlpha;
    int bitsForAlpha;

    bool HasCTable;
    COLOR_TABLE cTable;

    PIXEL* colorData;

} BMP_DATA;

typedef struct BMPFILE {

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

// Reads in a BMP file and returns a BMP struct as a pointer
BMP* readBMP(char* fileName);

// Verifies and reads the file header
bool readHeader(BMP* toReturn, FILE* fp);

// Reads the DIB Header
bool readDIB(BMP* toReturn, FILE* fp);

// Reads the bitmap color data
bool readData(BMP* toReturn, FILE* fp);

bool readDataBits(BMP* toReturn, FILE* fp);

bool readDataBytes(BMP* toReturn, FILE* fp);

bool readColorTable(BMP* toReturn, FILE* fp);

// Checks if a string ends with a substring
bool endsWith(char* toCheck, char* ending);

// Writes BMP to file
bool writeBMP(BMP* toWrite, char* fileName);

// Writes BMP image data
bool writeData(BMP* toWrite, FILE* fp, uint32_t* fileSize);

bool writeDataBits(BMP* toWrite, FILE* fp, uint32_t* fileSize);

bool writeDataBytes(BMP* toWrite, FILE* fp, uint32_t* fileSize);

bool writeColorTable(BMP* toWrite, FILE* fp, uint32_t* fileSize);

int power(int base, int exp);

#endif