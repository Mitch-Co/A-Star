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
        return NULL;
    }

    /* START PASS 1 */


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

    

    return toReturn;

}



