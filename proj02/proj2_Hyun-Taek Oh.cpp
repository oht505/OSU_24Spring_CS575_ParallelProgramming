#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// print debugging messages?
#ifndef DEBUG
#define DEBUG	false
#endif

// setting the number of threads:
#ifndef NUMT
#define NUMT		    4
#endif

#define CSV

int	NowYear;		// 2024- 2029
int	NowMonth;		// 0 - 11
int Month;     // 0 - 71

float	NowPrecip;		// inches of rain per month
float	NowTemp;		// temperature this month
float	NowHeight;		// grain height in inches
int	NowNumDeer;		// number of deer in the current population
int NowNumZombie;   // number of zombie in the current population

const float GRAIN_GROWS_PER_MONTH =	       12.0;
const float ONE_DEER_EATS_PER_MONTH =		1.0;
const float ONE_ZOMBIE_EATS_PER_MONTH =     1.0;
//const float ONE_ZOMBIE_PLANTS_PER_MONTH =   2.0;        // Zombie can be fertilizer(?)

const float AVG_PRECIP_PER_MONTH =		7.0;	// average
const float AMP_PRECIP_PER_MONTH =		6.0;	// plus or minus
const float RANDOM_PRECIP =			2.0;	// plus or minus noise

const float AVG_TEMP =				60.0;	// average
const float AMP_TEMP =				20.0;	// plus or minus
const float RANDOM_TEMP =			10.0;	// plus or minus noise

const float MIDTEMP =				40.0;
const float MIDPRECIP =				10.0;

// Units of grain growth are inches.
// Units of temperature are degrees Fahrenheit (°F).
// Units of precipitation are inches.

omp_lock_t	Lock;
volatile int	NumInThreadTeam;
volatile int	NumAtBarrier;
volatile int	NumGone;

void	InitBarrier( int );
void	WaitBarrier( );
float   Ranf( float, float );
void    Deer();
void    GrainGrowth();
void    Watcher();
void    Zombie();
float   SQR( float );

int 
main(int argc, char *argv[ ]){
#ifdef _OPENMP
	#ifndef CSV
		fprintf( stderr, "OpenMP is supported -- version = %d\n", _OPENMP );
	#endif
#else
        fprintf( stderr, "No OpenMP support!\n" );
        return 1;
#endif
        omp_set_num_threads( NUMT );
        InitBarrier( NUMT );    

        unsigned int seed = 0;

        // starting date and time:
        NowMonth =    0;
        Month =    0;
        NowYear  = 2024;

        // starting state (feel free to change this if you want):
        NowNumDeer = 4;
        NowHeight = 10.;
        NowNumZombie = 0;

        #pragma omp parallel sections
        {
            #pragma omp section
            {
                Deer();
            }

            #pragma omp section
            {
                GrainGrowth();
            }

            #pragma omp section
            {
                Watcher();
            }

            #pragma omp section
            {
                Zombie();
            }

        }   // ipmlied barrier -- all functions must return to get past here
            // to allow any of them to get past here

    // return 0;
}


// specify how many threads will be in the barrier:
//	(also init's the Lock)
void
InitBarrier( int n )
{
    NumInThreadTeam = n;
    NumAtBarrier = 0;
	omp_init_lock( &Lock );
}

void
WaitBarrier( )
{
        omp_set_lock( &Lock );
        {
                NumAtBarrier++;
                if( NumAtBarrier == NumInThreadTeam )
                {
                        NumGone = 0;
                        NumAtBarrier = 0;
                        // let all other threads get back to what they were doing
			            // before this one unlocks, knowing that they might immediately
			            // call WaitBarrier( ) again:
                        while( NumGone != NumInThreadTeam-1 );
                        omp_unset_lock( &Lock );
                        return;
                }
        }
        omp_unset_lock( &Lock );

        while( NumAtBarrier != 0 );	// this waits for the nth thread to arrive

        #pragma omp atomic
            NumGone++;			// this flags how many threads have returned
}

float
Ranf( float low, float high )
{
        float r = (float) rand();               // 0 - RAND_MAX
        float t = r  /  (float) RAND_MAX;       // 0. - 1.

        return   low  +  t * ( high - low );
}

void
Deer()
{
    while ( NowYear < 2030 )
    {
        // int nextXXX = << function of what all states are right now >>
        int nextNumDeer = NowNumDeer;
        int carryingCapacity = (int)( NowHeight );
        int EatenByZombie = (int)( NowNumZombie);

        if ( nextNumDeer < carryingCapacity )
            nextNumDeer++;
        else
            if ( nextNumDeer > carryingCapacity )
                nextNumDeer--;

        nextNumDeer -= (float) NowNumZombie * ONE_ZOMBIE_EATS_PER_MONTH;

        if ( nextNumDeer < 0 )
            nextNumDeer = 0;
        
        WaitBarrier();

        // NowXXX = nextXXX;    // Copy the computed next state to Now state
        NowNumDeer = nextNumDeer;

        WaitBarrier();


        // Do nothing
        WaitBarrier();
    }
}

void    
GrainGrowth()
{
    while ( NowYear < 2030 )
    {
        float ang = (  30.*(float)NowMonth + 15.  ) * ( M_PI / 180. );	// angle of earth around the sun

        float temp = AVG_TEMP - AMP_TEMP * cos( ang );
        //temp = (temp-32.) * 5/9;    // Fahrenheit to Celsius
        NowTemp = temp + Ranf( -RANDOM_TEMP, RANDOM_TEMP );

        float precip = AVG_PRECIP_PER_MONTH + AMP_PRECIP_PER_MONTH * sin( ang );
        //precip *= 2.54;     // Inches to centimeter
        NowPrecip = precip + Ranf( -RANDOM_PRECIP, RANDOM_PRECIP );
        if( NowPrecip < 0. )
	        NowPrecip = 0.;

        float tempFactor = exp( -SQR( ( NowTemp - MIDTEMP ) / 10. ) );
        float precipFactor = exp( -SQR( ( NowPrecip - MIDPRECIP ) / 10. ) );

        float nextHeight = NowHeight;
        nextHeight += tempFactor * precipFactor * GRAIN_GROWS_PER_MONTH;
        nextHeight -= (float) NowNumDeer * ONE_DEER_EATS_PER_MONTH;
        
        if ( nextHeight < 0. )  
            nextHeight = 0.;

        WaitBarrier();

        // NowXXX = nextXXX;    // Copy the computed next state to Now state
        NowHeight = nextHeight;

        WaitBarrier();


        // Do nothing
        WaitBarrier();
    }
}

void   
Watcher()
{
    while ( NowYear < 2030 )
    {
        // Do nothing
        WaitBarrier();

        // Do nothing
        WaitBarrier();

        // Write out the "Now" state of data
        // Advance time and re-compute all environmental variable

#ifdef CSV
        fprintf(stderr, "%d , %6.2lf , %6.2lf , %6.2lf , %2d, %2d\n", 
                 Month, (NowTemp-32.) * 5/9, NowPrecip*2.54, NowHeight*2.54, NowNumDeer, NowNumZombie);
#endif
        NowMonth += 1;
        Month += 1;
        if ( NowMonth > 11 )
        {
            NowMonth = 0;
            NowYear += 1;
        }

        WaitBarrier();

    }
}

void    
Zombie()
{
    while ( NowYear < 2030 )
    {
        // int nextXXX = << function of what all states are right now >>
        int nextNumZombie = NowNumZombie;
        int carryingCapacityZ = (int) NowNumDeer;
        
        if( (nextNumZombie < carryingCapacityZ/2) & carryingCapacityZ > 4){
            nextNumZombie++;
        }
        else{
            if (carryingCapacityZ < nextNumZombie)
                //nextNumZombie /= 2;
                nextNumZombie--;
        }

        if ( nextNumZombie < 0 )
            nextNumZombie = 0;

        WaitBarrier();

        // NowXXX = nextXXX;    // Copy the computed next state to Now state
        NowNumZombie = nextNumZombie;
        WaitBarrier();


        // Do nothing
        WaitBarrier();
    }
}

float
SQR( float x )
{
    return x * x;
}