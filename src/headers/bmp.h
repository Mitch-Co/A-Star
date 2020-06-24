#ifndef BMP_H
#define BMP_H

#define longestFileName 100
#include <stdint.h>

typedef uint8_t BYTE;

typedef struct BMPHEADER {

} BMP_HEAD;

// 
typedef struct BMPDIBHEADER {
    
} BMP_DIB;

typedef struct BMPDATA {

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

void errMsg(char func[],char err[]);

void freeBMP();

BMP* newBMP();

BMP* readBMP(char fileName[]);

#endif