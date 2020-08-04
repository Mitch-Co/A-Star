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
    BMP* temp = (*toFree);
    if(temp->data.colorData != NULL)
    {
        free(temp->data.colorData);
    }
    free(*toFree);
}

BMP* newBMP()
{
    BMP* toReturn = malloc(sizeof(BMP));
    toReturn->data.colorData = NULL;
    return toReturn;
}

// NOTE: BYTEARRAY IN LITTLE ENDIAN
// OUTPUT IN BIG ENDIAN
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
// OUTPUT IN BIG ENDIAN
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



BMP* readBMP(char* fileName) 
{
    /* INITIALIZATION AND ERROR CHECKING */

    if(fileName == NULL)
    {
        return NULL;
    }
    if(!endsWith(fileName,".bmp"))
    {
        return NULL;
    }

    // Open bitmap file to read
    FILE* fp = NULL;
    fp = fopen(fileName, "rb");
    if(fp == NULL)
    {
        return NULL;
    }

    // Allocate memory for bitmap struct
    BMP* toReturn = newBMP();
    if(toReturn == NULL)
    {
        fclose(fp);
        return NULL;
    }

    // Read headers
    if(!readHeader(toReturn,fp))
    {
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
    if(!readDIB(toReturn,fp))
    {
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }

    // Read data
    if(!readData(toReturn,fp))
    {
        freeBMP(&toReturn);
        fclose(fp);
    }


    fclose(fp);
    return toReturn;
}

bool readHeader(BMP* toReturn, FILE* fp)
{
    if(toReturn == NULL || fp == NULL)
    {
        return false;
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

    // Used to check the return value of various file functions
    int returnChk = 0;

    // Actual fileSize stored here - to compare against listedSize for verification of size
    long fileSize = 0;

    // Temp variables
    uint16_t signiture = 0;
    uint32_t listedSize = 0;
    uint32_t headerSize = 0;
    uint32_t compression = 0;
    uint16_t bitDepth = 0;

    // Start reading at the beginning of the file
    rewind(fp);

    // Check for bitmap header signature at beginning of file
    returnChk = fread(&signiture, sizeof(uint16_t), 1, fp);
    if (returnChk != 1 || signiture != bmpSignature)
    {
        return false;
    }

    // Seek to end of file to find actual length
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);

    // Seek to 0x2 (long) (position of fileSize)
    fseek(fp, 0x2L, SEEK_SET);
    returnChk = fread(&listedSize, sizeof(uint32_t), 1, fp);
    if(returnChk != 1)
    {
        return NULL;
    }
    if(listedSize != fileSize)
    {
        //WARNING MESSAGE GOES HERE
        printf("\n[WARNING] BITMAP HEADER LISTED SIZE NOT EQUAL TO ACTUAL SIZE [WARNING]\n");
        printf("\nActual file size = %ld\nFile Size in BMP Head = %d\n", fileSize, listedSize);
        printf("\n[WARNING] BITMAP HEADER LISTED SIZE NOT EQUAL TO ACTUAL SIZE [WARNING]\n");
    }

    //Check if header is BITMAPINFOHEADER or newer
    fseek(fp, 0x0EL, SEEK_SET);
    returnChk = fread(&headerSize, sizeof(uint32_t), 1, fp);
    if(returnChk != 1 || headerSize < 40)
    {
        return false;
    }
    
    // Check file compression
    fseek(fp, 0x1EL, SEEK_SET);
    returnChk = fread(&compression, sizeof(uint32_t), 1, fp);
    if (returnChk != 1 || compression != 0x0)
    {
        return false;
    }

    //Check bit depth
    fseek(fp, 0x1CL, SEEK_SET);
    returnChk = fread(&bitDepth, sizeof(uint16_t), 1, fp);
    if (returnChk != 1)
    {
        return false;
    }

    //TODO: TEST IF 16, 12, and 8 bpp bitmaps work with reading/writing (they should with little to no change)
    //TODO: ACTUALLY TO ADD LESSER COLORS YOU NEED TO ADD SUPPORT FOR THE COLOR TABLE
    //TODO: ADD SUPPORT FOR THE COLOR TABLE
    if(bitDepth != 32 && bitDepth != 24 && bitDepth != 4)
    {
        printf("BITMAP DEPTH = %d\n", bitDepth);
        return false;
    }

    // Copy all the verified header data to the bitmap struct
    toReturn->head.signiture = signiture;

    //Use the actual size instead of the size the file tells us
    toReturn->head.fileSize = fileSize;

    toReturn->dib.headerSize = headerSize;
    toReturn->dib.compression = compression;
    toReturn->dib.bitsPerPixel = bitDepth;
    
    return true;
}

bool readDIB(BMP* toReturn, FILE* fp)
{
    if(toReturn == NULL || fp == NULL)
    {
        return false;
    }
    /* START PASS 2 */

    /*
        Pass 2 is meant to fill the BMP struct with all data from the bitmap file.
        Some data stored redundantly; the total memory size of all structs 
        should be around 1.5 - 2.2x the size of the file.
    */

    // Used to check the return value of various file functions
    int returnChk = 0;

    // Grab the rest of the header/DIB
    // General format as follows:
    // Seek to data location -> Grab data -> Check if data grabbed -> Put in struct

    // offset
    fseek(fp, 0xAL, SEEK_SET);
    returnChk = fread(&(toReturn->head.offset), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {return false;}

    // bmpWidth
    fseek(fp, 0x12L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.bmpWidth), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {return false;}

    // bmpHeight
    fseek(fp, 0x16L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.bmpHeight),sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {return false;}

    // colorPlanes
    fseek(fp, 0x1AL, SEEK_SET);
    returnChk = fread(&(toReturn->dib.colorPlanes), sizeof(uint16_t), 1, fp);
    if(returnChk != 1) {return false;}

    // imageSize
    fseek(fp, 0x22L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.imageSize), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {return false;}

    // resWidthPPM
    fseek(fp, 0x26L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.resWidthPPM), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {return false;}

    // resHeightPPM
    fseek(fp, 0x2AL, SEEK_SET);
    returnChk = fread(&(toReturn->dib.resHeightPPM), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {return false;}

    // colorPalette
    fseek(fp, 0x2EL, SEEK_SET);
    returnChk = fread(&(toReturn->dib.colorPalette), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {return false;}

    // importantColors
    fseek(fp, 0x32L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.importantColors), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {return false;}

    /*
        Just your daily reminder that height and width are signed.
    */
    if(toReturn->dib.bmpWidth <= 0 || toReturn->dib.bmpHeight <= 0)
    {
        return false;
    }

    return true;
}

bool readData(BMP* toReturn, FILE* fp)
{
    if(toReturn == NULL || fp == NULL)
    {
        return false;
    }

    bool success = true;
    if(toReturn->dib.bitsPerPixel < 8)
    {
        success = readDataBits(toReturn, fp);
    }
    else
    {
        success = readDataBytes(toReturn, fp);
    }
    
    return success;
}

bool readDataBits(BMP* toReturn, FILE* fp)
{
    if(toReturn == NULL || fp == NULL)
    {
        return false;
    }

    int rowSize = 0;
    int rowPadding = 0;

    rowSize = ((toReturn->dib.bmpWidth * toReturn->dib.bitsPerPixel) + 31) / 32;
    rowSize = rowSize * 4;

    //rowSize is in bytes, and we want it in bits
    rowSize = rowSize * 8;

    rowPadding = rowSize - (toReturn->dib.bmpWidth * toReturn->dib.bitsPerPixel);

    /* START FILLING IN BMPDATA*/
    toReturn->data.width = toReturn->dib.bmpWidth;
    toReturn->data.height = toReturn->dib.bmpHeight;
    toReturn->data.area = toReturn->data.width * toReturn->data.height;
    toReturn->data.bitDepth = toReturn->dib.bitsPerPixel;
    // TODO: hasAlpha, bitsForAlpha
    
    int tempHeight = toReturn->data.height;
    int tempWidth = toReturn->data.width;
    int tempBPP = toReturn->data.bitDepth;
    int returnChk = 0;
    int bitsUntilBufferEnd = 0;
    
    uint8_t bufferByte = 0;
    uint8_t bitMask = (0xFF << (8 - tempBPP));

    PIXEL* pixArray = malloc(sizeof(PIXEL) * toReturn->data.area);

    fseek(fp, toReturn->head.offset, SEEK_SET);
    for(int y = 0; y < tempHeight; y++)
    {
        for (int x = 0; x < tempWidth; x++)
        {
            if(bitsUntilBufferEnd == 0)
            {
                returnChk = fread(&bufferByte, sizeof(uint8_t), 1, fp);
                if(returnChk != 1)
                {
                    return false;
                }
                bitsUntilBufferEnd = 8;
                bitMask = (0xFF << (8 - tempBPP));
            };
            pixArray[x + (tempWidth * y)].value = (bufferByte & bitMask)>>(bitsUntilBufferEnd - tempBPP);
            bitMask >>= tempBPP;
            bitsUntilBufferEnd -= tempBPP;
        }
        if(rowPadding/8 > 0)
        {
            // For some reason fseeking to skip the filler bytes does not work
            //fseek(fp, (rowPadding/8), SEEK_CUR);
            fread(&bufferByte, sizeof(uint8_t), rowPadding/8, fp);
        }
    }

    toReturn->data.colorData = pixArray;

    for(int y = 0; y < tempHeight; y++)
    {
        printf("y = %d - ", y);
        for (int x = 0; x < tempWidth; x++)
        {
            printf("%x ",(toReturn->data.colorData)[x + (tempWidth * y)].value);
        }
        printf("\n");
    }
    

    return true;
}

bool readDataBytes(BMP* toReturn, FILE* fp)
{
    if(toReturn == NULL || fp == NULL)
    {
        return false;
    }
    /* READ IN BMP PIXEL DATA */

    /*
        NOTE: This rowSize calculation looks complicated because
        I did not want to link the math library, and as such abused
        the fact that c uses a floor function to force division into ints.
        The proper rowSize calculation is as follows:
        RowSize = (ceil(BitsPerPixel * ImageWidth)/32) * 4

        This rounds the RowSize up to a multiple of 4 bytes.
    */
    int rowSize = 0;
    int rowPadding = 0;
    int bytesPerPixel = toReturn->dib.bitsPerPixel/8;

    rowSize = ((toReturn->dib.bmpWidth * toReturn->dib.bitsPerPixel) + 31) / 32;
    rowSize = rowSize * 4;

    // Padding is equal to the (bytes) rowsize
    // minus the amount of bytes used to store the pixel data
    rowPadding = rowSize - (toReturn->dib.bmpWidth * (bytesPerPixel));

    /* START FILLING IN BMPDATA*/
    toReturn->data.width = toReturn->dib.bmpWidth;
    toReturn->data.height = toReturn->dib.bmpHeight;
    toReturn->data.area = toReturn->data.width * toReturn->data.height;
    toReturn->data.bitDepth = toReturn->dib.bitsPerPixel;
    // TODO: hasAlpha, bitsForAlpha

    PIXEL* pixArray = malloc(sizeof(PIXEL) * toReturn->data.area);

    // Start at beginning of color data
    fseek(fp, toReturn->head.offset, SEEK_SET);

    // Used so for loops dont have to do pointer junk every iteration
    int tempHeight = toReturn->data.height;
    int tempWidth = toReturn->data.width;

    int returnChk = 0;
    uint32_t tempInt = 0;
    BYTE* byteBuffer = malloc(sizeof(BYTE)* 8);
    // Read in all pixel data
    for(int y = 0; y < tempHeight; y++)
    {
        for (int x = 0; x < tempWidth; x++)
        {
            // Grab pixel
            returnChk = fread(byteBuffer, sizeof(BYTE), bytesPerPixel, fp);
            if(returnChk != bytesPerPixel)
            {
                return false;
            }

            //Swap endianness of byteBuffer
            tempInt = 0x0;
            for(int i = bytesPerPixel - 1; i >= 0; i--)
            {
                // Bitshift by 8 to bump the previous byte added up one byte
                // EXAMPLE: 0000000011111111 --> 1111111100000000
                tempInt <<= 8;
                tempInt += byteBuffer[i];
            }
            //Save to array
            pixArray[x + (tempWidth * y)].value = tempInt;
        }
        // Skip row padding
        fseek(fp, rowPadding, SEEK_CUR);
    }
    
    toReturn->data.colorData = pixArray;
    
    // TODO: Remove this test print statement
    // for(int y = 0; y < tempHeight; y++)
    // {
    //     printf("y = %d - ", y);
    //     for (int x = 0; x < tempWidth; x++)
    //     {
    //         printf("%x ",(toReturn->data.colorData)[x + (tempWidth * y)].value);
    //     }
    //     printf("\n");
    // }
    

    return true;
}

bool endsWith(char* toCheck, char* ending)
{
    // toCheck holds something like "imafile.bmp"
    // ending holds something like ".bmp"
    // This function checks if toCheck ends with ending

    if(toCheck == NULL || ending == NULL)
    {
        return false;
    }

    int strLength = strlen(toCheck);
    int endLength = strlen(ending);

    // toCheck must be at least long enough to hold ending
    if(endLength > strLength)
    {
        return false;
    }

    int offset = strLength - endLength;
    for (int i = offset; i < strLength; i++)
    {
        if(toCheck[i] != ending[i - offset])
        {
            return false;
        }
    }

    return true;
}


bool writeBMP(BMP* toWrite, char* fileName)
{
    /* INITIALIZATION AND ERROR CHECKING */

    if(fileName == NULL || toWrite == NULL)
    {
        return NULL;
    }
    if(!endsWith(fileName,".bmp"))
    {
        return NULL;
    }

    // Open new file
    FILE* fp = NULL;
    fp = fopen(fileName, "wb");
    if(fp == NULL)
    {
        return NULL;
    }

    // Used to check the return value of various file functions
    int returnChk = 0;
    
    // Set file pointer to beginning of file (unnessisary)
    fseek(fp, 0x0, SEEK_SET);

    // Used to store the new file size as stuff is written
    uint32_t fileSize = 0x0;

    // Temp variables used in fwrite to write static data
    uint16_t toWrite16 = 0x0;
    uint32_t toWrite32 = 0x0;

    /* START WRITING TO FILE */
    fseek(fp, 0x0, SEEK_SET);

    // Write BMP signiture
    toWrite16 = bmpSignature;
    returnChk = fwrite(&toWrite16, sizeof(uint16_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint16_t); 

    // Skip filesize, reserved and offset, go to BMP header 
    fseek(fp, 0x0E, SEEK_SET);
    fileSize += sizeof(uint32_t) * 3;

    // Write header size (40 for BITMAPINFOHEADER)
    toWrite32 = 0x28; // 40 in decimal
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint32_t);

    // Write bmpWidth
    toWrite32 = toWrite->dib.bmpWidth;
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint32_t);
    
    // Write bmpHeight
    toWrite32 = toWrite->dib.bmpHeight;
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint32_t);
    
    // Write colorPlanes
    toWrite16 = 1; // Must be 1 according to spec
    returnChk = fwrite(&toWrite16, sizeof(uint16_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint16_t);

    // Write bitsPerPixel
    toWrite16 = toWrite->dib.bitsPerPixel;
    returnChk = fwrite(&toWrite16, sizeof(uint16_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint16_t);

    // Write compression
    toWrite32 = 0; // No compression
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint32_t);

    // Write imageSize
    toWrite32 = toWrite->dib.imageSize;
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint32_t);

    // Write resWidthPPM
    toWrite32 = 0; // This is dumb and nobody uses this
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint32_t);

    // Write resHeightPPM
    toWrite32 = 0; // This is dumb and nobody uses this
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
     fileSize += sizeof(uint32_t);

    // Write colorPalette
    toWrite32 = toWrite->dib.colorPalette; 
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint32_t);

    // Write importantColors
    toWrite32 = toWrite->dib.importantColors; 
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }
    fileSize += sizeof(uint32_t);

    // Go to and write offset
    fseek(fp, 0x0A, SEEK_SET);

    toWrite32 = fileSize; 
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }

    if(!writeData(toWrite, fp, &fileSize))
    {
        goto writeError;
    }

    // Go to and write file size
    fseek(fp, 0x02, SEEK_SET);

    toWrite32 = fileSize; 
    returnChk = fwrite(&toWrite32, sizeof(uint32_t), 1, fp);
    if(returnChk != 1) { goto writeError; }

    fclose(fp);
    return true;

    writeError:
    fclose(fp);
    errMsg("writeBMP", "Writing to BMP file failed!");
    return false;
}

bool writeData(BMP* toWrite, FILE* fp, uint32_t* fileSize)
{
    if(toWrite == NULL || fp == NULL || fileSize == NULL)
    {
        return false;
    }

    bool success = true;
    if(toWrite->dib.bitsPerPixel < 8)
    {
        success = writeDataBits(toWrite, fp, fileSize);
    }
    else
    {
        success = writeDataBytes(toWrite, fp, fileSize);
    }
    
    return success;
}

bool writeDataBits(BMP* toWrite, FILE* fp, uint32_t* fileSize)
{
    if(toWrite == NULL || fp == NULL || fileSize == NULL)
    {
        return false;
    }

    // Return to current position and start writing pixel data
    fseek(fp, (*fileSize), SEEK_SET);

    int returnChk = 0;

    int rowSize = 0;
    int rowPadding = 0;

    rowSize = ((toWrite->dib.bmpWidth * toWrite->dib.bitsPerPixel) + 31) / 32;
    rowSize = rowSize * 4;

    //rowSize is in bytes, and we want it in bits
    rowSize = rowSize * 8;
    rowPadding = rowSize - (toWrite->dib.bmpWidth * toWrite->dib.bitsPerPixel);

    uint8_t packedByte = 0;
    int currentBitsInByte = 8;
    int rowTarget = rowSize;

    int tempBPP = toWrite->data.bitDepth;
    int numRows = toWrite->data.height;
    int pixelsPerRow = toWrite->data.width;

    for(int y = 0; y < numRows; y++)
    {
        rowTarget = rowSize;
        currentBitsInByte = 8;
        for(int x = 0; x < pixelsPerRow; x++)
        {
            rowTarget -= tempBPP;
            packedByte <<= tempBPP;
            packedByte += (toWrite->data.colorData)[x + (pixelsPerRow * y)].value;
            currentBitsInByte -= tempBPP;

            if(currentBitsInByte <= 0)
            {
                returnChk = fwrite(&packedByte, sizeof(uint8_t), 1, fp);
                if(returnChk != 1) { return false; }
                (*fileSize) += 1;
                currentBitsInByte = 8;
                packedByte = 0;
            }

        }
        while(rowTarget > 0)
        {
            rowTarget -= tempBPP;
            packedByte <<= tempBPP;
            currentBitsInByte -= tempBPP;

            if(currentBitsInByte <= 0)
            {
                returnChk = fwrite(&packedByte, sizeof(uint8_t), 1, fp);
                if(returnChk != 1) { return false; }
                (*fileSize) += 1;
                currentBitsInByte = 8;
                packedByte = 0;
            }
        }

    }

    return true;
}

bool writeDataBytes(BMP* toWrite, FILE* fp, uint32_t* fileSize)
{
    // Return to current position and start writing pixel data
    fseek(fp, (*fileSize), SEEK_SET);

    const uint32_t zero = 0x0;
    int returnChk = 0x0;

    int rowSize = 0;
    int rowPadding = 0;
    int pixelsPerRow = toWrite->dib.bmpWidth;
    int numRows = toWrite->dib.bmpHeight;

    int bytesPerPixel = toWrite->dib.bitsPerPixel/8;
    rowSize = ((toWrite->dib.bmpWidth * toWrite->dib.bitsPerPixel) + 31) / 32;
    rowSize = rowSize * 4;
    rowPadding = rowSize - (toWrite->dib.bmpWidth * (bytesPerPixel));
    printf ("\nWRITING - numrows = %d, rowPadding = %d, pixelsPerRow = %d, bytesPerPixel = %d\n",numRows,rowPadding,pixelsPerRow, bytesPerPixel);
    for(int y = 0; y < numRows; y++)
    {
        for(int x = 0; x < pixelsPerRow; x++)
        {
            returnChk = fwrite(&((toWrite->data.colorData)[x + (pixelsPerRow * y)].value), bytesPerPixel, 1, fp);
            if(returnChk != 1) { return false; }
            (*fileSize) += bytesPerPixel;
        }
        if(rowPadding != 0)
        {
            returnChk = fwrite(&zero, rowPadding, 1, fp); // Writes zeros for padding
            if(returnChk != 1) { return false; }
            (*fileSize) += rowPadding;
        }

    }
    return true;
}


/* UNUSED FUNCTIONS */
/*
    uint16_t reverseEndian16(uint16_t toReverse)
    {
        return toReverse>>8 | toReverse<<8;
    }
    uint32_t reverseEndian32(uint32_t toReverse)
    {
        // Moves byte 1 to 4, Moves byte 4 to 1, Moves byte 3 to 2, Moves byte 2 to 3
        // The "Magic Numbers" 0xff00 and 0xff0000 are masks to only allow bytes 2 and 3 in the OR operation
        // Bytes 1 and 4 do not need these magic numbers, because shifting left or right 24 bits removes all other bytes besides 1 and 4
        return toReverse>>24 | toReverse<<24 | ((toReverse>>8)&0xff00) | ((toReverse<<8)&0xff0000);
    }
*/
