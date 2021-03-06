//      cluster.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFLEN 300    // Size of line buffer during file reading
#define RT_INIT 200   // Initial number of scans allocated in a spectrum
#define RT_INC 100    // Number of scans to expand by when a spectrum fills up
#define PT_INIT 2000  // Initial number of points allocated in a scan
#define PT_INC 1000   // Number of points to expand by when a scan fills up
#define NBBUFLEN 100  // Size of neighbour buffer during neighbour search
#define N_FLAG 10000

#define DEFAULT_RT_DIST 0.8
#define DEFAULT_MZ_DIST 0.05
#define DEFAULT_MIN_NB  3

// Command line arguments
double RT_dist = DEFAULT_RT_DIST, MZ_dist = DEFAULT_MZ_DIST;
int min_nb = DEFAULT_MIN_NB;
FILE *infile = NULL, *outfile = NULL;

typedef struct Point Point;
struct Point {
	double mz, I;
	int *cluster_flag; // Pointer to cluster point belongs to (NULL if none)
};

typedef struct Scan Scan;
struct Scan {
	double RT;
	Scan *lo_nb, *hi_nb; // Pointers to lowest and highest neighbouring scans
	size_t len, alloc; // Current number and allocated memory for points
	Point *points; // Pointer to allocated array of Points
};

typedef struct Spectrum Spectrum;
struct Spectrum {
	size_t len, alloc;   // Current number and allocated memory for scans
	Scan *scans;         // Pointer to allocated array of Scans
};

typedef struct Neighbour Neighbour;
struct Neighbour {
	Scan *scan;
	Point *point;
};

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

// Adds a point to a scan
void addpointtoScan(Scan *scan, float mz, float I) {
	// Not enough space in points array of scan, so lengthen it
	if (scan->len == scan->alloc) {
		scan->alloc += PT_INC;
		scan->points = realloc(scan->points,scan->alloc*sizeof(Point));
		if (scan->points == NULL)
			infox ("Couldn't grow points array.", -1);
	}
	
	scan->points[scan->len].mz = mz;
	scan->points[scan->len].I = I;
	scan->points[scan->len].cluster_flag = NULL;
	++scan->len;
}

// Initializes a scan (constructor)
void constructScan(Scan *scan, double RT) {
	scan->RT = RT;
	scan->lo_nb = NULL;
	scan->hi_nb = NULL;
	scan->len = 0;
	scan->alloc = PT_INIT;
	scan->points = malloc(scan->alloc*sizeof(Point));
	if (scan->points == NULL)
		infox ("Couldn't init points array.", -1);
}

// Frees all the allocated memory in a scan (destructor)
void destructScan(Scan *scan) {
	free(scan->points);
}

// Adds a point to a spectrum
void addpointtoSpectrum(Spectrum *spec, float RT, float mz, float I) {
	Scan *scan = NULL;
	
	// Search for scan in spectrum to add to
	for (int a = spec->len-1; a >= 0 ;--a) {
		if (RT == spec->scans[a].RT) {
			scan = &(spec->scans[a]);
			break;
		}
	}
	
	// Binary search (need to keep scans array sorted)
	// Scan *scan = bsearch(&RT, spec->scans, spec->len, sizeof(Scan*), compRT);
	
	// Scan does not exist in spectrum, so create a new one
	if (scan == NULL) {
		// Not enough space in scans array of spectrum, so lengthen it
		if (spec->len == spec->alloc) {
			spec->alloc += RT_INC;
			spec->scans = realloc(spec->scans,spec->alloc*sizeof(Scan));
			if (spec->scans == NULL)
				infox ("Couldn't grow scans array.", -1);
		}
		scan = &(spec->scans[spec->len]);
		++spec->len;
		
		constructScan(scan, RT);
	}
	
	addpointtoScan(scan, mz, I);
}

// Initializes a spectrum (constructor)
void constructSpectrum(Spectrum *spec) {
	spec->len = 0;
	spec->alloc = RT_INIT;
	spec->scans = malloc(spec->alloc*sizeof(Scan));
	if (spec->scans == NULL)
		infox ("Couldn't init scans array.", -1);
}

// Frees all the allocated memory in a spectrum (destructor)
void destructSpectrum(Spectrum *spec) {
	for (int a = 0; a < spec->len; ++a)
		destructScan(&(spec->scans[a]));
	free(spec->scans);
}

// Compare points by mz (qsort and bsearch comparator)
int compPoint ( const void *Point1, const void *Point2 ) {
	if (((Point*)Point1)->mz > ((Point*)Point2)->mz) return 1;
	if (((Point*)Point1)->mz < ((Point*)Point2)->mz) return -1;
	return 0;
}

// Trim unused memory and sort points in a scan
void optimizeScan(Scan *scan) {
	scan->alloc = scan->len;
	scan->points = realloc(scan->points,scan->alloc*sizeof(Point));
	if (scan->points == NULL)
		infox ("Couldn't trim points array.", -1);
	qsort(scan->points, scan->len, sizeof(Point), compPoint);
}

// Compare scans by RT (qsort and bsearch comparator)
int compScan ( const void *Scan1, const void *Scan2 ) {
	if (((Scan*)Scan1)->RT > ((Scan*)Scan2)->RT) return 1;
	if (((Scan*)Scan1)->RT < ((Scan*)Scan2)->RT) return -1;
	return 0;
}

// Compare double to scan RT (qsort and bsearch comparator)
int compRT ( const void * pkey, const void * pelem ) {
	if (*(double*)pkey > ((Scan*)pelem)->RT) return 1;
	if (*(double*)pkey < ((Scan*)pelem)->RT) return -1;
	return 0;
}

// Trim unused memory, sort scans and calculate neighbours in a spectrum
void optimizeSpectrum(Spectrum *spec) {
	spec->alloc = spec->len;
	spec->scans = realloc(spec->scans,spec->alloc*sizeof(Scan));
	if (spec->scans == NULL)
		infox ("Couldn't trim scans array.", -1);
	qsort(spec->scans, spec->len, sizeof(Scan), compScan);
	
	for(int a=0; a<spec->len ;++a) {
		double lo_RT = spec->scans[a].RT - RT_dist;
		double hi_RT = spec->scans[a].RT + RT_dist;
		
		spec->scans[a].lo_nb = bsearch2(&lo_RT, &(spec->scans[0]), a+1, 
										sizeof(Scan), compScan, 1);
		if (spec->scans[a].lo_nb == NULL)
			infox ("Couldn't find low scan neighbour.", -5);
		spec->scans[a].lo_nb = bsearch2(&hi_RT, &(spec->scans[a]), spec->len-a, 
										sizeof(Scan), compScan, -1);
		if (spec->scans[a].hi_nb == NULL)
			infox ("Couldn't find high scan neighbour.", -5);
		optimizeScan(&(spec->scans[a]));
	}
}

// Parse and validate command line, storing parameters and opening files
void parse_command_line(int argc, char** argv) {
	int opt;

	while ((opt = getopt (argc, argv, "t:s:c:l:v:")) != -1) {
		switch (opt) {
			case 't':
				RT_dist = atof(optarg);
				break;
			case 'm':
				MZ_dist = atof(optarg);
				break;
			case 'n':
				min_nb = atoi(optarg);
				break;
			case '?':
			default:
				break;
		}
	}

	if (argc-optind != 2) {
		// Print usage information and abort if too few or too many parameters
		printf("Usage: %s [flags] input.csv output.csv\n",argv[0]);
		printf("\n");
		printf("Flags: -t <RT> Maximum RT distance in s (default %.3f)\n", DEFAULT_RT_DIST);
		printf("       -m <mz> Maximum m/z distance in ue (default %.3f)\n", DEFAULT_MZ_DIST);
		printf("       -n <nb> Minimum number of neighbours (default %d)\n", DEFAULT_MIN_NB);
		exit (1);
	} else {
		infile = fopen(argv[optind],"r");
		if (infile == NULL)
			infox("Couldn't open input file.", -2);
		outfile = fopen(argv[++optind],"w");
		if (outfile == NULL)
			infox("Couldn't open output file.", -2);
	}
}

// Find the neighbours of a particular pt
int getNeighbours(Neighbour pt, Neighbour *ret) {
	int nbs = 0;
	Scan *curr_scan;
	
	for(curr_scan = pt.scan->lo_nb; curr_scan <= pt.scan->hi_nb; curr_scan++) {
		
	}
	
	return nbs;
}

// Assign cluster numbers in a spectrum
void clusterSpectrum(Spectrum *spec, int *flags) {
	Neighbour curr_pt, nbbuff[NBBUFLEN];
	int curr_flag = 1;
	
	for(int a=0; a<spec->len; ++a) {
		curr_pt.scan = &(spec->scans[a]);
		for(int b=0; b<spec->scans[a].len; ++b) {
			curr_pt.point = &(curr_pt.scan->points[b]);
			if (curr_pt.point->cluster_flag == NULL) {
				int nbs = getNeighbours(curr_pt, nbbuff);
				if (nbs >= min_nb) {
					// Cluster point
					
				} else {
					// Noise point
					curr_pt.point->cluster_flag = &(flags[0]);
				}
			}
		}
	}
}

int main(int argc, char** argv)
{
	char line [BUFLEN] = {'\0'};
	Spectrum spec;
	int flags[N_FLAG];
	
	constructSpectrum(&spec);
	for(int a = 0; a<N_FLAG; ++a) flags[a]=a;
	
	infile = fopen(argv[1],"r");
	while(fgets(line,BUFLEN,infile)!=NULL) {
		double RT, mz, I;
		int ret = sscanf (line, " %*d  %lf  %lf  %lf", &RT, &mz, &I);
		if (ret == 3)
			addpointtoSpectrum(&spec, RT, mz, I);
	}
	fclose(infile);

	// optimizeSpectrum(&spec);

	// Print out a summary of spec
	int max = 0;
	for(int a=0; a<spec.len; ++a){
		if (spec.scans[a].len > spec.scans[max].len) max = a;
		printf("Scan %d: RT %f, %d points\n", a, spec.scans[a].RT, spec.scans[a].len);
	}
	printf("Max - Scan %d: RT %f, %d points\n", max, spec.scans[max].RT, spec.scans[max].len);

	//clusterSpectrum(&spec, flags);

	destructSpectrum(&spec);
	return 0;
}
