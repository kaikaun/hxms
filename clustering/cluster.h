//      cluster.h

typedef struct {
	double mz, I;
	int *cluster_flag; // Pointer to cluster point belongs to (NULL if none)
} Point;
