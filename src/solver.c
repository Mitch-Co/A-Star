#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bmp.h"

int main()
{
    BMP* testBMP = readBMP("6x6.bmp");
    writeBMP("test.bmp",testBMP);
    freeBMP(&testBMP);
    return 0;
}