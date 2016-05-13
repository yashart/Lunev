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
	double start, end, result[2];
	Range distance;
	while(true)
	{
		MPI_Recv(&start, 1, MPI_DOUBLE, 0,
				 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&end, 1, MPI_DOUBLE, 0,
				 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		if(start > end)
			break;
		result[1] = start;
		distance = Range_ctor(start, end, step);
		result[0] = calc_integral(&distance);
		MPI_Send(&result, 2, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
	}
}

void error_handler(MPI_Comm* comm, int* error, ...)
{
	printf("Error: %d", *error);
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

	int provided = 0;
	MPI_Init(&argc, &argv);
	MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);


	//MPI_Errhandler newErrorHandler;
	//MPI_Comm_create_errhandler(error_handler, &newErrorHandler);
	//MPI_Comm_set_errhandler(MPI_COMM_WORLD, newErrorHandler);
	
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
		//printf("result = %lg, time = %lg\n", globalResult, endtime-starttime);
		return 0;
	}

	if(procNumber == 0)
	{
		double localResult[2];
		int distanceQuantity = 30;
		Range distancesQuery[distanceQuantity];
		//Make distance Query
		double part = (end - start)/distanceQuantity;
		for(int i = 0; i < distanceQuantity; i++)
		{
			distancesQuery[i] = Range_ctor(start + i * part, 
						start + (i + 1) * part, step);
		}
		//Send distance to child
		int currentPoint = 0;
		Range distance;
		for(int i = 1; i < childrenQuantity; i++)
		{
			MPI_Send(&distancesQuery[currentPoint].start, 1,\
					 MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
			MPI_Send(&distancesQuery[currentPoint].end, 1, \
					 MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
			currentPoint++;
		}
		//Get answer and send more!
		MPI_Status status;
		int recieveMessagesNumbers = 0;
		while(recieveMessagesNumbers < distanceQuantity - 1)
		{
			try
			{
				MPI_Recv(&localResult, 2, MPI_DOUBLE, MPI_ANY_SOURCE,
						 0, MPI_COMM_WORLD, &status);
				recieveMessagesNumbers ++;
				globalResult += localResult[0];
				//delete recieve message from query
				for(int i = 0; i < distanceQuantity; i++)
				{
					if(distancesQuery[i].start == localResult[1])
					{
						distancesQuery[i].step = -1;
						break;
					}
				}
				while(true)
				{
					if(distancesQuery[currentPoint].step > 0)
					{
						MPI_Send(&distancesQuery[currentPoint].start, 1, MPI_DOUBLE, status.MPI_SOURCE,
							 	0, MPI_COMM_WORLD);
						MPI_Send(&distancesQuery[currentPoint].end, 1, MPI_DOUBLE, status.MPI_SOURCE,
								 0, MPI_COMM_WORLD);
						currentPoint = (currentPoint + 1) % distanceQuantity;
						break;
					}
					currentPoint = (currentPoint + 1) % distanceQuantity;
				}
			}
			catch(...)
			{
				printf("Hello");
			}
		}
		//end all threads:
		endtime = MPI_Wtime();
		printf("result = %lg, time = %lg", globalResult, endtime-starttime);

		double closeThread[2];
		closeThread[0] = 0;
		closeThread[1] = -1;
		for(int i = 1; i < childrenQuantity; i++)
		{
			MPI_Send(&closeThread[0], 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
			MPI_Send(&closeThread[1], 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
		}
		MPI_Finalize();
		return 0;
	}
	create_threads(childrenQuantity, start, end, procNumber);
	MPI_Finalize();

	//double result = calc_integral(childrenQuantity, start, end);
	//printf("result = %lg", result);

	return 0;
}