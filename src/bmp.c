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
    if(bytesToUINT16(byteBuffer, 2) != bmpSignature)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap signature)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }

    // Check that filesize is listed correctly
    long fileSize = 0;
    uint32_t listedSize = 0;

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
    if(bytesToUINT32(byteBuffer, 4) < 40)
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
    if(bytesToUINT32(byteBuffer, 4) != 0x0)
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
    int bitDepth = bytesToUINT32(byteBuffer, 4);
    if(bitDepth != 32 && bitDepth != 24)
    {
        errMsg("readBMP", "Bitmap file has unknown depth (only bit depths of 24 or 32 bits are able to be read)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }



    
    // unsigned char* buffer = malloc(sizeof(char));
    // FILE* fp;

    // fp = fopen("9x9.bmp","rb");

    // int count = 0;

    // while(fread(buffer,sizeof(char),1,fp) != 0)
    // {
    //     if(count >= 10)
    //     {
    //         printf("\n");
    //         count = 0;
    //     }
    //     printf("%x ",*buffer);
    //     count++;
    // }

    fclose(fp);
    free(byteBuffer);
    return toReturn;

}



