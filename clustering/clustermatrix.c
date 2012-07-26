//      clustermatrix.c

#include <stdio.h>
#include "cluster.h"

#define BUFLEN 300
#define N_SCANS 500
#define N_MZPOINTS 100000

Point* pointmatrix(int dim1, int dim2) {
	Point *matrix;
	
	matrix = malloc(dim1*dim2, sizeof(Point));
	if (matrix == NULL)
		infox ("Couldn't malloc pointmatrix.", -1);
	
	return matrix;
}

int main(int argc, char** argv)
{
	FILE *infile;
	char line [BUFLEN] = {'\0'};
	Point* points;
	
	points = pointmatrix(N_SCANS, N_MZPOINTS);
	
	
	free(points);
	return 0;
}
