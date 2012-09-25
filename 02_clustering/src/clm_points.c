// clm_points.c

#include <stdlib.h>
#include <string.h>
#include "clm.h"

// Construct Pointmatrix, returning pointer to matrix or NULL for error
Point** Pointmatrix(int dim1, int dim2) {
	Point **matrix;
	int a;

	// Invalid arguments
	if (dim1<=0) return NULL;
	if (dim2<=0) return NULL;

	matrix = malloc(dim1*sizeof(Point*));
	if (!matrix) return NULL;
	matrix[0] = calloc(dim1*dim2,sizeof(Point)); // calloc zeroes main matrix
	if (!matrix[0]) return NULL;
	for(a = 1; a < dim1; ++a) matrix[a] = matrix[a-1] + dim2;

	return matrix;
}

// Free Pointmatrix
void freePointmatrix(Point **matrix) {
	free(matrix[0]);
	free(matrix);
}

// Move up a memory block (base, size) by step, and zero the end of the block
// Returns a pointer to the start of the zeroed area or NULL for error
void* memstep(void *base, size_t size, size_t step) {
	 // Invalid argument
	if (step < 0) return NULL;
	if (step > size) return NULL;

	memmove(base, base+step, size-step); // Move up the memory block
	memset(base+size-step, 0, step); // Zero the end of the memory block
	return base+size-step;
}

// Move pointmatrix up by step rows, returning the new tail or -1 for error
int stepPointmatrix(Point **matrix, int dim1, int dim2, int step) {
	// Invalid arguments
	if (step<=0) return -1;
	if (step>dim1) return -1;

	if(!memstep(matrix[0],dim1*dim2*sizeof(Point),step*dim2*sizeof(Point)))
		return -1;
	return dim1-step;
	// memmove(matrix[0], matrix[step], (dim1-step)*dim2*sizeof(Point));
	// memset(matrix[dim1-step], 0, step*dim2*sizeof(Point)); 
}

// Move up an array of doubles by step, returning the new tail or -1 for error
int stepDoubles(double *array, int len, int step) {
	// Invalid arguments
	if (step<=0) return -1;
	if (step>len) return -1;

	if(!memstep(array,len*sizeof(double),step*sizeof(double)))
		return -1;
	return len-step;
}
