#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <string>

// setting the number of threads:
#ifndef NUMT
#define NUMT		    1
#endif

// setting the number of capitals we want to try:
#ifndef NUMCAPITALS
#define NUMCAPITALS		5
#endif

// maximum iterations to allow looking for convergence:
#define MAXITERATIONS	100

// how many tries to discover the maximum performance:
#define NUMTIMES	20

#define CSV

struct city
{
	std::string	name;
	float		longitude;
	float		latitude;
	int		capitalnumber;
	float		mindistance;
};

#include "UsCities.data"

// setting the number of cities we want to try:
#define NUMCITIES 	( sizeof(Cities) / sizeof(struct city) )

struct capital
{
	std::string	name;
	float		longitude;
	float		latitude;
	float		longsum;
	float		latsum;
	int		numsum;
};

struct capital	Capitals[NUMCAPITALS];


float
Distance( int city, int capital )
{
	float dx = Cities[city].longitude - Capitals[capital].longitude;
	float dy = Cities[city].latitude  - Capitals[capital].latitude;
	return sqrtf( dx*dx + dy*dy );
}

float
Ranf( float low, float high )
{
        float r = (float) rand();               // 0 - RAND_MAX
        float t = r  /  (float) RAND_MAX;       // 0. - 1.

        return   low  +  t * ( high - low );
}

// call this if you want to force your program to use
// a different random number sequence every time you run it:
void
TimeOfDaySeed( )
{
	struct tm y2k = { 0 };
	y2k.tm_hour = 0;   y2k.tm_min = 0; y2k.tm_sec = 0;
	y2k.tm_year = 100; y2k.tm_mon = 0; y2k.tm_mday = 1;

	time_t  timer;
	time( &timer );
	double seconds = difftime( timer, mktime(&y2k) );
	unsigned int seed = (unsigned int)( 1000.*seconds );    // milliseconds
	srand( seed );
}

int
main( int argc, char *argv[ ] )
{
#ifdef _OPENMP
		//fprintf( stderr, "OpenMP is supported -- version = %d\n", _OPENMP );
#else
        fprintf( stderr, "No OpenMP support!\n" );
        return 1;
#endif

	// make sure we have the data correctly:
	//for( int i = 0; i < NUMCITIES; i++ )
	//{
		//fprintf( stderr, "%3d  %8.2f  %8.2f  %s\n", i, Cities[i].longitude, Cities[i].latitude, Cities[i].name.c_str() );
	//}

    omp_set_num_threads( NUMT );    // set the number of threads to use in parallelizing the for-loop:`

	// seed the capitals:
	// (this is just picking initial capital cities at uniform intervals)
		
	TimeOfDaySeed();

	int checkDuplicate[NUMCAPITALS];

	for( int k = 0; k < NUMCAPITALS; k++ )
	{
		int cityIndex = k * (NUMCITIES-1) / (NUMCAPITALS-1);
		// int cityIndex = 0 + (int)((float)(NUMCITIES-1) * rand() / (RAND_MAX + 1.0));
		// int cityIndex = Ranf( 0., (float)(NUMCITIES-1) );
		int i = 0;

		// Avoid redundant cityIndex
		while ( i <= NUMCAPITALS)
		{
			if ( checkDuplicate[i] == cityIndex )
			{
				cityIndex = Ranf( 0., (float)(NUMCITIES-1) );
				i = 0;
				continue;
			}
			i++;
		}

		checkDuplicate[k] = cityIndex;
		Capitals[k].longitude = Cities[cityIndex].longitude;
		Capitals[k].latitude  = Cities[cityIndex].latitude;
		Cities[cityIndex].capitalnumber = k;
	}


	double time0, time1;
	for( int n = 0;  n < MAXITERATIONS; n++ )
	{
		// reset the summations for the capitals:
		for( int k = 0; k < NUMCAPITALS; k++ )
		{
			Capitals[k].longsum = 0.;
			Capitals[k].latsum  = 0.;
			Capitals[k].numsum = 0;
		}

		time0 = omp_get_wtime( );

        // the #pragma goes here -- you figure out what it needs to look like:
        #pragma omp parallel for default(none) shared(Cities, Capitals, checkDuplicate, stderr) 

		for( int i = 0; i < NUMCITIES; i++ )
		{
			int capitalnumber = -1;
			float mindistance = 1.e+37;

			for( int k = 0; k < NUMCAPITALS; k++ )
			{
				float dist = Distance( i, k );
				if( dist < mindistance )
				{
					capitalnumber = k;
					Cities[i].capitalnumber = capitalnumber;
					mindistance = dist;
					Cities[i].mindistance = mindistance;	// The minimum distance between Capital and city
				}
			}

			// // Protect original Capital numbers from misallocation 
			for ( int j = 0; j < NUMCAPITALS; j++ )
			{
				if (checkDuplicate[j] == i)
				{
					Cities[i].capitalnumber = j;
					Cities[i].mindistance = mindistance;
				}
			}

			int k = Cities[i].capitalnumber;
			
			// this is here for the same reason as the Trapezoid noteset uses it:
			#pragma omp critical
			{
				Capitals[k].longsum += Cities[i].longitude;
				Capitals[k].latsum  += Cities[i].latitude;
				Capitals[k].numsum++;
			}
		}

		time1 = omp_get_wtime( );

		// get the average longitude and latitude for each capital:
		for( int k = 0; k < NUMCAPITALS; k++ )
		{
			Capitals[k].longitude = Capitals[k].longsum / (float) (Capitals[k].numsum);
			Capitals[k].latitude  = Capitals[k].latsum / (float) (Capitals[k].numsum);
			//printf("%3d  Capital's longitude: %8.3lf, Capital's latitude: %8.3lf\n", k,  Capitals[k].longitude, Capitals[k].latitude);
		}
	}

	double megaCityCapitalsPerSecond = (double)NUMCITIES * (double)NUMCAPITALS / ( time1 - time0 ) / 1000000.;


	// figure out what actual city is closest to each capital:
	// this is the extra credit:
	for( int k = 0; k < NUMCAPITALS; k++ )
	{
		float closestDistance = 1.e+37;
		int cityIdx = 0;

		for ( int i = 0; i < NUMCITIES; i++ )
		{
			float currDist = Distance( i, k );
			if ( ( Cities[i].capitalnumber == k ) & ( currDist < closestDistance  ) )
			{
				closestDistance = currDist;
				cityIdx = i;
			}
		}
		Capitals[k].name = Cities[cityIdx].name;

	}


	// print the longitude-latitude of each new capital city:
	// you only need to do this once per some number of NUMCAPITALS -- do it for the 1-thread version:
	if( NUMT == 1 )
	{
		for( int k = 0; k < NUMCAPITALS; k++ )
		{
			//fprintf( stderr, "\t%3d:  %8.2f , %8.2f\n", k, Capitals[k].longitude, Capitals[k].latitude );

			// if you did the extra credit, use this fprintf instead:
			fprintf( stderr, "\t%3d:  %8.2f , %8.2f , %s\n", k, Capitals[k].longitude, Capitals[k].latitude, Capitals[k].name.c_str() );
		}
	}
#ifdef CSV
        fprintf(stderr, "%2d , %4d , %4d , %8.3lf:\n", NUMT, NUMCITIES, NUMCAPITALS, megaCityCapitalsPerSecond );
#else
        fprintf(stderr, "%2d threads : %4d cities ; %4d capitals; megatrials/sec = %8.3lf\n",
                NUMT, NUMCITIES, NUMCAPITALS, megaCityCapitalsPerSecond );
#endif

}