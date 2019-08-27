#pragma once
#include "pngLoader.h"

extern float mask[];

Image_t* filter2D(Image_t* im, float* mask);

Image_t* filter2D_Paralell_1(Image_t* im, float* mask, int numThreads);

void filter2D_Thread1(Image_t* im, Image_t* imOut, float* mask, int x, int y);