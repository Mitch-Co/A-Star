#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "bmp.h"

int main()
{
    free(readBMP("9x9.bmp"));
    return 0;
}