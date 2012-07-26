//      cluster.h

#define N_SCANS 500
#define N_MZPOINTS 100000

typedef struct {
	double mz, I;
	int *cluster_flag; // Pointer to cluster point belongs to (NULL if none)
} Point;

// Print error message and abort program with error exit value
int infox (const char *errmsg, int exitval) {
	fprintf(stderr, "%s\nExiting at %s:%d.\n", errmsg, __FILE__, __LINE__);
	exit(exitval);
}
