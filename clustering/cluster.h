// cluster.h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N_SCANS 500
#define N_MZPOINTS 20000
#define N_FLAG 100000
#define N_PREV 3

typedef struct {
	int color, last_seen;
} Flag;

typedef struct {
	double mz, I;
	Flag *cluster_flag; // Pointer to cluster point belongs to (NULL if none)
} Point;

// Print error message and abort program with error exit value
int infox (const char *errmsg, int exitval, const char *file, int line) {
	fprintf(stderr, "%s\nExiting at %s:%d.\n", errmsg, file, line);
	exit(exitval);
}

// Return index to next available flag in an array of flags or <0 for error
int getnextFlag(Flag *flags, int len, int current) {
	int next;

	// Invalid arguments
	if (len<=0) return -1;
	if (current<0) return -1;
	if (current>=len) return -1;

	next = current;
	if (++next >= len) next -= len;
	while (next != current) {
		if (flags[next].last_seen == -1) return next;
		if (++next >= len) next -= len;
	}
	return -2; // No available flag found
}

// Clear old flags with last_seen < scan from flags[]
// Send old colors and their latest last_seen to output() (0 return for success)
// Return number of cleared flags or <0 for error
int freshenFlags(Flag *flags, int len, int scan, int (*output)(int c, int l)) {
	Flag new[len];
	int curr_color = -1, tail = 0, cleared = 0;
	int a,b;

	// Invalid arguments
	if (len <= 0) return -1;
	if (scan < 0) return -1;

	// Populate new[] with the latest last_seen for each color and store highest
	// color number in curr_color
	for (a=0; a<len; ++a) {
		if (flags[a].last_seen == -1) continue;
		for (b=0; b<tail; ++b) {
			if (flags[a].color == new[b].color) {
				if (new[b].last_seen < flags[a].last_seen)
					new[b].last_seen = flags[a].last_seen;
				break;
			}
		}
		if (b == tail) {
			new[tail++] = flags[a];
			if (curr_color < flags[a].color) curr_color = flags[a].color;
		}
	}

	// Go through new[] and display old colors
	for (b=0; b<tail; ++b)
		if (new[b].last_seen < scan) {
			int ret = output(new[b].color, new[b].last_seen);
			if (ret) return -2; // Output of point failed
		}

	// Mark old flags as available and give them unique new colors
	for (a=0; a<len; ++a) {
		if (flags[a].last_seen == -1) continue;
		for (b=0; b<tail; ++b) {
			if (flags[a].color == new[b].color) {
				if (new[b].last_seen < scan) {
					flags[a].color = ++curr_color;
					flags[a].last_seen = -1;
					++cleared;
				}
				break;
			}
		}
	}

	return cleared;
}

// Construct Pointmatrix, returning pointer to matrix or NULL for error
Point** Pointmatrix(int dim1, int dim2) {
	Point **matrix;
	int a;

	// Invalid arguments
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
	// Invalid arguments
	if (step<=0) return -1;
	if (step>dim1) return -1;

	memmove(matrix[0], matrix[step], (dim1-step)*dim2*sizeof(Point));
	memset(matrix[dim1-step],0, step*dim2*sizeof(Point));

	return dim1-step;
}


