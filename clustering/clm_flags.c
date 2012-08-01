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

// Write all the clusters in mtx[][] with colors older than scan to separate
// files in dir, given the output of getlatestseenFlags() and RT[]
// Return the number of clusters written or <0 for error
int writeoldClusters(Point **mtx, int dim1, int dim2, const Flag *latest,
						int tail, int scan, const double *RT, const char *dir) {
	char buf[BUFLEN];
	FILE *outfile;
	int a, b, c;
	int written = 0;

	// Invalid arguments
	if (dim1 <= 0) return -1;
	if (dim2 <= 0) return -1;
	if (tail <= 0) return -1;
	if (scan < 0) return -1;

	for (a=0; a<tail; ++a) {
		// Check if this color is old
		if (latest[a].last_seen == -1) continue;
		if (latest[a].last_seen >= scan) continue;

		// Open file to write the cluster into
		sprintf(buf,"%s/%06d.clust",dir,latest[a].color);
		outfile = fopen(buf, "w");
		if (!outfile) {
			return -2; // Could not open file to write cluster to
		}

		char toolong = 0; //DEBUG

		// Find points with colors that match this old color and write them out
		for (b=0; b<dim1; ++b) {
			for (c=0; c<dim2; ++c) {
				if (!mtx[b][c].mz) break; // End of scan, so break to next scan
				if (mtx[b][c].cluster_flag->color == latest[a].color) {
					fprintf(outfile, "%f %f %f\n", RT[b], mtx[b][c].mz,
							mtx[b][c].I);

					if (b==0) toolong = 1; //DEBUG
				}
			}
		}
		if (toolong) printf("Cluster %d reached head of buffer!\n",latest[a].color);//DEBUG

		fclose(outfile);
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

// Clear old flags with last_seen < scan from flags[]
// Send old colors and their latest last_seen to output() (0 return for success)
// Return number of cleared flags or <0 for error
/* int freshenFlags(Flag *flags,int len,int scan,int (*outflag)(Flag oldflag)) {
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

	// Go through new[] and pass flags with old colors to outflag()
	for (b=0; b<tail; ++b)
		if (new[b].last_seen < scan) {
			int ret = outflag(new[b]);
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
} */
