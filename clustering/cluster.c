//      cluster.c

#include <stdio.h>
#include <stdlib.h>
#include "cluster.h"

#define BUFLEN 300   // Size of line buffer during file reading
#define RT_INIT 200  // Initial number of scans allocated in a spectrum
#define RT_INC 100   // Number of scans to expand by when a spectrum fills up
#define PT_INIT 2000 // Initial number of points allocated in a scan
#define PT_INC 1000  // Number of points to expand by when a scan fills up

typedef struct {
	double RT;
	unsigned int len, alloc; // Current number and allocated memory for points
	Point *points; // Pointer to allocated array of Points
} Scan;

typedef struct {
	unsigned int len, alloc; // Current number and allocated memory for scans
	Scan *scans; // Pointer to allocated array of Scans
} Spectrum;

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

// Compare points by mz (qsort comparator)
int compPoint ( const void *Point1, const void *Point2 ) {
	if (((Point*)Point1)->mz > ((Point*)Point2)->mz) return 1;
	if (((Point*)Point1)->mz < ((Point*)Point2)->mz) return -1;
	return 0;
}

// Trims off unused allocated memory and sorts points in a scan
void optimizeScan(Scan *scan) {
	scan->alloc = scan->len;
	scan->points = realloc(scan->points,scan->alloc*sizeof(Point));
	if (scan->points == NULL)
		infox ("Couldn't trim points array.", -1);
	qsort(scan->points, scan->len, sizeof(Point), compPoint);
}

// Compare scans by RT (qsort comparator)
int compScan ( const void *Scan1, const void *Scan2 ) {
	if (((Scan*)Scan1)->RT > ((Scan*)Scan2)->RT) return 1;
	if (((Scan*)Scan1)->RT < ((Scan*)Scan2)->RT) return -1;
	return 0;
}

// Trims off unused allocated memory and sorts scans in a spectrum
void optimizeSpectrum(Spectrum *spec) {
	spec->alloc = spec->len;
	spec->scans = realloc(spec->scans,spec->alloc*sizeof(Scan));
	if (spec->scans == NULL)
		infox ("Couldn't trim scans array.", -1);
	qsort(spec->scans, spec->len, sizeof(Scan), compScan);
	
	for(int a=0; a<spec->len ;++a)
		optimizeScan(&(spec->scans[a]));
}

// Comparison function for binary search
/* int compRT ( const void * pkey, const void * pelem ) {
	if (*(float*)pkey > (Scan*)pelem->RT) return 1;
	if (*(float*)pkey < (Scan*)pelem->RT) return -1;
	return 0;
} */

int main(int argc, char** argv)
{
	FILE *infile;
	char line [BUFLEN] = {'\0'};
	Spectrum spec;
	
	constructSpectrum(&spec);
	
	infile = fopen(argv[1],"r");
	while(fgets(line,BUFLEN,infile)!=NULL) {
		double RT, mz, I;
		int ret = sscanf (line, " %*d  %lf  %lf  %lf", &RT, &mz, &I);
		if (ret == 3)
			addpointtoSpectrum(&spec, RT, mz, I);
	}
	fclose(infile);
	
	optimizeSpectrum(&spec);

	// Print out a summary of spec
	for(int a=0; a<spec.len; ++a){
		printf("Scan %d: RT %f, %u points\n", a, spec.scans[a].RT, spec.scans[a].len);
	}

	
	destructSpectrum(&spec);
	return 0;
}
