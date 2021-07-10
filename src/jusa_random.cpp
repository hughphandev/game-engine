#include <time.h>
#include <stdlib.h>
#include "jusa_random.h"

float Rand(float a, float b)
{
    float r = (float)rand() / (float)RAND_MAX;

    float delta = b - a;
    return a + (delta * r);
}