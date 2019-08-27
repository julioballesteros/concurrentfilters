#include "filtro2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <iostream>
using namespace std;

float mask[] = {
	0.1111f,0.1111f,0.1111f,
	0.1111f,0.1111f,0.1111f,
	0.1111f,0.1111f,0.1111f
};

Image_t* filter2D(Image_t* im, float* mask)
{
	Image_t* imOut = new Image_t;
	(*imOut) = (*im);
	imOut->row_pointers = (char**)malloc(sizeof(char*)*im->height);
	for (int i = 0; i < im->height; i++)
		imOut->row_pointers[i] = (char*)malloc(sizeof(char)*im->width * 4);


	for (int i = 1; i < (im->height - 1); i++)
		for (int j = 1; j < (im->width - 1); j++)
		{
			pixel32bpp pixlIn, pixelOut;
			float a, r, g, b;
			a = r = g = b = 0;
			pixelOut.a = pixelOut.r = pixelOut.g = pixelOut.b = 0;
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

Image_t* filter2D_Paralell_2(Image_t* im, float* mask, int numThreads)
{
	//Crear imagen resultado
	Image_t* imOut = new Image_t;
	(*imOut) = (*im);
	imOut->row_pointers = (char**)malloc(sizeof(char*)*im->height);
	for (int i = 0; i < im->height; i++)
		imOut->row_pointers[i] = (char*)malloc(sizeof(char)*im->width * 4);

	//Declarar threads
	thread** th1;
	th1 = (thread**)malloc(sizeof(thread*)*numThreads);

	//Crear numThreads paquetes de trabajo
	work_package_t* packages = createWorkPackages(im->height, numThreads);
	/*
	cout << "Paquetes de trabajo:" << endl;
	for (int i = 0; i < numThreads; i++)
	{
		cout << "Thread " << i << ": \t" << packages[i].i_start << " - " << packages[i].i_final << endl;
	}
	*/
	//Crear numThreads hilos
	for (int i = 0; i < numThreads; i++) {
		th1[i] = new thread(filter2D_Thread2, im, imOut, mask, packages[i]);
	}

	//Esperar numThreads hilos
	for (int i = 0; i < numThreads; i++) {
		th1[i]->join();
		delete(th1[i]);
	}

	//Liberar memoria
	free(th1);
	free(packages);

	return imOut;
}

void filter2D_Thread2(Image_t* im, Image_t* imOut, float* mask, work_package_t work_package)
{
	pixel32bpp pixlIn, pixelOut;
	float a, r, g, b;

	for (int i = work_package.i_start; i < work_package.i_final; i++)
		for (int j = 1; j < (im->width - 1); j++)
		{
			a = r = g = b = 0;
			pixelOut.a = pixelOut.r = pixelOut.g = pixelOut.b = 0;
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
}

work_package_t* createWorkPackages(int rows, int numThreads) {
	//Declarar paquetes
	work_package_t* packages = (work_package_t*)malloc(sizeof(work_package_t)*numThreads);

	//Dividir filas entre paquetes
	int rows_per_package = (rows - 2) / numThreads;
	int rest_rows = (rows - 2) % numThreads;

	int row = 1;
	for (int i = 0; i < numThreads; i++) {
		packages[i].i_start = row;
		row += rows_per_package;
		if (i < rest_rows)
		{
			row++;
		}
		packages[i].i_final = row;
	}
	packages[numThreads - 1].i_final = rows - 1;

	return packages;
}