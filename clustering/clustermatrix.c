//      clustermatrix.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cluster.h"

#define BUFLEN 300

#define MZ_DIST 0.05

// Construct Pointmatrix
Point** pointmatrix(int dim1, int dim2) {
	Point **matrix;
	
	matrix = malloc(dim1*sizeof(Point*));
	if (matrix == NULL)
		infox ("Couldn't init main matrix.", -1);
	matrix[0] = malloc(dim1*dim2*sizeof(Point));
	if (matrix[0] == NULL)
		infox ("Couldn't init value matrix.", -1);
	for(int a = 1; a < dim1; ++a) {
		matrix[a] = matrix[a-1] + dim2;
	}
	return matrix;
}

// Free Pointmatrix
void freePointmatrix(Point **matrix) {
	free(matrix[0]);
	free(matrix);
}

int main(int argc, char** argv)
{
	FILE *infile;
	char line [BUFLEN] = {'\0'};
	Point **points;
	int scan_idx = N_SCANS, mz_idx = 0;
	int flags[N_FLAG], current_flag = 0;
	
	int total_scans = 0; //DEBUG
	
	points = pointmatrix(N_SCANS, N_MZPOINTS);
	for(int a = 0; a<N_FLAG; ++a) flags[a]=a;
	
	infile = fopen(argv[1],"r");
	if (infile == NULL)
		infox("Cannot open input file", -2);
	double current_RT = -1;
	int RT_step = N_SCANS/3;
	while(fgets(line,BUFLEN,infile)!=NULL) {
		double RT, mz, I;
		
		int ret = sscanf (line, " %*d  %lf  %lf  %lf", &RT, &mz, &I);
		if (ret == 3) {
			// Assume that points in CSV are already sorted by RT
			if (RT != current_RT) {
				printf("Scan %d, RT %f, %d points\n",total_scans,current_RT,mz_idx); //DEBUG
				total_scans++; //DEBUG
				
				scan_idx++;
				if (scan_idx >= N_SCANS) {
					printf("Resetting\n"); //DEBUG
					
					// Reset to 0
					/* memset(points, 0, N_SCANS*N_MZPOINTS*sizeof(Point));
					scan_idx = 0; */
					
					// Reset by RT_step
					memmove(points[0], points[RT_step], 
						(N_SCANS-RT_step)*N_MZPOINTS*sizeof(Point));
					memset(points[N_SCANS-RT_step],0,
						RT_step*N_MZPOINTS*sizeof(Point));
					scan_idx = N_SCANS-RT_step;
				}
				mz_idx = 0;
				current_RT = RT;
			}
			if (mz_idx >= N_MZPOINTS)
				infox ("mz_idx out of bounds. Raise N_MZPOINTS.", -3);
			points[scan_idx][mz_idx].mz = mz;
			points[scan_idx][mz_idx].I = I;

			char two_neighbours = 0;
			for (int a = scan_idx-1; a >= scan_idx-N_PREV; --a) {
				if (a < 0) continue;
				
				for (int b = 0; b < N_MZPOINTS; ++b) {
					if (abs(points[a][b].mz - mz) < MZ_DIST) {
						if (points[a][b].cluster_flag == NULL)
							infox("Neighbour has no cluster!", -10);
						if (points[scan_idx][mz_idx].cluster_flag == NULL) {
							points[scan_idx][mz_idx].cluster_flag = 
								points[a][b].cluster_flag;
						} else {
							*(points[a][b].cluster_flag) = 
								*(points[scan_idx][mz_idx].cluster_flag);
							two_neighbours = 1;
							break;
						}
					}
				}
				if (two_neighbours) break;
			}
			if (points[scan_idx][mz_idx].cluster_flag == NULL) {
				points[scan_idx][mz_idx].cluster_flag = &flags[current_flag];
				current_flag++;
				if (current_flag >= N_FLAG)
					infox("Out of cluster flags. Raise N_FLAG.",-3);
			}
			mz_idx++;
		} else {
			// Blank or invalid line in file
		}
	}
	
	fclose(infile);
	freePointmatrix(points);
	return 0;
}
