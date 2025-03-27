#include <omp.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#ifndef NUMT
#define NUMT	4
#endif

#ifndef SIZE
#define SIZE	123456
#endif

#define NUMTRIES	20

float A[SIZE];
float B[SIZE];
float C[SIZE];

int main(){
#ifdef	_OPENMP
	fprintf(stderr, "OpenMP version %d is supported here\n", _OPENMP);
#else
	fprintf(stderr, "OpenMP is not supported here - sorry!\n");
#endif

	// initialize the arrays:
	for(int i=0; i<SIZE; i++)
	{
		A[i] = 1.;
		B[i] = 2.;

	}

	omp_set_num_threads(NUMT);

	double maxMegaMults = 0;

	for(int t=0; t<NUMTRIES; t++)
	{
		double time0 = omp_get_wtime();

		#pragma omp parallel for
		for(int i=0; i<SIZE; i++)
		{
			C[i] = A[i] * B[i];
		}

		double time1 = omp_get_wtime();
		double megaMults = (double)SIZE/(time1-time0)/1000000;
		if(megaMults > maxMegaMults){
			maxMegaMults = megaMults;
		}
	}

	fprintf(stderr, "For %d threads, Peak Performance = %8.2lf MegaMults/Sec\n", NUMT, maxMegaMults);

	// note: %lf stands for "long float", which is how printf prints a "double"
	// 	 %d stands for "decimal integer", not "double"
	//
	// Speedup = (Peak performance for 4 threads) / (Peak performance for 1 thread)

	return 0;
}


