//      cluster.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <assert.h>
#include "mxmlmzXML.h"

#define DEFAULT_MZ_BINS 10000
#define EPSILON 1E-8

typedef struct {
	float mz, I;
} mzI_32;

typedef struct {
	double mz, I;
} mzI_64;

typedef struct {
	float I;
	int *cluster;
} Point;

void fill_point_buffer (Point *, mxml_node_t *, double *, int, double *, int);
double get_lowest_t_gap(mxml_node_t *);
void parse_command_line(int, char**);

FILE *input = NULL, *output = NULL;
int mz_bins = DEFAULT_MZ_BINS;

double t_gap;

int main(int argc, char** argv)
{
	mxml_node_t *tree = NULL, *node = NULL;
	double min_mz = DBL_MAX, max_mz = 0, min_t;
	Point *buff, *buffer[3];
	
	parse_command_line(argc, argv);
	
	// Read input mzXML
	mxmlSetCustomHandlers(mzXML_load_custom,mzXML_save_custom);
	tree = mxmlLoadFile(NULL,input,mzXML_load_cb);
	fclose(input);
	
	// Get number of scans
	node = mxmlFindElement(tree, tree, "msRun", NULL, NULL, MXML_DESCEND);
	int scans = atoi(mxmlElementGetAttr(node, "scanCount"));
	
	size_t buff_size = scans * mz_bins * sizeof(Point);
	buff = malloc(3 * buff_size);
	buffer[0] = buff;
	buffer[1] = buff + buff_size;
	buffer[2] = buff + 2*buff_size;
	double *mzs = malloc(3 * mz_bins * sizeof(double));
	
	// Allocate and set RT array, min_mz, max_mz
	double *RTs = malloc(scans * sizeof(double));
	for(int a = 0; a < scans; ++a) {
		char tmp[40];
		double mz;
		sprintf(tmp,"%d",a+1);
		node = mxmlFindElement(tree, tree, "scan", "num", tmp, MXML_DESCEND);
		RTs[a] = xsduration_to_s(mxmlElementGetAttr(node,"retentionTime"));
		mz = strtod(mxmlElementGetAttr(node,"lowMz"),NULL);
		if (mz < min_mz) min_mz = mz;
		mz = strtod(mxmlElementGetAttr(node,"highMz"),NULL);
		if (mz > max_mz) max_mz = mz;
	}
	min_t = sqrt(min_mz);
	t_gap = get_lowest_t_gap(node); // Set t_gap based on just the last scan
	
	// Initialize initial mzs array
	for(int a = 0; a < 3*mz_bins; ++a) {
		mzs[a] = (min_t + a*t_gap);
		mzs[a] *= mzs[a];
	}
	int mz_index = 3*mz_bins;
	
	// Load initial buffers
	memset(buff, 0, 3*buff_size);
	fill_point_buffer(buffer[0], tree, &mzs[0], mz_bins, &RTs[0], scans);
	fill_point_buffer(buffer[1], tree, &mzs[mz_bins], mz_bins, &RTs[0], scans);
	fill_point_buffer(buffer[2], tree, &mzs[2*mz_bins], mz_bins, &RTs[0], scans);
	
	int bins = mz_bins;
	while(bins == mz_bins) {
		// do_something();
		
		// Shift buffers 2 and 3 up
		memcpy(buffer[0], buffer[1], buff_size);
		memcpy(&mzs[0], &mzs[mz_bins], mz_bins*sizeof(double));
		memcpy(buffer[1], buffer[2], buff_size);
		memcpy(&mzs[mz_bins], &mzs[2*mz_bins], mz_bins*sizeof(double));

		// Fill buffer 3
		for(bins = 0; bins < mz_bins; ++bins) {
			mzs[2*mz_bins+bins] = min_t + (mz_index+bins)*t_gap;
			mzs[2*mz_bins+bins] *= mzs[2*mz_bins+bins];
			if (mzs[2*mz_bins+bins] > max_mz) break;
		}
		mz_index += bins;
		memset(buffer[2], 0, buff_size);
		fill_point_buffer(buffer[2], tree, &mzs[2*mz_bins], bins, &RTs[0], scans);
	}
	
	free(buff);
	free(RTs);
	free(mzs);
	return 0;
}

// Fills a buffer with points based on RTs and mzs in arrays
// - buffer should be pre-zeroed
// - mzs should be ascending
void fill_point_buffer
	(Point *buff, mxml_node_t *tree, double mzs[], int bins, double RTs[], int scans) {
	mxml_node_t *node, *peaksnode;
	
	mxml_index_t *index = mxmlIndexNew(tree,"scan",NULL);
	mxmlIndexReset(index);
	while ((node = mxmlIndexEnum(index)) != NULL) {
		// Find which scan we're on and assign it to a
		double RT = xsduration_to_s(mxmlElementGetAttr(node,"retentionTime"));
		int a=-1;
		for(int i=0; i<scans; ++i) {
			if (RTs[i] == RT) {
				a = i;
				break;
			}
		}
		if (a == -1) continue;
		
		// Load up the peak list
		peaksnode = mxmlFindElement(node, node, "peaks", NULL, NULL, MXML_DESCEND);
		int peaks = atoi(mxmlElementGetAttr(node, "peaksCount"));\
		void *peaklist = (void *)mxmlGetCustom(peaksnode);
		int size = atoi(mxmlElementGetAttr(peaksnode,"precision"))/8;
		assert(4 == size || 8 == size);
		
		// Go through all the peaks
		int next_mzs = 0;
		for(int b = 0; b < peaks; ++b) {
			double mz, I;
			
			if (4 == size) {
				mz = ((mzI_32*)peaklist)[b].mz;
				I = ((mzI_32*)peaklist)[b].I;
			} else {
				mz = ((mzI_64*)peaklist)[b].mz;
				I = ((mzI_64*)peaklist)[b].I;
			}
			
			if (mz < mzs[0]-EPSILON) continue;
			if (mz > mzs[bins]+EPSILON) continue;
			
			// If the peak mz matches one on the list, add its I to the buffer
			for(int c = next_mzs; c < bins; ++c) {
				if (abs(mzs[c]-mz) < EPSILON) {
					buff[a*scans+c].I = I;
					next_mzs = c+1;
					break;
				}
			}
			if (next_mzs >= bins) break;
		}
		
	}
	mxmlIndexDelete(index);
}

double get_lowest_t_gap(mxml_node_t *node) {
	double min_t_gap = DBL_MAX, last_t = 0;
	
	mxml_node_t *peaksnode = mxmlFindElement(node, node, "peaks", NULL, NULL, MXML_DESCEND);
	int peaks = atoi(mxmlElementGetAttr(node, "peaksCount"));
	void *peaklist = (void *)mxmlGetCustom(peaksnode);
	int size = atoi(mxmlElementGetAttr(peaksnode,"precision"))/8;
	assert(4 == size || 8 == size);
	
	for(int i = 0; i < peaks; ++i) {
		double t;
		
		if (4 == size) {
			t = sqrt(((mzI_32*)peaklist)[i].mz);
		} else {
			t = sqrt(((mzI_64*)peaklist)[i].mz);
		}
		
		if (t - last_t < min_t_gap) min_t_gap = t - last_t;
		last_t = t;
	}
	
	return min_t_gap;
}

void parse_command_line(int argc, char** argv) {
	input = openfile(argv[1],"r");
}
