// clm_flags.c

#include <stdio.h>
#include <string.h>
#include "clm.h"

#define BUFLEN 100

// Return index to next available flag in an array of flags or <0 for error
int getnextFlag(const Flag *flags, int len, int current) {
	int next;

	// Invalid arguments
	if (len<=0) return -1;
	if (current<0) return -1;
	if (current>=len) return -1;

	next = current;
	do {
		if (flags[next].last_seen == -1) return next; // Return available flag
		if (++next >= len) next -= len; // Go to next flag, wrapping around
	} while (next != current); // Stop if we come back to the starting flag
	return -2; // No available flag found
}

// Return the number of unique colors in flags[] (or <0 for error) and put the
// latest last_seen for each of those colors in latest[] (len(latest[]) >= len)
int getlatestFlags(const Flag *flags, int len, Flag *latest) {
	int tail = 0;
	int a,b;

	// Invalid arguments
	if (len <= 0) return -1;

	// Populate latest[] with the latest last_seen for each used color
	for (a=0; a<len; ++a) {
		if (flags[a].last_seen == -1) continue;
		for (b=0; b<tail; ++b) {
			if (flags[a].color == latest[b].color) {
				if (latest[b].last_seen < flags[a].last_seen)
					latest[b].last_seen = flags[a].last_seen;
				break;
			}
		}
		if (b >= tail) latest[tail++] = flags[a]; // Add unseen color to latest
	}

	return tail; // tail == len(latest[])
}

// Write all the clusters in mtx[][] with colors older than scan and at least
// min points to separate files in dir, given the output of getlatestseenFlags()
// and RT[]. Return the number of clusters written or <0 for error
int writeoldClusters(Point **mtx,int dim1,int dim2,const Flag *latest,int tail,
						int scan, const double *RT, const char *dir, int min) {
	char buf[BUFLEN];
	int a, b, c;
	double pointbuf[MIN_CLUSTER_SIZE][3];
	int written = 0;

	// Invalid arguments
	if (dim1 <= 0) return -1;
	if (dim2 <= 0) return -1;
	if (tail <= 0) return -1;
	if (scan < 0) return -1;
	if (min < 0) return -1;

	for (a=0; a<tail; ++a) {
		// Check if this color is old
		if (latest[a].last_seen == -1) continue;
		if (latest[a].last_seen >= scan) continue;

		FILE *outfile = NULL;
		//char longclust = 0;
		int points = 0;

		// Find points with colors that match this old color and write them out
		for (b=0; b<dim1; ++b) {
			for (c=0; c<dim2; ++c) {
				if (!mtx[b][c].mz) break; // End of scan, so break to next scan
				if (mtx[b][c].cluster_flag->color == latest[a].color) {
					//if (b==0) longclust = 1; //DEBUG
					if (points < min) {
						// Buffer points temporarily
						pointbuf[points][0] = RT[b];
						pointbuf[points][1] = mtx[b][c].mz;
						pointbuf[points][2] = mtx[b][c].I;
					} else {
						if (points == min) {
							// Open file to write the cluster into
							sprintf(buf,"%s/%06d.clust",dir,latest[a].color);
							outfile = fopen(buf, "a");
							if (!outfile) return -2; // Could not open file

							// Dump buffered points to file
							int d;
							for(d=0;d<MIN_CLUSTER_SIZE;++d)
								fprintf(outfile, "%f %f %f\n", pointbuf[d][0],
										pointbuf[d][2], pointbuf[d][2]);
						}
						fprintf(outfile,"%f %f %f\n", RT[b], mtx[b][c].mz,
								mtx[b][c].I);
					}
					++points;
				}
			}
		}
		//if (longclust) printf("Cluster %d reached head of buffer!\n",latest[a].color);//DEBUG

		if (outfile) fclose(outfile);
		++written;
	}

	return written;
}

// Mark old flags as available and give them unique new color numbers, given the
// output of getlatestseenFlags(). Return number of flags cleared or <0 if error
int clearoldFlags(Flag *flags, int len, const Flag *latest, int tail, int scan){
	int a,b;
	int curr_color = 0;
	int cleared = 0;

	// Invalid arguments
	if (len <= 0) return -1;
	if (tail <= 0) return -1;
	if (scan < 0) return -1;

	// Put highest color number in flags in curr_color
	for (a=0; a<len; ++a)
		if (curr_color < flags[a].color) curr_color = flags[a].color;

	for (a=0; a<len; ++a) {
		if (flags[a].last_seen == -1) continue;
		for (b=0; b<tail; ++b) {
			if (flags[a].color == latest[b].color) {
				if (latest[b].last_seen < scan) {
					flags[a].color = ++curr_color;
					flags[a].last_seen = -1;
					++cleared;
				} else flags[a].last_seen = latest[b].last_seen;
				break;
			}
		}
	}

	return cleared;
}
