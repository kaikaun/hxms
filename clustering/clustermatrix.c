//      clustermatrix.c

#include <stdio.h>
#include "cluster.h"

#define BUFLEN 300

// Allocate pointmatrix
Point* pointmatrix(int dim1, int dim2) {
	Point *matrix;
	
	matrix = malloc(dim1*dim2, sizeof(Point));
	if (matrix == NULL)
		infox ("Couldn't init pointmatrix.", -1);
	
	return matrix;
}

int main(int argc, char** argv)
{
	FILE *infile;
	char line [BUFLEN] = {'\0'};
	Point *points;
	unsigned int scan_index = 0;
	unsigned int mz_index = 0;
	
	points = pointmatrix(N_SCANS, N_MZPOINTS);
	
	infile = fopen(argv[1],"r");
	while(fgets(line,BUFLEN,infile)!=NULL) {
		double curr_RT = 0;
		double RT, mz, I;
		int ret = sscanf (line, " %*d  %lf  %lf  %lf", &RT, &mz, &I);
		if (ret == 3) {
			// Assume that points in CSV are already sorted by RT
			
		}
	}
	fclose(infile);
	
	free(points);
	
	return 0;
}
