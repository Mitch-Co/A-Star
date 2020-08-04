#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "bmp.h"

int main()
{
    char* buffer = malloc(longestFileName * sizeof(char));
    buffer = fgets(buffer, longestFileName, stdin);
    
    int inputSize = strlen(buffer);

    char* name = malloc(inputSize * sizeof(char));
    
    for (int i = 0; i < inputSize; i++ )
    {
        name[i] = buffer[i];
    }
    name[inputSize - 1] = 0;

    BMP* testBMP = readBMP(name);
    writeBMP(testBMP,"test.bmp");
    freeBMP(&testBMP);
    return 0;
}