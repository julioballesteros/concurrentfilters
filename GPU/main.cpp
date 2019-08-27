#include <stdio.h>
#include <png.h>
#include <iostream>
#include <chrono>
#include <string>
#include <stdlib.h>
#include "pngLoader.h"
#include "kernel.h"

using namespace std;
using namespace std::chrono;

int main(int argc, char *argv[]) {

	float mask[] = {
	0.1111f,0.1111f,0.1111f,
	0.1111f,0.1111f,0.1111f,
	0.1111f,0.1111f,0.1111f
	};

	//Obtenemos el numero de threads que vamos a utilizar
	int numThreadsPerBlock = 16;
	char *path = "sunHD4k.png";

	if (argc > 2) {
		numThreadsPerBlock = atoi(argv[1]);
		path = argv[2];
	}

	cout << "Numero de threads por bloque y dimension a utilizar: " << numThreadsPerBlock << endl;
	high_resolution_clock::time_point t1 = high_resolution_clock::now();
	{
		//Cargamos la imagen
		high_resolution_clock::time_point t1 = high_resolution_clock::now();
		Image_t* im = loadPNG(path);
		duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
		cout << "Tiempo de carga de la imagen PNG: " << time_span.count() * 1000 << "\n";

		//Filtramos con el metodo de paralelizacion
		cout << "Tiempos de ejecucion del metodo de filtrado:" << endl;
		t1 = high_resolution_clock::now();
		Image_t* imResult = filter2D_CUDA_SM(im, mask, numThreadsPerBlock);
		time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
		cout << "\t Tiempo total: " << time_span.count() * 1000 << "\n";

		//Comparamos imagenes
		Image_t* im2 = filter2D(im, mask);
		if (!comparePNG(im2, imResult)) {
			cout << "Error: Las imagenes no son iguales" << endl;
		}
		else {
			cout << "Correcto, las imagenes son iguales" << endl;
		}

		//Escribimos el resultado
		t1 = high_resolution_clock::now();
		writePNG(im2, "result.png");
		time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
		cout << "Tiempo de escritura de la imagen PNG: " << time_span.count() * 1000 << "\n";
	}
	duration<double> time_span = duration_cast<duration<double>>(high_resolution_clock::now() - t1);
	cout << "Tiempo total del programa: " << time_span.count() * 1000 << "\n";

	return 0;
}


