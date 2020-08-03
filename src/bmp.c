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
    uint16_t bitDepth = 0;

    // Check for bitmap header signature at beginning of file
    returnChk = fread(&signiture, sizeof(uint16_t), 1, fp);
    if (returnChk != 1)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap signature)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
    if(signiture != bmpSignature)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap signature)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }

    // Seek to end of file to find length
    fseek(fp, 0, SEEK_END);
    fileSize = ftell(fp);

    // Seek to 0x2 (long) (position of fileSize)
    fseek(fp, 0x2L, SEEK_SET);
    returnChk = fread(&listedSize, sizeof(uint32_t), 1, fp);
    if(returnChk != 1)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid file size)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
    if(listedSize != fileSize)
    {
        printf("\n[WARNING] BITMAP HEADER LISTED SIZE NOT EQUAL TO ACTUAL SIZE [WARNING]\n");
        printf("\nActual file size = %ld\nFile Size in BMP Head = %d\n", fileSize, listedSize);
        printf("\n[WARNING] BITMAP HEADER LISTED SIZE NOT EQUAL TO ACTUAL SIZE [WARNING]\n");
    }

    //Check if header is BITMAPINFOHEADER or newer
    fseek(fp, 0x0EL, SEEK_SET);
    returnChk = fread(&headerSize, sizeof(uint32_t), 1, fp);
    if (returnChk != 1)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap header)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
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
    returnChk = fread(&compression, sizeof(uint32_t), 1, fp);
    if (returnChk != 1)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap header)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }
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
    returnChk = fread(&bitDepth, sizeof(uint16_t), 1, fp);
    if (returnChk != 1)
    {
        errMsg("readBMP", "File is not a bitmap file (does not have valid bitmap header)!");
        free(byteBuffer);
        freeBMP(&toReturn);
        fclose(fp);
        return NULL;
    }

    //TODO: TEST IF 16, 12, and 8 bpp bitmaps work with reading/writing (they should with little to no change)
    //TODO: ACTUALLY TO ADD LESSER COLORS YOU NEED TO ADD SUPPORT FOR THE COLOR TABLE
    //TODO: ADD SUPPORT FOR THE COLOR TABLE
    if(bitDepth != 32 && bitDepth != 24)
    {
        errMsg("readBMP", "Bitmap file has unknown depth (only bit depths of 24 or 32 bits are able to be read)!");
        printf("BITMAP DEPTH = %d\n", bitDepth);
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
    returnChk = fread(&(toReturn->head.offset), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {goto pass2Error;}

    // bmpWidth
    fseek(fp, 0x12L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.bmpWidth), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {goto pass2Error;}

    // bmpHeight
    fseek(fp, 0x16L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.bmpHeight),sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {goto pass2Error;}

    // colorPlanes
    fseek(fp, 0x1AL, SEEK_SET);
    returnChk = fread(&(toReturn->dib.colorPlanes), sizeof(uint16_t), 1, fp);
    if(returnChk != 1) {goto pass2Error;}

    // imageSize
    fseek(fp, 0x22L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.imageSize), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {goto pass2Error;}

    // resWidthPPM
    fseek(fp, 0x26L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.resWidthPPM), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {goto pass2Error;}

    // resHeightPPM
    fseek(fp, 0x2AL, SEEK_SET);
    returnChk = fread(&(toReturn->dib.resHeightPPM), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {goto pass2Error;}

    // colorPalette
    fseek(fp, 0x2EL, SEEK_SET);
    returnChk = fread(&(toReturn->dib.colorPalette), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {goto pass2Error;}

    // importantColors
    fseek(fp, 0x32L, SEEK_SET);
    returnChk = fread(&(toReturn->dib.importantColors), sizeof(uint32_t), 1, fp);
    if(returnChk != 1) {goto pass2Error;}

    /*
        Just your daily reminder that height and width are signed.
        This is not included in pass 1 because having negitive BMP does not mean your
        file is invalid. For some reason...
    */
    if(toReturn->dib.bmpWidth <= 0 || toReturn->dib.bmpHeight <= 0)
    {
        goto pass2Error;
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
    // TODO: bitsPerChannel, hasAlpha(?) bitsForAlpha

    PIXEL* pixArray = malloc(sizeof(PIXEL) * toReturn->data.area);
    //printf("rowsize = %d padding = %d\n", rowSize, rowPadding);

    // Start at beginning of color data
    fseek(fp, toReturn->head.offset, SEEK_SET);

    // Used so for loops dont have to do pointer junk every time they
    // want to check their end condition
    int tempHeight = toReturn->data.height;
    int tempWidth = toReturn->data.width;

    // Read in all pixel data
    for(int y = 0; y < tempHeight; y++)
    {
        for (int x = 0; x < tempWidth; x++)
        {
            returnChk = fread(byteBuffer, sizeof(BYTE), bytesPerPixel, fp);
            if(returnChk != bytesPerPixel)
            {
                goto pass2Error;
            }
            pixArray[x + (tempWidth * y)].value = bytesToUINT32(byteBuffer, bytesPerPixel);
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


bool writeBMP(char fileName[], BMP* toWrite)
{
    /* INITIALIZATION AND ERROR CHECKING */

    if(fileName == NULL || toWrite == NULL)
    {
        errMsg("writeBMP", "input file name or bitmap struct is NULL!");
        return NULL;
    }

    FILE* fp = NULL;
    int fileNameSize = strlen(fileName);

    // File name must be within acceptable bounds
    // File name must be at least long enough to hold ".bmp" and have at least a single character name (5 char)
    if(fileNameSize > longestFileName || fileNameSize < 5)
    {
       errMsg("readBMP", "File name outside of allowable size bounds");
       return NULL;
    }

    // Error triggers if filename does not end in ".bmp"
    if(fileName[fileNameSize - 4] != '.' || fileName[fileNameSize - 3] != 'b' || fileName[fileNameSize - 2] != 'm' || fileName[fileNameSize - 1] != 'p')
    {
       errMsg("writeBMP", "File name not valid \".bmp\" file!");
       return NULL;
    }

    fp = fopen(fileName, "wb");

    // Used to check the return value of various file functions
    int returnChk = 0;
    
    // Set file pointer to beginning of file (unnessisary)
    fseek(fp, 0x0, SEEK_SET);

    /* START WRITING TO FILE */

    // Used to store the new file size as stuff is written
    int32_t fileSize = 0x0;

    // Temp variables used in fwrite to write static data
    uint16_t toWrite16 = 0x0;
    uint32_t toWrite32 = 0x0;
    const uint32_t zero = 0x0;

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

    // Return to current position and start writing pixel data
    fseek(fp, fileSize, SEEK_SET);

    //Calculate padding (same as in readBMP)
    int rowSize = 0;
    int rowPadding = 0;
    int pixelsPerRow = toWrite->dib.bmpWidth;
    int numRows = toWrite->dib.bmpHeight;

    int bytesPerPixel = toWrite->dib.bitsPerPixel/8;
    rowSize = ((toWrite->dib.bmpWidth * toWrite->dib.bitsPerPixel) + 31) / 32;
    rowSize = rowSize * 4;
    rowPadding = rowSize - (toWrite->dib.bmpWidth * (bytesPerPixel));
    
    printf ("\n\n\nWRITING - numrows = %d, rowPadding = %d, pixelsPerRow = %d, bytesPerPixel = %d\n",numRows,rowPadding,pixelsPerRow, bytesPerPixel);
    
    for(int y = 0; y < numRows; y++)
    {
        for(int x = 0; x < pixelsPerRow; x++)
        {
            returnChk = fwrite(&((toWrite->data.colorData)[x + (pixelsPerRow * y)].value), bytesPerPixel, 1, fp);
            if(returnChk != 1) { goto writeError; }
            fileSize += bytesPerPixel;
        }
        if(rowPadding != 0)
        {
            returnChk = fwrite(&zero, rowPadding, 1, fp); // Writes zero for padding
            if(returnChk != 1) { goto writeError; }
            fileSize += rowPadding;
        }

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





