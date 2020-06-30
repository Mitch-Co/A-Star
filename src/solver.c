#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bmp.h"

int main()
{
    BMP* testBMP = readBMP("9x9.bmp");
    freeBMP(&testBMP);
    return 0;
}