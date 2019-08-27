#pragma once
#include "pngLoader.h"

extern float mask[];

typedef struct work_package_t
{
	int i_start;
	int i_final;

}work_package_t;

Image_t* filter2D(Image_t* im, float* mask);

Image_t* filter2D_Paralell_2(Image_t* im, float* mask, int numThreads);

void filter2D_Thread2(Image_t* im, Image_t* imOut, float* mask, work_package_t work_package);

work_package_t* createWorkPackages(int rows, int numThreads);