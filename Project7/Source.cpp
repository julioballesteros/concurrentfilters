#include <stdio.h>
#include <png.h>
#include <iostream>
#include <chrono>
#include <stdlib.h>
#include "pngLoader.h"
#include "filtro2d.h"

int main(int argc, char *argv[]) {
	clock_t timeTotal, timeLoad, timeFilter, timeWrite;
	clock_t timeIni;

	timeIni = clock();

	timeLoad = clock();
	Image_t* im = loadPNG("simpson.png");
	timeLoad = clock() - timeLoad;

	timeFilter = clock();
	Image_t* im2 = filter2D(im, mask);
	timeFilter = clock() - timeFilter;

	timeWrite = clock();
	writePNG(im2, "simpson2.png");
	timeWrite = clock() - timeWrite;

	timeTotal = clock() - timeIni;

	int ms = (double)timeTotal / CLOCKS_PER_SEC * 1000;
	std::cout << "tiempo total: " << ms << "\n";
	ms = (double)timeLoad / CLOCKS_PER_SEC * 1000;
	std::cout << "tiempo de carga: " << ms << "\n";
	ms = (double)timeFilter / CLOCKS_PER_SEC * 1000;
	std::cout << "tiempo de filtro: " << ms << "\n";
	ms = (double)timeWrite / CLOCKS_PER_SEC * 1000;
	std::cout << "tiempo de escritura: " << ms << "\n";
	return 0;
}