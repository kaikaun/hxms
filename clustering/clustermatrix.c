//      clustermatrix.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cluster.h"

#define BUFLEN 300

// Allocate pointmatrix
Point* pointmatrix(int dim1, int dim2) {
	Point *matrix;
	
	matrix = malloc(dim1*dim2*sizeof(Point));
	if (matrix == NULL)
		infox ("Couldn't init pointmatrix.", -1);
	
	return matrix;
}

int main(int argc, char** argv)
{
	FILE *infile;
	char line [BUFLEN] = {'\0'};
	Point *points;
	int scan_index = N_SCANS, mz_index = 0;
	int flags[N_FLAG], current_flag = 0;
	
	int total_scans = 0; //DEBUG
	
	points = pointmatrix(N_SCANS, N_MZPOINTS);
	for(int a = 0; a<N_FLAG; ++a) flags[a]=a;
	
	infile = fopen(argv[1],"r");
	double curr_RT = -1;
	int RT_step = N_SCANS/3;
	while(fgets(line,BUFLEN,infile)!=NULL) {
		double RT, mz, I;
		
		int ret = sscanf (line, " %*d  %lf  %lf  %lf", &RT, &mz, &I);
		if (ret == 3) {
			// Assume that points in CSV are already sorted by RT
			if (RT != curr_RT) {
				printf("Scan %d, RT %f, %d points\n",total_scans,RT,mz_index); //DEBUG
				total_scans++; //DEBUG
				
				scan_index++;
				if (scan_index >= N_SCANS) {
					printf("Resetting\n"); //DEBUG
					
					// Reset to 0
					/* memset(points, 0, N_SCANS*N_MZPOINTS*sizeof(Point));
					scan_index = 0; */
					
					// Reset by RT_step
					memmove(points, &points[RT_step*N_MZPOINTS], 
						(N_SCANS-RT_step)*N_MZPOINTS*sizeof(Point));
					memset(&points[(N_SCANS-RT_step)*N_MZPOINTS],0,
						RT_step*N_MZPOINTS*sizeof(Point));
					scan_index = N_SCANS-RT_step;
				}
				mz_index = 0;
				curr_RT = RT;
			}
			if (mz_index >= N_MZPOINTS)
				infox ("mz_index out of bounds. Raise N_MZPOINTS.", -1);
			points[scan_index*N_MZPOINTS+mz_index].mz = mz;
			points[scan_index*N_MZPOINTS+mz_index].I = I;
			mz_index++;
		}
	}
	
	fclose(infile);
	free(points);
	return 0;
}
