#pragma once
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>
#include <ratio>
#include "pngLoader.h"

Image_t* filter2D_CUDA_1(Image_t* im, float* mask, int numThreadsKernel);
Image_t* filter2D_CUDA_SM(Image_t* img, float* mask, int numThreadsKernel);
Image_t* filter2D_CUDA_Async(Image_t* img, float* mask, int numThreadsKernel);
Image_t* filter2D(Image_t* im, float* mask);
