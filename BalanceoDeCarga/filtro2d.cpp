#include "filtro2d.h"
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
using namespace std;

mutex m1;
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

Image_t* filter2D_Paralell_3(Image_t* im, float* mask, int numThreads, int packageSize)
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

	//Crear lista de paquetes de trabajo
	list<work_package_t*>* packagesList = createPackagesList(im->height, packageSize);

	//Crear numThreads hilos
	for (int i = 0; i < numThreads; i++) {
		th1[i] = new thread(filter2D_Thread3, im, imOut, mask, packagesList);
	}

	//Esperar numThreads hilos
	for (int i = 0; i < numThreads; i++) {
		th1[i]->join();
		delete(th1[i]);
	}

	//Liberar memoria
	free(th1);
	delete packagesList;

	return imOut;
}

void filter2D_Thread3(Image_t* im, Image_t* imOut, float* mask, list<work_package_t*>* packagesList)
{
	work_package_t* work_package;
	pixel32bpp pixlIn, pixelOut;
	float a, r, g, b;
	bool empty = false;
	while (true)
	{
		//Acceder al siguiente paquete de la lista
		//Realizar esta operacion con cerrojo
		m1.lock();
		if (packagesList->empty()) {
			m1.unlock();
			break;
		}
		work_package = (work_package_t*)packagesList->front();
		packagesList->pop_front();
		m1.unlock();

		for (int i = work_package->i_start; i < work_package->i_final; i++)
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
		delete work_package;
	}
}

list<work_package_t*>* createPackagesList(int rows, int package_size) {

	list<work_package_t*>* packages_list = NULL;;
	packages_list = new list<work_package_t*>;

	work_package_t* package = NULL;

	//Calcular numero de paquetes
	int packages_count = (rows - 2) / package_size;

	int row = 1;
	for (int i = 0; i < packages_count; i++) {
		package = new work_package_t;
		package->i_start = row;
		row += package_size;
		package->i_final = row;
		if (i == packages_count - 1) package->i_final = rows - 1;
		packages_list->push_back(package);
	}

	return packages_list;
}