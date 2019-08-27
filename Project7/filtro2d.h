#pragma once
#include "pngLoader.h"

extern float mask[];

Image_t* filter2D(Image_t* im, float* mask);

