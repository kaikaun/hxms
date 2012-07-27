// cluster.h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N_SCANS 500
#define N_MZPOINTS 20000
#define N_FLAG 100000
#define N_PREV 3

typedef struct {
	double mz, I;
	int *cluster_flag; // Pointer to cluster point belongs to (NULL if none)
} Point;

// Print error message and abort program with error exit value
int infox (const char *errmsg, int exitval) {
	fprintf(stderr, "%s\nExiting at %s:%d.\n", errmsg, __FILE__, __LINE__);
	exit(exitval);
}

// Construct Pointmatrix, returning pointer to matrix or NULL for error
Point** Pointmatrix(int dim1, int dim2) {
	Point **matrix;
	int a;

	if (dim1<=0) return NULL;
	if (dim2<=0) return NULL;
	matrix = malloc(dim1*sizeof(Point*));
	if (!matrix) return NULL;
	matrix[0] = calloc(dim1*dim2,sizeof(Point));
	if (!matrix[0]) return NULL;
	for(a = 1; a < dim1; ++a) matrix[a] = matrix[a-1] + dim2;

	return matrix;
}

// Free Pointmatrix
void freePointmatrix(Point **matrix) {
	free(matrix[0]);
	free(matrix);
}

// Move pointmatrix up by step, returning the new tail or -1 for error
int stepPointmatrix(Point **matrix, int dim1, int dim2, int step) {
	if (step<=0) return -1;
	if (step>dim1) return -1;
	memmove(matrix[0], matrix[step], (dim1-step)*dim2*sizeof(Point));
	memset(matrix[dim1-step],0, step*dim2*sizeof(Point));
	return dim1-step;
}
