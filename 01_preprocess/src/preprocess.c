//  preprocess.c
//
//  Copyright 2012 David Khoo <davidk@bii.a-star.edu.sg>
//
//  Preprocesses mzXML produced by Waters Qtof micro MS in MS^E mode

#define DEFAULT_SCAN_TYPE "Full"

#define SCAN_DIST 20 // Number of scans for two peaks to be considered near
#define MZ_DIST 0.05 // m/z gap for two peaks to be considered near

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <float.h>
#include <assert.h>
#include "mxmlmzXML.h"

typedef struct {
	float mz, I;
} mzI_32;

typedef struct {
	double mz, I;
} mzI_64;

typedef struct {
	unsigned int scan_num;
	double RT, mz, I;
} peak;

void parse_command_line(int, char**);
unsigned int strip_peaks(mxml_node_t *);
void find_highest_peaks(mxml_node_t *, peak *);
void sax_cb(mxml_node_t *, mxml_sax_event_t,void *);
int compare_I_peak(const void *, const void *);
void usage(char**);

// Global command line parameters with default values
char *inputname = NULL, *outputname = NULL;
FILE *input = NULL, *output = NULL;
const char *scan_type = DEFAULT_SCAN_TYPE;
unsigned int n_highest = 0, min_num = 0, max_num = UINT_MAX;
double min_t = 0, max_t = DBL_MAX, min_mz = 0, max_mz = DBL_MAX, min_I = 0;
bool compress_peaks = true, verbose = false, renumber_scans = true;
enum {
NEVER, NO, YES
} skip = NEVER, write_csv = NO;

int main(int argc, char** argv) {
	mxml_node_t *tree = NULL, *node = NULL;
	const char *low_t_str = NULL, *high_t_str = NULL;
	double low_t = DBL_MAX, high_t = 0;
	peak *highest;

	parse_command_line(argc, argv);

	// Load relevant parts of XML tree using SAX
	if (verbose) printf("Parsing input mzXML\n");
	mxmlSetCustomHandlers(mzXML_load_custom,mzXML_save_custom);
	tree = mxmlSAXLoadFile(NULL,input,mzXML_load_cb,sax_cb,NULL);
	fclose(input);
	if (verbose) printf("Parsing done\n");

	// Allocate array for n-highest peaks, plus one extra for additions
	if (n_highest > 0) highest = calloc(n_highest + 1, sizeof(peak));

	mxml_index_t *index = mxmlIndexNew(tree,"scan",NULL);
	mxmlIndexReset(index);
	while ((node = mxmlIndexEnum(index)) != NULL) {
		mxml_node_t *peaksnode = mxmlFindElement(node, node, "peaks", NULL, NULL, MXML_DESCEND);

		if (verbose) printf("\rProcessing scan %s", mxmlElementGetAttr(node,"num"));

		unsigned int peaks = strip_peaks(peaksnode); // Strip out unwanted peaks
		if (n_highest > 0)
			find_highest_peaks(peaksnode, highest); // Look for highest peaks

		if (write_csv == NO) {
			if (compress_peaks)
				mxmlElementSetAttr(peaksnode,"compressionType","zlib");
			else mxmlElementSetAttr(peaksnode,"compressionType","none");

			mxmlElementSetAttrf(node,"peaksCount","%u",peaks);

			// Delete base peak information
			mxmlElementDeleteAttr(node,"basePeakMz");
			mxmlElementDeleteAttr(node,"basePeakIntensity");

			// Update lowest and highest retentionTime
			double t = xsduration_to_s(mxmlElementGetAttr(node,"retentionTime"));
			if (t < low_t) {
				low_t = t;
				low_t_str = mxmlElementGetAttr(node,"retentionTime");
			}
			if (t > high_t) {
				high_t = t;
				high_t_str = mxmlElementGetAttr(node,"retentionTime");
			}
		}
	}

	if (verbose) printf("\nProcessing done\n\n");

	if (write_csv == NO) {
		// Set indexOffset to nil
		node = mxmlFindElement(tree, tree, "indexOffset", NULL, NULL, MXML_DESCEND);
		mxmlElementSetAttr(node,"xsi:nil","1");
		mxmlDelete(mxmlGetFirstChild(node));

		// Correct scanCount, startTime and endTime values
		node = mxmlFindElement(tree, tree, "msRun", NULL, NULL, MXML_DESCEND);
		mxmlElementSetAttrf(node,"scanCount","%u",mxmlIndexGetCount(index));
		mxmlElementSetAttr(node,"startTime",low_t_str);
		mxmlElementSetAttr(node,"endTime",high_t_str);

		// Write out mzXML
		mxmlSetWrapMargin(0);
		mxmlSaveFile(tree, output, mzXML_whitespace_cb);
	}

	// Print out highest n peaks
	if (n_highest > 0) {
		printf("%u highest peaks (num, RT, m/z, I)\n\n", n_highest);
		for (int i = 0; i < n_highest; ++i) {
			printf("%u %.3f %.3f %.3f\n", highest[i].scan_num, highest[i].RT, highest[i].mz, highest[i].I);
		}
		free(highest);
	}

	// Clean up
	if (output) fclose(output);
	mxmlIndexDelete(index);
	mxmlDelete(tree);
	return 0;
}

// Parse and validate command line, store params and open files
void parse_command_line(int argc, char** argv) {
	int opt;

	while ((opt = getopt (argc, argv, "s:p:n:N:t:T:m:M:i:cxzrh:v")) != -1) {
		switch (opt) {
			case 's':
				scan_type = optarg;
				break;
			case 'p':
				switch(optarg[0]) {
					case 'e':
						skip = YES;
						break;
					case 'o':
						skip = NO;
						break;
					case 'a':
					default:
						skip = NEVER;
						break;
				}
				break;
			case 'n':
				min_num = atoi(optarg);
				break;
			case 'N':
				max_num = atoi(optarg);
				break;
			case 't':
				min_t = atof(optarg);
				break;
			case 'T':
				max_t = atof(optarg);
				break;
			case 'm':
				min_mz = atof(optarg);
				break;
			case 'M':
				max_mz = atof(optarg);
				break;
			case 'i':
				min_I = atof(optarg);
				break;
			case 'c':
				write_csv = YES;
				break;
			case 'x':
				write_csv = NEVER;
				break;
			case 'z':
				compress_peaks = false;
				break;
			case 'r':
				renumber_scans = false;
				break;
			case 'h':
				n_highest = atoi(optarg);
				break;
			case 'v':
				verbose = true;
				break;
			default:
				break;
		}
	}

	if (optind < argc) {
		inputname = argv[optind];
		input = openfile(inputname,"r");
	} else usage(argv);
	if (write_csv != NEVER) {
		if (++optind < argc) {
			outputname = argv[optind];
			output = openfile(outputname,"w");
		} else usage(argv);
	}

	if (verbose) {
		printf("Reading from %s\n", inputname);
		printf("Keeping scans of \"%s\" type ",scan_type);
		switch(skip) {
			case NO:
				printf("and odd scan numbers ");
				break;
			case YES:
				printf("and even scan numbers ");
				break;
			case NEVER:
				printf("and all scan numbers ");
				break;
			default:
				exit( 129 ); // Should NEVER get here!
		}

		if (min_num != 0 && max_num != UINT_MAX) printf("between %u and %u ", min_num, max_num);
		else if (min_num != 0) printf ("above %u ", min_num);
		else if (max_num != UINT_MAX) printf("below %u ", max_num);

		if (min_t != 0 && max_t != DBL_MAX) printf("and %.3f < RT < %.3f", min_t, max_t);
		else if (min_t != 0) printf ("and RT > %.3f", min_t);
		else if (max_t != DBL_MAX) printf("and RT < %.3f", max_t);
		printf("\n");

		if (min_t != 0 || max_t != DBL_MAX || min_mz != 0 || max_mz != DBL_MAX || min_I != 0) {
			printf("Keeping peaks with ");

			if (min_mz != 0 && max_mz != DBL_MAX) printf("%.3f < m/z < %.3f ", min_mz, max_mz);
			else if (min_mz != 0) printf ("m/z > %.3f ", min_mz);
			else printf("m/z < %.3f ", max_mz);

			if (min_I != 0) printf("I > %.3f", min_I);
			printf("\n");
		} else printf("Keeping all peaks\n");

		switch(write_csv) {
			case NO:
				if (compress_peaks) printf("Writing compressed mzXML to %s\n", outputname);
				else printf("Writing uncompressed mzXML to %s\n", outputname);
				break;
			case YES:
				printf("Writing CSV to %s\n", outputname);
				break;
			case NEVER:
				printf("Writing nothing\n");
				break;
			default:
				exit( 129 ); // Should NEVER get here!
		}

		if (n_highest > 0) printf("Printing %u highest peaks\n", n_highest);
		printf("\n");
	}
}

// SAX callback function
void sax_cb(mxml_node_t *node, mxml_sax_event_t event,void *data){
	static int num = 0;
	mxml_node_t *parent = mxmlGetParent(node);

	if (event == MXML_SAX_ELEMENT_OPEN) {
		char *name = (char *)mxmlGetElement(node);

		// Do not retain optional trees
		if (!strcmp(name, "sha1") || !strcmp(name, "index")) {
			return;
		} else if (!strcmp(name,"scan")) {
			if (strcmp(mxmlElementGetAttr(node,"scanType"),scan_type)) {
				// Reject scan node if wrong scanType, without toggling skip
				return;
			} else {
				switch (skip) {
				case YES:
					// Reject scan node if skip == YES
					skip = NO;
					return;
				case NO:
					// Else reject next scan node unless never skipping
					skip = YES;
				case NEVER:
				{
					unsigned int scan_num = atoi(mxmlElementGetAttr(node,"num"));
					double t = xsduration_to_s(mxmlElementGetAttr(node,"retentionTime"));
					if (t < min_t || t > max_t || scan_num < min_num || scan_num > max_num) {
						// Reject scan node if wrong retentionTime or scan_num
						return;
					} else {
						// Renumber scan
						if (renumber_scans) mxmlElementSetAttrf(node,"num","%u",++num);
						// Accept scan node
						mxmlRetain(node);
					}
					break;
				}
				default:
					exit( 129 ); // Should NEVER get here!
				}
			}
		} else if (!strcmp(name,"peaks")) {
			if (mxmlGetRefCount(parent) > 1) {
				// Accept peak nodes only if parent scan node was accepted
				mxmlRetain(node);
			}
		} else {
			mxmlRetain(node);
		}
	} else if (event == MXML_SAX_DIRECTIVE) {
		mxmlRetain(node);
	} else if (event == MXML_SAX_DATA) {
		if (mxmlGetRefCount(parent) > 1) {
			if (!strcmp(mxmlGetElement(parent),"peaks")) {
				// Only accept data nodes if they are children of peak nodes
				// This may be incorrect for certain separation schemas
				// TODO: Correct this to take above into account
				mxmlRetain(node);
			}
		}
	}
}

// Removes unwanted peaks and returns number of remaining peaks
unsigned int strip_peaks(mxml_node_t *peaksnode) {
	int scan_num = atoi(mxmlElementGetAttr(mxmlGetParent(peaksnode),"num"));
	int length = atoi(mxmlElementGetAttr(peaksnode,"compressedLen"));
	int peaks = 0;
	int size = atoi(mxmlElementGetAttr(peaksnode,"precision"))/8;
	assert(4 == size || 8 == size);

	void *new, *new_ptr, *old, *old_ptr;
	old = old_ptr = (void *)mxmlGetCustom(peaksnode); // Old peak list and position to read from
	new = new_ptr = malloc(length); // New peak list and position to write to

	while(old_ptr-old < length) {
		double mz, I;

		if (4 == size) {
			mz = (*(mzI_32*)old_ptr).mz;
			I = (*(mzI_32*)old_ptr).I;
		} else {
			mz = (*(mzI_64*)old_ptr).mz;
			I = (*(mzI_64*)old_ptr).I;
		}

		// Accept peak if mz and I are within range
		if ( I > min_I && mz > min_mz && mz < max_mz) {
			// Write peak to CSV
			if (write_csv == YES) {
				mxml_node_t *node = mxmlGetParent(peaksnode);
				double RT = xsduration_to_s(mxmlElementGetAttr(node,"retentionTime"));
				fprintf(output,"%u %.3f %.3f %.3f\n",scan_num,RT,mz,I);
			}
			// Copy peak to new peak list
			memcpy(new_ptr, old_ptr, 2*size);
			new_ptr += 2*size;
			++peaks;
		}
		old_ptr += 2*size;
	}

	length = new_ptr-new;
	assert(length == 2*size*peaks);
	new = realloc(new,length);
	mxmlElementSetAttrf(peaksnode,"compressedLen","%u",length);
	mxmlSetCustom(peaksnode,new,mzXML_destroy_custom); // Handles freeing of new

	return peaks;
}

// Updates the list of n-highest peaks (VERY QUICK AND DIRTY!)
void find_highest_peaks(mxml_node_t *peaksnode, peak *highest) {
	mxml_node_t *node = mxmlGetParent(peaksnode);
	void *peaklist = (void *)mxmlGetCustom(peaksnode);
	int scan_num = atoi(mxmlElementGetAttr(mxmlGetParent(peaksnode),"num"));
	double RT = xsduration_to_s(mxmlElementGetAttr(node,"retentionTime"));
	int length = atoi(mxmlElementGetAttr(peaksnode,"compressedLen"));
	int size = atoi(mxmlElementGetAttr(peaksnode,"precision"))/8;
	assert(4 == size || 8 == size);

	for (int i = 0; i < length/(2*size); ++i) {
		double mz, I;

		// Continue to next pair if I is not a local maximum in peak list
		if (4 == size) {
			mzI_32 *peaks32 = (mzI_32*)peaklist;
			if (i > 0) {
				if (peaks32[i].I < peaks32[i-1].I) continue;
			}
			if (i < length/(2*size)-1) {
				if (peaks32[i].I < peaks32[i+1].I) continue;
			}
			mz = peaks32[i].mz;
			I = peaks32[i].I;
		} else {
			mzI_64 *peaks64 = (mzI_64*)peaklist;
			if (i > 0) {
				if (peaks64[i].I < peaks64[i-1].I) continue;
			}
			if (i < length/(2*size)-1) {
				if (peaks64[i].I < peaks64[i+1].I) continue;
			}
			mz = peaks64[i].mz;
			I = peaks64[i].I;
		}

		// Continue to next pair if I lower than lowest of the highest peak list
		if (I < highest[n_highest-1].I)
			continue;

		int j;
		for (j = 0; j < n_highest; ++j) {
			// If peak with scan_num and mz within distance exists
			if (abs(highest[j].scan_num - scan_num) < SCAN_DIST &&
				abs(highest[j].mz - mz) < MZ_DIST) {
					// If that peak is lower, replace it and sort the array
					if (highest[j].I < I) {
						highest[j].scan_num = scan_num;
						highest[j].RT = RT;
						highest[j].mz = mz;
						highest[j].I = I;
						qsort(highest, n_highest, sizeof(peak), compare_I_peak);
					}
					break;
			}
		}
		if (j < n_highest) continue;

		// Peak passed with no neighbor. Add it to end of array of highest entries
		highest[n_highest].scan_num = scan_num;
		highest[n_highest].RT = RT;
		highest[n_highest].mz = mz;
		highest[n_highest].I = I;

		// Sort array plus extra value in reverse order to bring highest to front
		qsort(highest, n_highest+1, sizeof(peak), compare_I_peak);
	}
}

// qsort comparison function for peaks to sort in REVERSE order by I
int compare_I_peak(const void *peak1, const void *peak2) {
	if ((*(peak*)peak2).I < (*(peak*)peak1).I) return -1;
	else if ((*(peak*)peak2).I > (*(peak*)peak1).I) return 1;
	else return 0;
}

// Print usage information and abort
void usage(char** argv) {
	printf("Usage: %s [flags] inname outname\n",argv[0]);
	printf("\n");
	printf("Flags: -s <scanType> Keep scans of this scanType (default %s)\n", DEFAULT_SCAN_TYPE);
	printf("                     e.g. calibration, zoom, SIM, SRM, CRM, Q1, Q3\n");
	printf("       -p <a/e/o>    Keep \"a\"ll (default), \"e\"ven or \"o\"dd scans\n");
	printf("\n");
	printf("       -n <scannum>  Minimum scan number (default 0)\n");
	printf("       -N <scannum>  Maximum scan number (default max)\n");
	printf("       -t <time>     Minimum retentionTime in s (default 0)\n");
	printf("       -T <time>     Maximum retentionTime in s (default max)\n");
	printf("\n");
	printf("       -m <m/z>      Minimum m/z in u/e (default 0)\n");
	printf("       -M <m/z>      Minimum m/z in u/e (default max)\n");
	printf("       -i <I>        Minimum peak intensity (default 0)\n");
	printf("\n");
	printf("       -c            Write CSV instead of mzXML\n");
	printf("       -x            Do not write anything (for use with -h below)\n");
	printf("       -z            Do not compress peaklists in mzXML output\n");
	printf("       -r            Do not renumber scans (mzXML incompliant)\n");
	printf("\n");
	printf("       -h <n>        Display the n highest peaks (default 0)\n");
	printf("                     (Peaks typically extend across 10s RT and 0.5 m/z)\n");
	printf("       -v            Verbose output (default off)\n");
	exit (1);
}
