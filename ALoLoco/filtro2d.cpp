#include "filtro2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <thread>
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

Image_t* filter2D_Paralell_1(Image_t* im, float* mask, int numMaxThreads)
{
	int k = 0;
	//Crear imagen resultado
	Image_t* imOut = new Image_t;
	(*imOut) = (*im);
	imOut->row_pointers = (char**)malloc(sizeof(char*)*im->height);
	for (int i = 0; i < im->height; i++)
		imOut->row_pointers[i] = (char*)malloc(sizeof(char)*im->width * 4);
	
	//Declarar threads
	thread ** th1;
	th1 = (thread**)malloc(sizeof(thread*)*numMaxThreads);
	
	//Por cada numMaxThreads
	for (int i = 1; i < (im->height - 1); i++)
		for (int j = 1; j < (im->width - 1); j++)
		{
			//Crear numMaxThreads hilos
			th1[k] = new thread(filter2D_Thread1, im, imOut, mask, i, j);
			k++;

			//Si se han ocupado los numMaxThreads Hilos
			if (k == numMaxThreads)
			{
				//Esperar hilos
				for (k = 0; k < numMaxThreads; k++)
				{
					th1[k]->join();
				}
				k = 0;
			}

		}
	//Esperar la ultima ronda de threads que no se ha completado
	for (int l = 0; l < k; l++)
	{
		th1[l]->join();
		delete th1[l];
	}
	//Liberar memoria
	free(th1);

	return imOut;
}

void filter2D_Thread1(Image_t* im, Image_t* imOut, float* mask, int i, int j)
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