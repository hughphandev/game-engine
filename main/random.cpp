#ifndef RANDOM_CPP
#define RANDOM_CPP


#include <time.h>
#include <stdlib.h>
#include "random.h"

float Rand(float a, float b)
{
    float r = (float)rand() / (float)RAND_MAX;

    float delta = b - a;
    return a + (delta * r);
}

#endif