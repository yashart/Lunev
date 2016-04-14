//#include "integral.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdint.h>
#include <time.h>
#include <math.h>
#include <mpi.h>


const double step = 1e-8;

struct Range
{
	double start;
	double end;
	double step;
};

Range Range_ctor(double start, double end, double step)
{
	Range range;
	range.start = start;
	range.end   = end;
	range.step  = step;
	return range;
}

double function_value(double argument)
{
	return sin(argument)/argument;
}

double calc_integral(Range* distance)
{
	double result = 0;
	for(double i = distance->start; i < distance->end; i += distance->step)
	{
		result += (function_value(i) + function_value(i + distance->step))\
					 / 2 * distance->step;
	}
	return result;
}

void create_threads(int childrenQuantity, double startIntegral,
					 double endIntegral, int procNumber)
{
	double start, end, result;
	Range distance;
	while(true)
	{
		MPI_Recv(&start, 1, MPI_DOUBLE, 0,
				 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&end, 1, MPI_DOUBLE, 0,
				 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if(start > end)
			break;
		distance = Range_ctor(start, end, step);
		result = calc_integral(&distance);
		MPI_Send(&result, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
	}
}


int main(int argc, char *argv[])
{
	if(argc != 3)
	{
		fprintf(stderr, "Usage: %s start, end \n", argv[0]);
		exit(EXIT_FAILURE);
	}

	double starttime, endtime;
	starttime = MPI_Wtime();

	char* endptr;
	double start = strtod(argv[1], &endptr);
	double end = strtod(argv[2], &endptr);

	MPI_Init(&argc, &argv);
	int childrenQuantity, procNumber;
	MPI_Comm_rank(MPI_COMM_WORLD, &procNumber);
	MPI_Comm_size(MPI_COMM_WORLD, &childrenQuantity);
	double globalResult = 0;
	if(childrenQuantity == 1)
	{
		Range distance = Range_ctor(start, end, step);
		globalResult = calc_integral(&distance);
		MPI_Finalize();
		endtime = MPI_Wtime();
		printf("result = %lg, time = %lg\n", globalResult, endtime-starttime);
		return 0;
	}

	if(procNumber == 0)
	{
		double localResult = 0;
		//Send distance to child
		int currentPoint = 0;
		int distanceQuantity = 30;
		double part = (end - start)/distanceQuantity;
		Range distance;
		for(int i = 1; i < childrenQuantity; i++)
		{
			distance = Range_ctor(start + currentPoint * part, 
						start + (currentPoint + 1) * part, step);
			MPI_Send(&distance.start, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
			MPI_Send(&distance.end, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
			currentPoint++;
		}
		//Get answer and send more!
		MPI_Status status;
		while(currentPoint < distanceQuantity)
		{
			MPI_Recv(&localResult, 1, MPI_DOUBLE, MPI_ANY_SOURCE,
					 0, MPI_COMM_WORLD, &status);
			globalResult += localResult;
			distance = Range_ctor(start + currentPoint * part, 
						start + (currentPoint + 1) * part, step);
			MPI_Send(&distance.start, 1, MPI_DOUBLE, status.MPI_SOURCE,
					 0, MPI_COMM_WORLD);
			MPI_Send(&distance.end, 1, MPI_DOUBLE, status.MPI_SOURCE,
					 0, MPI_COMM_WORLD);
			currentPoint++;
		}
		//end all threads:
		double closeThread[2];
		closeThread[0] = 0;
		closeThread[1] = -1;
		for(int i = 1; i < childrenQuantity; i++)
		{
			MPI_Send(&closeThread[0], 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
			MPI_Send(&closeThread[1], 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
		}
		endtime = MPI_Wtime();
		printf("result = %lg, time = %lg", globalResult, endtime-starttime);
		MPI_Finalize();
		return 0;
	}
	create_threads(childrenQuantity, start, end, procNumber);
	MPI_Finalize();

	//double result = calc_integral(childrenQuantity, start, end);
	//printf("result = %lg", result);

	return 0;
}