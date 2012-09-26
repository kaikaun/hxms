//  deadtime.c
//
//  Copyright 2012 David Khoo <davidk@bii.a-star.edu.sg>
//
//  Performs TOF deadtime correction on mzXML files

#define DEFAULT_SCANTIME   (double)0.23  // in s
#define DEFAULT_CYCLETIME  (double)45    // in μs
#define DEFAULT_NEDEADTIME (double)5     // in ns
#define DEFAULT_EDEADTIME  (double)0     // in ns
#define DEFAULT_LENGTH     (double)1078  // in mm
#define DEFAULT_VOLTAGE    (double)5630  // in V

#define e_amu (double)96485333.7 // elementary charge / atomic mass unit

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <assert.h>
#include "mxmlmzXML.h"

typedef struct {
	float mz, I;
} mzI_32;

typedef struct {
	double mz, I;
} mzI_64;

void parse_command_line(int, char**);
void correct_deadtime(mxml_node_t *);
int compare_mz_32(const void *, const void *);
int compare_mz_64(const void *, const void *);

// Global command line parameters with default values
FILE *input = NULL, *output = NULL;

double nedeadtime = DEFAULT_NEDEADTIME * 1E-9;
double edeadtime = DEFAULT_EDEADTIME * 1E-9;
double scantime = DEFAULT_SCANTIME, cycletime = DEFAULT_CYCLETIME * 1E-6;
double length = DEFAULT_LENGTH * 1E-3, voltage = DEFAULT_VOLTAGE;
double pulses; // Pulses per scan = scantime / cycletime
double A; // Proportionality const : t = A * sqrt(m/z), A = L/sqrt(2*(e/amu)*V)

// Warning flags
double highest_t = 0.0;

int main(int argc, char** argv) {
	mxml_node_t *tree = NULL, *node = NULL;

	parse_command_line(argc, argv);
	A = length / sqrt( 2 * e_amu * voltage );
	pulses = scantime / cycletime;

	mxmlSetCustomHandlers(mzXML_load_custom,mzXML_save_custom);
	tree = mxmlLoadFile(NULL,input,mzXML_load_cb);
	fclose(input);

	mxml_index_t *index = mxmlIndexNew(tree,"scan",NULL);
	mxmlIndexReset(index);
	while ((node = mxmlIndexEnum(index)) != NULL) {
		correct_deadtime(node);
	}

	// Write out mzXML
	mxmlSetWrapMargin(0);
	mxmlSaveFile(tree, output, mzXML_whitespace_cb);

	// Print out warnings
	if (highest_t > cycletime)
		printf("Warning: Longest flight time (%.3f μs) longer than cycle time (%.3f μs)\n", 
				highest_t*1E6, cycletime*1E6);

	// Clean up
	if (output) fclose(output);
	mxmlIndexDelete(index);
	mxmlDelete(tree);
	return 0;
}

// Parse and validate command line, storing parameters and opening files
void parse_command_line(int argc, char** argv) {
	int opt;

	while ((opt = getopt (argc, argv, "d:D:s:c:l:v:")) != -1) {
		switch (opt) {
			case 'd':
				nedeadtime = atof(optarg) * 1E-9;
				break;
			case 'D':
				edeadtime = atof(optarg) * 1E-9;
				break;
			case 's':
				scantime = atof(optarg);
				break;
			case 'c':
				cycletime = atof(optarg) * 1E-6;
				break;
			case 'l':
				length = atof(optarg) * 1E-3;
				break;
			case 'v':
				voltage = atof(optarg);
				break;
			case '?':
			default:
				break;
		}
	}

	if (argc-optind != 2) {
		// Print usage information and abort if too few or too many parameters
		printf("Usage: %s [flags] input.mzXML output.mzXML\n",argv[0]);
		printf("\n");
		printf("Flags: -d <deadtime>  Non-extending TDC deadtime in ns (default %.3f)\n", DEFAULT_NEDEADTIME);
		printf("       -D <deadtime>  Extending TDC deadtime in ns (default %.3f)\n", DEFAULT_EDEADTIME);
		printf("       -s <scantime>  Scan time in s (default %.3f)\n", DEFAULT_SCANTIME);
		printf("       -c <cycletime> Cycle time in μs (default %.3f)\n", DEFAULT_CYCLETIME);
		printf("       -l <length>    Effective TOF path length in mm (default %.3f)\n", DEFAULT_LENGTH);
		printf("       -v <voltage>   Effective accelerating voltage in V (default %.3f)\n", DEFAULT_VOLTAGE);
		printf("\nNon-extending deadtime should be greater than extending deadtime.\n");
		exit (1);
	} else {
		input = openfile(argv[optind],"r");
		output = openfile(argv[++optind],"w");
	}
}

// Correct peaklist of scan node for deadtimes
void correct_deadtime(mxml_node_t *node) {
	mxml_node_t *peaksnode = mxmlFindElement(node, node, "peaks", NULL, NULL, MXML_DESCEND);
	int peaks = atoi(mxmlElementGetAttr(node, "peaksCount"));
	void *peaklist = (void *)mxmlGetCustom(peaksnode);
	int size = atoi(mxmlElementGetAttr(peaksnode,"precision"))/8;
	assert(4 == size || 8 == size);

	struct {
		double I0; // Original uncorrected intensity, i.e. reported hits
		double t; // Flight time for this peak in s
	} peakattr[peaks];

	if (4 == size) qsort(peaklist, peaks, sizeof(mzI_32), compare_mz_32);
	else qsort(peaklist, peaks, sizeof(mzI_64), compare_mz_64);

	for(int i = 0; i < peaks; ++i) {
		double mz, I;
		double pulses_not_hit = pulses; // Number of pulses not in NE DT
		double blocked_pulses = 0;      // Number of pulses in E DT

		if (4 == size) {
			mz = ((mzI_32*)peaklist)[i].mz;
			I = ((mzI_32*)peaklist)[i].I;
		} else {
			mz = ((mzI_64*)peaklist)[i].mz;
			I = ((mzI_64*)peaklist)[i].I;
		}

		peakattr[i].I0 = I;
		peakattr[i].t = A * sqrt(mz);
		if (peakattr[i].t > highest_t ) highest_t = peakattr[i].t;

		for(int j = i-1; j >= 0; --j) {
			assert(peakattr[i].t > peakattr[j].t);
			if (peakattr[i].t - peakattr[j].t > nedeadtime) break;
			if (peakattr[i].t - peakattr[j].t > edeadtime) {
				pulses_not_hit -= peakattr[j].I0;
			} else {
				if (4 == size) blocked_pulses += ((mzI_32*)peaklist)[j].I;
				else blocked_pulses += ((mzI_64*)peaklist)[j].I;
			}
		}
		assert(pulses_not_hit > I);

		I = -log(1 - I * exp(blocked_pulses/pulses) / pulses_not_hit) * pulses;

		if (4 == size) ((mzI_32*)peaklist)[i].I = I;
		else ((mzI_64*)peaklist)[i].I = I;
	}
}

// qsort comparison function for 32-bit m/z-I pairs to sort by m/z
int compare_mz_32(const void *pair1, const void *pair2) {
	if ((*(mzI_32*)pair1).mz < (*(mzI_32*)pair2).mz) return -1;
	else if ((*(mzI_32*)pair1).mz > (*(mzI_32*)pair2).mz) return 1;
	else return 0;
}

// qsort comparison function for 64-bit m/z-I pairs to sort by m/z
int compare_mz_64(const void *pair1, const void *pair2) {
	if ((*(mzI_64*)pair1).mz < (*(mzI_64*)pair2).mz) return -1;
	else if ((*(mzI_64*)pair1).mz > (*(mzI_64*)pair2).mz) return 1;
	else return 0;
}
