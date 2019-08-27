#pragma once
#include <list>
#include "pngLoader.h"

extern float mask[];

Image_t* filter2D(Image_t* im, float* mask);

Image_t* filter2D_Paralell_3(Image_t* im, float* mask, int numThreads, int packageSize);

void filter2D_Thread3(Image_t* im, Image_t* imOut, float* mask, std::list<work_package_t*>* packagesList);

std::list<work_package_t*>* createPackagesList(int rows, int packageSize);
