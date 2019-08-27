#pragma once
#include "kernel.h"

using namespace std::chrono;

//La macro es una sola linea.
//Macro for checking cuda errors following a cuda launch or api call
#define cudaCheckError() {                                          \
 cudaError_t e=cudaGetLastError();                                 \
 if(e!=cudaSuccess) {                                              \
   printf("Cuda failure %s:%d: '%s'\n",__FILE__,__LINE__,cudaGetErrorString(e));           \
 \
 }                                                                 \
}

__global__ void filter2D_Kernel_1(char** in, char** out, float* mask, int width, int height)
{
	//Seleccionar un pixel de image_in
	int threadIDx = threadIdx.x + blockIdx.x * blockDim.x;
	int threadIDy = threadIdx.y + blockIdx.y * blockDim.y;

	if (threadIDx >= width) return;
	if (threadIDy >= height) return;

	if (threadIDx == 0 || threadIDx == width - 1)	return;
	if (threadIDy == 0 || threadIDy >= height - 1)	return;

	//Aplicar filtro a pixel
	pixel32bpp pixelIn, pixelOut;
	float a, r, g, b;
	a = r = g = b = 0;
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			pixelIn = *((pixel32bpp*)(&(in[threadIDy + i][(threadIDx + j) * 4])));
			a += ((float)pixelIn.a)*mask[(i + 1) * 3 + j + 1];
			r += ((float)pixelIn.r)*mask[(i + 1) * 3 + j + 1];
			g += ((float)pixelIn.g)*mask[(i + 1) * 3 + j + 1];
			b += ((float)pixelIn.b)*mask[(i + 1) * 3 + j + 1];
		}
	}

	//Guardar resultado en image_out
	pixelOut.r = (char)r;
	pixelOut.g = (char)g;
	pixelOut.b = (char)b;
	pixelOut.a = (char)a;
	((pixel32bpp*)out[threadIDy])[threadIDx] = pixelOut;
}


Image_t* filter2D_CUDA_1(Image_t* img, float* mask, int numThreadsKernel)
{
	//Reservar imagen resultado
	Image_t* img_result = (Image_t*)malloc(sizeof(Image_t));
	img_result->height = img->height;
	img_result->width = img->width;
	img_result->color_type = img->color_type;
	img_result->bit_depth = img->bit_depth;
	img_result->row_pointers = (char**)malloc(sizeof(char*)*img->height);
	for (int i = 0; i < img->height; i++)
	{
		img_result->row_pointers[i] = (char*)malloc(sizeof(char)*img->width * 4);
		memset(img_result->row_pointers[i], 0, sizeof(char)*img->width * 4);
	}

	//Copiar datos a GPU
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	
		//reservar memoria para la imagen resultado
	char** row_pointers_result = (char**)malloc(sizeof(char*)*img->height);
	for (int i = 0; i < img->height; i++)
	{
		cudaMalloc((void**)&row_pointers_result[i], sizeof(char)*img->width * 4);
		cudaMemset(row_pointers_result[i], 0, sizeof(char)*img->width * 4);
		cudaCheckError();
	}
	char** d_pixels_result = NULL;
	cudaMalloc((void**)&d_pixels_result, sizeof(char*)*img->height);
	cudaMemset(d_pixels_result, 0, sizeof(char*)*img->height);
	cudaMemcpy(d_pixels_result, row_pointers_result, sizeof(char*)*img->height, cudaMemcpyHostToDevice);
	cudaCheckError();

		//reservar memoria y copiar imagen original
	char** row_pointers = (char**)malloc(sizeof(char*)*img->height);
	for (int i = 0; i < img->height; i++)
	{
		cudaMalloc((void**)&row_pointers[i], sizeof(char)*img->width * 4);
		cudaMemset(row_pointers[i], 0, sizeof(char)*img->width * 4);
		cudaMemcpy(row_pointers[i], img->row_pointers[i], sizeof(char)*img->width * 4, cudaMemcpyHostToDevice);
		cudaCheckError();
	}
	char** d_pixels = NULL;
	cudaMalloc((void**)&d_pixels, sizeof(char*)*img->height);
	cudaMemset(d_pixels, 0, sizeof(char*)*img->height);
	cudaMemcpy(d_pixels, row_pointers, sizeof(char*)*img->height, cudaMemcpyHostToDevice);
	cudaCheckError();

		//copiar el filtro a GPU
	float* d_mask;
	cudaMalloc((void**)&d_mask, 9 * sizeof(float));
	cudaMemset(d_mask, 0, sizeof(float)*9);
	cudaMemcpy(d_mask, mask, sizeof(float)*9, cudaMemcpyHostToDevice);
	cudaCheckError();

	duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	std::cout << "\t Tiempo de copia de datos de CPU a GPU: " << time_span.count() * 1000 << "\n";

	//Ejecutar filter2D_Kernel_1
	dim3 threadsPerBlock = dim3(numThreadsKernel, numThreadsKernel, 1);
	dim3 numBlocks = dim3((img->width / threadsPerBlock.x) + 1, (img->height / threadsPerBlock.y) + 1, 1);

	t1 = high_resolution_clock::now();
	filter2D_Kernel_1 << <numBlocks, threadsPerBlock>> >
		(d_pixels,d_pixels_result, d_mask, img->width, img->height);
	cudaCheckError();
	cudaDeviceSynchronize();
	cudaCheckError();
	time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	std::cout << "\t Tiempo de ejecución del kernel: " << time_span.count() * 1000 << "\n";

	//Copiar datos de GPU
	t1 = high_resolution_clock::now();
	for (int i = 0; i < img->height; i++)
	{
		cudaMemcpy(img_result->row_pointers[i], row_pointers_result[i], sizeof(char) * img->width * 4, cudaMemcpyDeviceToHost);
	}
	cudaCheckError();
	time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	std::cout << "\t Tiempo de copia de datos de GPU a CPU: " << time_span.count() * 1000 << "\n";

	//Liberar memoria
	for (int i = 0; i < img->height; i++)
	{
		cudaFree(row_pointers_result[i]);
		cudaFree(row_pointers[i]);
	}
	free(row_pointers_result);
	free(row_pointers);
	cudaFree(d_pixels_result);
	cudaFree(d_pixels);
	cudaFree(d_mask);

	//Return imagen resultado
	return img_result;
}

extern __shared__ char sharedMem[];

__global__ void filter2D_Kernel_SM(char** in, char** out, float* mask, int width, int height)
{
	//Seleccionar un pixel de image_in
	int threadIDx = threadIdx.x + blockIdx.x * blockDim.x;
	int threadIDy = threadIdx.y + blockIdx.y * blockDim.y;

	if (threadIDx >= width) return;
	if (threadIDy >= height) return;

	int lindex = (threadIdx.y + 1) * (blockDim.x + 2) + (threadIdx.x + 1);

	((pixel32bpp*)sharedMem)[lindex] = ((pixel32bpp*)in[threadIDy])[threadIDx];

	if (threadIdx.x == 0 && threadIDx != 0) {
		((pixel32bpp*)sharedMem)[lindex - 1] = ((pixel32bpp*)in[threadIDy])[threadIDx - 1];
		if (threadIdx.y == blockDim.y - 1 && threadIDy != height - 1) {
			((pixel32bpp*)sharedMem)[lindex + (blockDim.x + 2) - 1] = ((pixel32bpp*)in[threadIDy + 1])[threadIDx - 1];
		}
	}

	if (threadIdx.x == blockDim.x - 1 && threadIDx != width - 1) {
		((pixel32bpp*)sharedMem)[lindex + 1] = ((pixel32bpp*)in[threadIDy])[threadIDx + 1];
		if (threadIdx.y == 0 && threadIDy != 0) {
			((pixel32bpp*)sharedMem)[lindex - (blockDim.x + 2) + 1] = ((pixel32bpp*)in[threadIDy - 1])[threadIDx + 1];
		}
	}

	if (threadIdx.y == 0 && threadIDy != 0) {
		((pixel32bpp*)sharedMem)[lindex - (blockDim.x + 2)] = ((pixel32bpp*)in[threadIDy - 1])[threadIDx];
		if (threadIdx.x == 0 && threadIDx != 0) {
			((pixel32bpp*)sharedMem)[lindex - (blockDim.x + 2) - 1] = ((pixel32bpp*)in[threadIDy - 1])[threadIDx - 1];
		}
	}

	if (threadIdx.y == blockDim.y - 1 && threadIDy != height - 1) {
		((pixel32bpp*)sharedMem)[lindex + (blockDim.x + 2)] = ((pixel32bpp*)in[threadIDy + 1])[threadIDx];
		if (threadIdx.x == blockDim.x - 1 && threadIDx != width - 1) {
			((pixel32bpp*)sharedMem)[lindex + (blockDim.x + 2) + 1] = ((pixel32bpp*)in[threadIDy + 1])[threadIDx + 1];
		}
	}
	
	if (threadIDx == 0 || threadIDx == width - 1)	return;
	if (threadIDy == 0 || threadIDy == height - 1)	return;

	__syncthreads();

	//Aplicar filtro a pixel
	pixel32bpp pixelIn, pixelOut;
	float a, r, g, b;
	a = r = g = b = 0;
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			pixelIn = ((pixel32bpp*)sharedMem)[(threadIdx.y + 1 + i) * (blockDim.x + 2) + (threadIdx.x + 1 + j)];
			a += ((float)pixelIn.a)*mask[(i + 1) * 3 + j + 1];
			r += ((float)pixelIn.r)*mask[(i + 1) * 3 + j + 1];
			g += ((float)pixelIn.g)*mask[(i + 1) * 3 + j + 1];
			b += ((float)pixelIn.b)*mask[(i + 1) * 3 + j + 1];
		}
	}
	
	//Guardar resultado en image_out
	pixelOut.r = (char)r;
	pixelOut.g = (char)g;
	pixelOut.b = (char)b;
	pixelOut.a = (char)a;
	((pixel32bpp*)out[threadIDy])[threadIDx] = pixelOut;
}

Image_t* filter2D_CUDA_SM(Image_t* img, float* mask, int numThreadsKernel)
{
	//Reservar imagen resultado
	Image_t* img_result = (Image_t*)malloc(sizeof(Image_t));
	img_result->height = img->height;
	img_result->width = img->width;
	img_result->color_type = img->color_type;
	img_result->bit_depth = img->bit_depth;
	img_result->row_pointers = (char**)malloc(sizeof(char*)*img->height);
	for (int i = 0; i < img->height; i++)
	{
		img_result->row_pointers[i] = (char*)malloc(sizeof(char)*img->width * 4);
		memset(img_result->row_pointers[i], 0, sizeof(char)*img->width * 4);
	}

	//Copiar datos a GPU
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
		//reservar memoria para la imagen resultado
	char** row_pointers_result = (char**)malloc(sizeof(char*)*img->height);
	for (int i = 0; i < img->height; i++)
	{
		cudaMalloc((void**)&row_pointers_result[i], sizeof(char)*img->width * 4);
		cudaMemset(row_pointers_result[i], 0, sizeof(char)*img->width * 4);
		cudaCheckError();
	}
	char** d_pixels_result = NULL;
	cudaMalloc((void**)&d_pixels_result, sizeof(char*)*img->height);
	cudaMemset(d_pixels_result, 0, sizeof(char*)*img->height);
	cudaMemcpy(d_pixels_result, row_pointers_result, sizeof(char*)*img->height, cudaMemcpyHostToDevice);
	cudaCheckError();

	//reservar memoria y copiar imagen original
	char** row_pointers = (char**)malloc(sizeof(char*)*img->height);
	for (int i = 0; i < img->height; i++)
	{
		cudaMalloc((void**)&row_pointers[i], sizeof(char)*img->width * 4);
		cudaMemset(row_pointers[i], 0, sizeof(char)*img->width * 4);
		cudaMemcpy(row_pointers[i], img->row_pointers[i], sizeof(char)*img->width * 4, cudaMemcpyHostToDevice);
		cudaCheckError();
	}
	char** d_pixels = NULL;
	cudaMalloc((void**)&d_pixels, sizeof(char*)*img->height);
	cudaMemset(d_pixels, 0, sizeof(char*)*img->height);
	cudaMemcpy(d_pixels, row_pointers, sizeof(char*)*img->height, cudaMemcpyHostToDevice);
	cudaCheckError();

	//copiar el filtro a GPU
	float* d_mask;
	cudaMalloc((void**)&d_mask, 9 * sizeof(float));
	cudaMemset(d_mask, 0, sizeof(float) * 9);
	cudaMemcpy(d_mask, mask, sizeof(float) * 9, cudaMemcpyHostToDevice);
	cudaCheckError();
	duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	std::cout << "\t Tiempo de copia de datos de CPU a GPU: " << time_span.count() * 1000 << "\n";

	//Ejecutar filter2D_Kernel_1
	dim3 threadsPerBlock = dim3(numThreadsKernel, numThreadsKernel, 1);
	dim3 numBlocks = dim3((img->width / threadsPerBlock.x) + 1, (img->height / threadsPerBlock.y) + 1, 1);

	t1 = high_resolution_clock::now();
	filter2D_Kernel_SM << <numBlocks, threadsPerBlock, (numThreadsKernel + 2)*(numThreadsKernel + 2) * 4>> >
		(d_pixels, d_pixels_result, d_mask, img->width, img->height);
	cudaCheckError();
	cudaDeviceSynchronize();
	cudaCheckError();
	time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	std::cout << "\t Tiempo de ejecucion del kernel: " << time_span.count() * 1000 << "\n";

	//Copiar datos de GPU
	t1 = high_resolution_clock::now();
	for (int i = 0; i < img->height; i++)
	{
		cudaMemcpy(img_result->row_pointers[i], row_pointers_result[i], sizeof(char) * img->width * 4, cudaMemcpyDeviceToHost);
	}
	cudaCheckError();
	time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	std::cout << "\t Tiempo de copia de datos de GPU a CPU: " << time_span.count() * 1000 << "\n";

	//Liberar memoria
	for (int i = 0; i < img->height; i++)
	{
		cudaFree(row_pointers_result[i]);
		cudaFree(row_pointers[i]);
	}
	free(row_pointers_result);
	free(row_pointers);
	cudaFree(d_pixels_result);
	cudaFree(d_pixels);
	cudaFree(d_mask);

	//Return imagen resultado
	return img_result;
}

Image_t* filter2D_CUDA_Async(Image_t* img, float* mask, int numThreadsKernel)
{
	//Crear stream
	cudaStream_t stream1;
	cudaStreamCreate(&stream1);

	//Reservar imagen resultado
	Image_t* img_result = (Image_t*)malloc(sizeof(Image_t));
	img_result->height = img->height;
	img_result->width = img->width;
	img_result->color_type = img->color_type;
	img_result->bit_depth = img->bit_depth;
	img_result->row_pointers = (char**)malloc(sizeof(char*)*img->height);
	for (int i = 0; i < img->height; i++)
	{
		img_result->row_pointers[i] = (char*)malloc(sizeof(char)*img->width * 4);
		memset(img_result->row_pointers[i], 0, sizeof(char)*img->width * 4);
	}

	//Copiar datos a GPU
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
		//reservar memoria para la imagen resultado
	char** row_pointers_result = (char**)malloc(sizeof(char*)*img->height);
	for (int i = 0; i < img->height; i++)
	{
		cudaMalloc((void**)&row_pointers_result[i], sizeof(char)*img->width * 4);
		cudaMemsetAsync(row_pointers_result[i], 0, sizeof(char)*img->width * 4, stream1);
	}
	char** d_pixels_result = NULL;
	cudaMalloc((void**)&d_pixels_result, sizeof(char*)*img->height);
	cudaMemsetAsync(d_pixels_result, 0, sizeof(char*)*img->height, stream1);
	cudaMemcpyAsync(d_pixels_result, row_pointers_result, sizeof(char*)*img->height, cudaMemcpyHostToDevice, stream1);

	//reservar memoria y copiar imagen original
	char** row_pointers = (char**)malloc(sizeof(char*)*img->height);
	for (int i = 0; i < img->height; i++)
	{
		cudaMalloc((void**)&row_pointers[i], sizeof(char)*img->width * 4);
		cudaMemsetAsync(row_pointers[i], 0, sizeof(char)*img->width * 4, stream1);
		cudaMemcpyAsync(row_pointers[i], img->row_pointers[i], sizeof(char)*img->width * 4, cudaMemcpyHostToDevice, stream1);
	}
	char** d_pixels = NULL;
	cudaMalloc((void**)&d_pixels, sizeof(char*)*img->height);
	cudaMemsetAsync(d_pixels, 0, sizeof(char*)*img->height, stream1);
	cudaMemcpyAsync(d_pixels, row_pointers, sizeof(char*)*img->height, cudaMemcpyHostToDevice, stream1);

	//copiar el filtro a GPU
	float* d_mask;
	cudaMalloc((void**)&d_mask, 9 * sizeof(float));
	cudaMemsetAsync(d_mask, 0, sizeof(float) * 9, stream1);
	cudaMemcpyAsync(d_mask, mask, sizeof(float) * 9, cudaMemcpyHostToDevice, stream1);
	duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	std::cout << "\t Tiempo de copia de datos de CPU a GPU: " << time_span.count() * 1000 << "\n";

	//Ejecutar filter2D_Kernel_SM
	dim3 threadsPerBlock = dim3(numThreadsKernel, numThreadsKernel, 1);
	dim3 numBlocks = dim3((img->width / threadsPerBlock.x) + 1, (img->height / threadsPerBlock.y) + 1, 1);

	t1 = high_resolution_clock::now();
	filter2D_Kernel_1 << <numBlocks, threadsPerBlock, 0, stream1 >> >
		(d_pixels, d_pixels_result, d_mask, img->width, img->height);
	time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	std::cout << "\t Tiempo de ejecucion del kernel: " << time_span.count() * 1000 << "\n";

	//Copiar datos de GPU
	t1 = high_resolution_clock::now();
	for (int i = 0; i < img->height; i++)
	{
		cudaMemcpyAsync(img_result->row_pointers[i], row_pointers_result[i], sizeof(char) * img->width * 4, cudaMemcpyDeviceToHost, stream1);
	}
	time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	std::cout << "\t Tiempo de copia de datos de GPU a CPU: " << time_span.count() * 1000 << "\n";

	//Sincronizar memoria
	cudaStreamSynchronize(stream1);
	cudaCheckError();

	//Liberar memoria
	for (int i = 0; i < img->height; i++)
	{
		cudaFree(row_pointers_result[i]);
		cudaFree(row_pointers[i]);
	}
	free(row_pointers_result);
	free(row_pointers);
	cudaFree(d_pixels_result);
	cudaFree(d_pixels);
	cudaFree(d_mask);

	//Return imagen resultado
	return img_result;
}

Image_t* filter2D(Image_t* im, float* mask)
{
	Image_t* imOut = new Image_t;
	(*imOut) = (*im);
	imOut->row_pointers = (char**)malloc(sizeof(char*)*im->height);
	for (int i = 0; i < im->height; i++)
	{
		imOut->row_pointers[i] = (char*)malloc(sizeof(char)*im->width * 4);
		memset(imOut->row_pointers[i], 0, sizeof(char)*im->width * 4);
	}

	for (int i = 1; i < (im->height - 1); i++)
		for (int j = 1; j < (im->width - 1); j++)
		{
			pixel32bpp pixlIn;
			float a, r, g, b;
			a = r = g = b = 0;
			for (int y = -1; y < 2; y++)
				for (int x = -1; x < 2; x++)
				{
					pixlIn = *((pixel32bpp*)(&(im->row_pointers[i + y][(j + x) * 4])));
					a += ((float)pixlIn.a)*mask[(y + 1) * 3 + x + 1];
					r += ((float)pixlIn.r)*mask[(y + 1) * 3 + x + 1];
					g += ((float)pixlIn.g)*mask[(y + 1) * 3 + x + 1];
					b += ((float)pixlIn.b)*mask[(y + 1) * 3 + x + 1];
				}
			imOut->row_pointers[i][j * 4] = (char)r;
			imOut->row_pointers[i][j * 4 + 1] = (char)g;
			imOut->row_pointers[i][j * 4 + 2] = (char)b;
			imOut->row_pointers[i][j * 4 + 3] = (char)a;
		}
	return imOut;
}