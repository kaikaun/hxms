//      cluster.h

#define N_SCANS 500
#define N_MZPOINTS 100000
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

/* Binary search on sorted array with unique keys, using same parameters as 
   bsearch plus mode: 2 = strictly greater than, 1 = greater than or equal
                     -2 = strictly less than,   -1 = less than or equal
                      0 = strictly equal (bsearch behaviour) */
void* bsearch2(const void* key, const void* base, size_t num, size_t size,
               int (*compare)(const void*, const void*), signed char mode ) {

	int min = 0, max = num-1;

	while (max >= min) {
		int mid = (min + max) >> 1;
		int com = compare(key, base + mid*size);
		
		if (com < 0) max = mid - 1;
		else if (com > 0) min = mid + 1;
		else {
			switch (mode) {
				case 2:
					if (mid >= num-1) return NULL;
					++mid;
					break;
				case -2:
					if (mid <= 0) return NULL;
					--mid;
					break;
			}
			return (void*)(base + mid*size);
		}
	}
	switch (mode) {
		case 2:
		case 1:
			if (min >= num) return NULL;
			return (void*)(base + min*size);
		case -2:
		case -1:
			if (max < 0) return NULL;
			return (void*)(base + max*size);
		case 0:
		default:
			return NULL;
	}
}
