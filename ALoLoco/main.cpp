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
	int numThreads = 10;

	if (argc > 1) {
		numThreads = atoi(argv[1]);
	}

	clock_t timeTotal, timeLoad, timeFilter, timeWrite;
	clock_t timeIni;

	timeIni = clock();

	//Cargamos la imagen
	timeLoad = clock();
	Image_t* im = loadPNG("simpson.png");
	timeLoad = clock() - timeLoad;

	//Filtramos con el metodo de paralelizacion
	timeFilter = clock();
	Image_t* im2 = filter2D_Paralell_1(im, mask, numThreads);
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
	writePNG(im2, "simpson2.png");
	timeWrite = clock() - timeWrite;

	timeTotal = clock() - timeIni;

	cout << "Numero de threads utilizados: " << numThreads << endl;
	int ms = (double)timeTotal / CLOCKS_PER_SEC * 1000;
	cout << "tiempo total: " << ms << "\n";
	ms = (double)timeLoad / CLOCKS_PER_SEC * 1000;
	cout << "tiempo de carga: " << ms << "\n";
	ms = (double)timeFilter / CLOCKS_PER_SEC * 1000;
	cout << "tiempo de filtro: " << ms << "\n";
	ms = (double)timeWrite / CLOCKS_PER_SEC * 1000;
	cout << "tiempo de escritura: " << ms << "\n";

	return 0;
}