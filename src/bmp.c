#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "bmp.h"

void errMsg(char func[],char err[])
{
    printf("Error in function %s - %s\n", func, err);
    return;
}

void freeBMP(BMP** toFree)
{

    free(*toFree);
}

BMP* newBMP()
{
    BMP* toReturn = malloc(sizeof(BMP));
    
    return toReturn;
}

// NOTE: BYTEARRAY IN LITTLE ENDIAN
uint32_t bytesToUINT32(BYTE* byteArray, int numBytes)
{
    uint32_t toReturn = 0x0;
    
    // Input Checking
    if(byteArray == NULL)
    {
        return toReturn;
    }
    if (numBytes <= 0)
    {
        return toReturn;
    }
    if (numBytes > 4)
    {
        numBytes = 4;
    }

    // Conversion starts at numBytes because byteArray is in little endian
    for(int i = numBytes - 1; i >= 0; i--)
    {
        // Bitshift by 8 to bump the previous byte added up one byte
        // EXAMPLE: 0000000011111111 --> 1111111100000000
        toReturn <<= 8;
        toReturn += byteArray[i];
    }

    return toReturn;
}

// NOTE: BYTEARRAY IN LITTLE ENDIAN
uint16_t bytesToUINT16(BYTE* byteArray, int numBytes)
{
    uint16_t toReturn = 0x0;
    
    // Input Checking
    if(byteArray == NULL)
    {
        return toReturn;
    }
    if (numBytes <= 0)
    {
        return toReturn;
    }
    if (numBytes > 2)
    {
        numBytes = 2;
    }

    // Conversion starts at numBytes because byteArray is in little endian
    for(int i = numBytes - 1; i >= 0; i--)
    {
        // Bitshift by 8 to bump the previous byte added up one byte
        // EXAMPLE: 0000000011111111 --> 1111111100000000
        toReturn <<= 8;
        toReturn += byteArray[i];
    }

    return toReturn;
}


BMP* readBMP(char fileName[]) 
{
    /* INITIALIZATION AND ERROR CHECKING */

    if(fileName == NULL)
    {
        errMsg("readBMP", "input filename is NULL!");
        return NULL;
    }

    FILE* fp = NULL;
    int fileNameSize = strlen(fileName);

    // Allocate memory for bitmap struct
    BMP* toReturn = newBMP();
    if(toReturn == NULL)
    {
        errMsg("readBMP", "Failed to allocate memory for bitmap struct!");
        return NULL;
    }

    // File name must be within acceptable bounds
    // File name must be at least long enough to hold ".bmp" and have at least a single character name (5 char)
    if(fileNameSize > longestFileName || fileNameSize < 5)
    {
       errMsg("readBMP", "File name outside of allowable size bounds");
       freeBMP(&toReturn);
       return NULL;
    }

    // Error triggers if filename does not end in ".bmp"
    if(fileName[fileNameSize - 4] != '.' || fileName[fileNameSize - 3] != 'b' || fileName[fileNameSize - 2] != 'm' || fileName[fileNameSize - 1] != 'p')
    {
       errMsg("readBMP", "File name not valid \".bmp\" file!");
       freeBMP(&toReturn);
       return NULL;
    }

    fp = fopen(fileName, "rb");

    if(fp == NULL)
    {
        errMsg("readBMP", "Failed to open file");
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }

    /* START PASS 1 */

    /*
        Pass 1 is meant to verify that everything is in order
        before reading is attempted.
        Verify that file:
            - Is a bmp file
            - Has correct DIB header
            - Filesize is correctly listed
            - Bit depth is above 16bit (optional)
            - Has no compression
    */

    // An input buffer for the binary file
    BYTE* byteBuffer = malloc(sizeof(BYTE) * 8);

    // Used to check the return value of various file functions
    int returnChk = 0;

    // Actual fileSize stored here - to compare against listedSize for verification of size
    long fileSize = 0;

    // Temp variables to store read in data for pass 2
    uint16_t signiture = 0;
    uint32_t listedSize = 0;
    uint32_t headerSize = 0;
    uint32_t compression = 0;
    uint32_t bitDepth = 0;

    // Check for bitmap header signature at beginning of file
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 2, fp);
    if (returnChk != sizeof(BYTE) * 2)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap signature)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
    signiture = bytesToUINT16(byteBuffer, 2);
    if(signiture != bmpSignature)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap signature)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }

    // Seek to end of file to find length
    fseek(fp, 0L, SEEK_END);
    fileSize = ftell(fp);

    // Seek to 0x2 (long) (position of fileSize)
    fseek(fp, 0x2L, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if(returnChk != sizeof(BYTE) * 4)
    {
        errMsg("readBMP", "");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
    listedSize = bytesToUINT32(byteBuffer, 4);
    if(listedSize != fileSize)
    {
        errMsg("readBMP", "File header contains invalid file size!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }

    //Check if header is BITMAPINFOHEADER or newer
    fseek(fp, 0x0EL, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if (returnChk != sizeof(BYTE) * 4)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap header)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
    headerSize = bytesToUINT32(byteBuffer, 4);
    if(headerSize < 40)
    {
        errMsg("readBMP", "Bitmap file has unknown header DIB!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
    
    // Check file compression
    fseek(fp, 0x1EL, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if (returnChk != sizeof(BYTE) * 4)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap header)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
    compression = bytesToUINT32(byteBuffer, 4);
    if(compression != 0x0)
    {
        errMsg("readBMP", "Bitmap file has compression enabled!\n readBMP only accepts non compressed bitmaps!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }

    //Check bit depth
    fseek(fp, 0x1CL, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if (returnChk != sizeof(BYTE) * 4)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap header)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
    bitDepth = bytesToUINT32(byteBuffer, 4);
    if(bitDepth != 32 && bitDepth != 24)
    {
        errMsg("readBMP", "Bitmap file has unknown depth (only bit depths of 24 or 32 bits are able to be read)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }

    
    /* START PASS 2 */

    /*
        Pass 2 is meant to fill the BMP struct with all data from the bitmap file.
        Some data stored redundantly; the total memory size of all structs 
        should be around 1.5 - 2.2x the size of the file.
    */

    // We already have valid data from pass 1
    // Let's use it to fill in some BMP data

    toReturn->head.signiture = signiture;
    toReturn->head.fileSize = listedSize;

    toReturn->dib.headerSize = headerSize;
    toReturn->dib.compression = compression;
    toReturn->dib.bitsPerPixel = bitDepth;

    // Grab the rest of the header/DIB
    // General format as follows:
    // Seek to data location -> Grab data -> Check if data grabbed -> Put in struct

    // offset
    fseek(fp, 0xAL, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if(returnChk != sizeof(BYTE) * 4) {goto pass2Error;}
    toReturn->head.offset = bytesToUINT32(byteBuffer, 4);

    // bmpWidth
    fseek(fp, 0x12L, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if(returnChk != sizeof(BYTE) * 4) {goto pass2Error;}
    toReturn->dib.bmpWidth = bytesToUINT32(byteBuffer, 4);;

    // bmpHeight
    fseek(fp, 0x16L, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if(returnChk != sizeof(BYTE) * 4) {goto pass2Error;}
    toReturn->dib.bmpHeight = bytesToUINT32(byteBuffer, 4);

    // colorPlanes
    fseek(fp, 0x1AL, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 2, fp);
    if(returnChk != sizeof(BYTE) * 2) {goto pass2Error;}
    toReturn->dib.colorPlanes = bytesToUINT16(byteBuffer, 2);

    // imageSize
    fseek(fp, 0x22L, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if(returnChk != sizeof(BYTE) * 4) {goto pass2Error;}
    toReturn->dib.imageSize = bytesToUINT32(byteBuffer, 4);

    // resWidthPPM
    fseek(fp, 0x26L, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if(returnChk != sizeof(BYTE) * 4) {goto pass2Error;}
    toReturn->dib.resWidthPPM = bytesToUINT32(byteBuffer, 4);

    // resHeightPPM
    fseek(fp, 0x2AL, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if(returnChk != sizeof(BYTE) * 4) {goto pass2Error;}
    toReturn->dib.resHeightPPM = bytesToUINT32(byteBuffer, 4);

    // colorPalette
    fseek(fp, 0x2EL, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if(returnChk != sizeof(BYTE) * 4) {goto pass2Error;}
    toReturn->dib.colorPalette = bytesToUINT32(byteBuffer, 4);

    // importantColors
    fseek(fp, 0x32L, SEEK_SET);
    returnChk = fread(byteBuffer, sizeof(BYTE), sizeof(BYTE) * 4, fp);
    if(returnChk != sizeof(BYTE) * 4) {goto pass2Error;}
    toReturn->dib.importantColors = bytesToUINT32(byteBuffer, 4);

    fclose(fp);
    free(byteBuffer);
    return toReturn;

    pass2Error:
    errMsg("readBMP", "Reading BMP file failed!");
    free(byteBuffer);
    freeBMP(&toReturn);
    fclose(fp);
    return NULL;


}



