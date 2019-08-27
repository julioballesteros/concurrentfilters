#include <stdio.h>
#include <png.h>
#include <iostream>
#include <chrono>
#include <stdlib.h>
#include "pngLoader.h"
#include "filtro2d.h"

using namespace std;

int main(int argc, char *argv[]) {
	//Obtenemos el numero de threads que vamos a utilizar
	int numThreads = 8;
	
	if (argc > 1) {
		numThreads = atoi(argv[1]);
	}
	
	clock_t timeTotal, timeLoad, timeFilter, timeWrite;
	clock_t timeIni;

	timeIni = clock();

	//Cargamos la imagen
	timeLoad = clock();
	Image_t* im = loadPNG("gente.png");
	timeLoad = clock() - timeLoad;

	//Filtramos con el metodo de paralelizacion
	timeFilter = clock();
	Image_t* im2 = filter2D_Paralell_2(im, mask, numThreads);
	timeFilter = clock() - timeFilter;
	
	//comparar imagenes
	Image_t* imResult = filter2D(im, mask);
	if (!comparePNG(imResult, im2)) {
		cout << "Error: Las imagenes no son iguales" << endl;
	}
	else {
		cout << "Correcto, las imagenes son iguales" << endl;
	}

	//Escribimos el resultado
	timeWrite = clock();
	writePNG(im2, "gente2.png");
	timeWrite = clock() - timeWrite;

	timeTotal = clock() - timeIni;

	cout << "Numero de threads utilizados: " << numThreads << endl;
	int lms = (double)timeLoad / CLOCKS_PER_SEC * 1000;
	cout << "tiempo de carga: " << lms << "\n";
	int fms = (double)timeFilter / CLOCKS_PER_SEC * 1000;
	cout << "tiempo de filtro: " << fms << "\n";
	int wms = (double)timeWrite / CLOCKS_PER_SEC * 1000;
	cout << "tiempo de escritura: " << wms << "\n";
	int ms = (double)timeTotal / CLOCKS_PER_SEC * 1000;
	cout << "tiempo total: " << lms + fms + wms << "\n";

	return 0;
}