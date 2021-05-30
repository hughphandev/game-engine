#ifndef JUSA_RANDOM_H
#define JUSA_RANDOM

#include <time.h>
#include <stdlib.h>

float Rand(float a, float b)
{
    float r = (float)rand() / (float)RAND_MAX;

    float delta = b - a;
    return a + (delta * r);
}

#endif