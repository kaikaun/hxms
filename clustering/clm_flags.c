// clm_flags.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> //DEBUG
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

// Return the number of unique colors in flags[] (or <0 for error) and latest 
// last_seen for each of those colors in latest[] (len(latest[]) >= len)
int getlatestFlags(const Flag *flags, int len, Flag *latest) {
	int tail = 0;
	int a,b;

	// Invalid arguments
	if (len <= 0) return -1;

	// Populate latest[] with the latest last_seen for each used color
	for (a=0; a<len; ++a) {
		if (flags[a].last_seen < 0) continue;
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

// Replace all last_seen in flags with the latest for that color
// Return the number of flags freshened or <0 for error
int freshenFlags(Flag *flags, int len, const Flag *latest, int tail) {
	int freshened = 0;
	int a,b;

	// Invalid arguments
	if (len <= 0) return -1;
	if (tail <= 0) return -1;

	for(a=0;a<len;++a) {
		if (flags[a].last_seen < 0) continue;
		for (b=0; b<tail; ++b) {
			if (flags[a].color == latest[b].color) {
				if (flags[a].last_seen < latest[b].last_seen) {
					flags[a].last_seen = latest[b].last_seen;
					++freshened;
				}
			}
		}
	}
	return freshened;
}

// Write a single point to a file
int writePoint(FILE *outfile, int scan_no, double RT, double mz, double I) {
	return fprintf(outfile,"%6d %9.3lf %9.3lf %9.3lf\n",scan_no,RT,mz,I);
}

// Write all the clusters in mtx[][] with colors older than scan and at least
// min points to separate files in dir, given the output of getlatestseenFlags()
// and RT[]. Return the number of clusters written or <0 for error
int writeoldClusters(Point **mtx,int dim1,int dim2,const Flag *latest,int tail,
		int scan, const double *RT, int scan_base, const char *dir, int min) {
	char buf[BUFLEN];
	int a, b, c;
	double ptbuf[min][4];
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
		int points = 0;

		// Find points with colors that match this old color and write them out
		for (b=0; b<dim1; ++b) {
			for (c=0; c<dim2; ++c) {
				if (!mtx[b][c].mz) break; // End of scan, so break to next scan
				if (mtx[b][c].cluster_flag->color == latest[a].color) {
					if (points < min) {
						// Buffer points temporarily
						ptbuf[points][0] = scan_base + b;
						ptbuf[points][1] = RT[b];
						ptbuf[points][2] = mtx[b][c].mz;
						ptbuf[points][3] = mtx[b][c].I;
					} else {
						if (points == min) {
							// Open file to write the cluster into
							sprintf(buf,"%s/%06d.clust",dir,latest[a].color);
							outfile = fopen(buf, "a");
							if (!outfile) return -2; // Could not open file

							// Dump buffered points to file
							int d;
							for(d=0;d<min;++d)
								writePoint(outfile,(int)ptbuf[d][0],ptbuf[d][1],
								ptbuf[d][2],ptbuf[d][3]);
						}
						writePoint(outfile,scan_base+b,RT[b],mtx[b][c].mz,
									mtx[b][c].I);
					}
					++points;
				}
			}
		}
		if (outfile) fclose(outfile);
		++written;
	}
	return written;
}

// Write clusters in mtx[][] with colors older than scan and at least min points
// to separate files in dir, given the output of getlatestseenFlags() and RT[]
// Return the number of clusters written or <0 for error (mtx must be freshened)
int writeClusters(Point **mtx,int dim1,int dim2,const Flag *latest,int tail,
		int scan, const double *RT, int scan_base, const char *dir, int min) {
	// Invalid arguments
	if (dim1 <= 0) return -1;
	if (dim2 <= 0) return -1;
	if (tail < 0) return -1;
	if (scan < 0) return -1;
	if (min < 0) return -1;

	// Nothing to do
	if (tail == 0) return 0;

	int a, b, c;
	int written = 0;
	int pts[tail];
	memset(pts, 0, sizeof pts);

	// Assign ptbuf[tail][min][4] on the heap
	double ***ptbuf = NULL;
	if (--min > 0) {
		ptbuf = malloc(tail * sizeof(double**));
		ptbuf[0] = malloc(tail * min * sizeof(double*));
		ptbuf[0][0] = malloc(tail * min * 4 * sizeof(double));
		for (a=0; a<tail; ++a) {
			if (a > 0) {
				ptbuf[a] = ptbuf[a-1] + min;
				ptbuf[a][0] = ptbuf[a-1][0] + min*4;
			}
			for (b=1; b<min; ++b)
				ptbuf[a][b] = ptbuf[a][b-1] + 4;
		}
	}

	for (b=0; b<dim1; ++b) {
		for (c=0; c<dim2; ++c) {
			if (!mtx[b][c].mz) break;
			if (mtx[b][c].cluster_flag->last_seen < 0) continue;
			if (mtx[b][c].cluster_flag->last_seen < scan) { // Assume freshened
				// Find this color's index
				for (a=0; a<tail; ++a)
					if (mtx[b][c].cluster_flag->color == latest[a].color) break;
				//assert(a<tail);

				if (pts[a]<min) {
					// Buffer point until we have enough to write
					ptbuf[a][pts[a]][0] = scan_base + b;
					ptbuf[a][pts[a]][1] = RT[b];
					ptbuf[a][pts[a]][2] = mtx[b][c].mz;
					ptbuf[a][pts[a]][3] = mtx[b][c].I;
				} else {
					// Open file to append the cluster to
					char buf[BUFLEN];
					sprintf(buf,"%s/%06d.clust",dir,latest[a].color);

					//DEBUG: Check whether file exists if cluster is long
					if (!b && scan_base > 0) {
						struct stat st;
						if (stat(buf,&st)) printf("%s was not present\n", buf);
					}

					FILE *outfile = fopen(buf, "a");
					if (!outfile) return -2; // Could not open file

					if (pts[a]==min) {
						// Dump this color's buffer to new file
						int d;
						for (d=0; d<min; ++d)
							writePoint(outfile,(int)ptbuf[a][d][0],
								ptbuf[a][d][1],ptbuf[a][d][2],ptbuf[a][d][3]);
						++written;
					}
					writePoint(outfile,scan_base+b,RT[b],mtx[b][c].mz,
								mtx[b][c].I);
					fclose(outfile);
				}
				++pts[a];
			}
		}
	}
	if (ptbuf) {
		free(ptbuf[0][0]);
		free(ptbuf[0]);
		free(ptbuf);
	}
	return written;
}


// Mark old flags as available and give them unique new color numbers, given the
// output of getlatestFlags() and the next highest color to use
// Return new highest color or <0 for error
int clearoldFlags(Flag *flags, int len, const Flag *latest, int tail, int scan,
					int curr_color){
	int a,b;

	// Invalid arguments
	if (len <= 0) return -1;
	if (tail <= 0) return -1;
	if (scan < 0) return -1;
	if (curr_color < 0) return -1;

	for (a=0; a<len; ++a) {
		if (flags[a].last_seen == -1) continue;
		for (b=0; b<tail; ++b) {
			if (flags[a].color == latest[b].color) {
				if (latest[b].last_seen < scan) {
					flags[a].color = curr_color++;
					flags[a].last_seen = -1;
				}
				break;
			}
		}
	}

	return curr_color;
}

// Change all flags with old_color to be the same as new_flag
// Return number of flags changed or <0 for error
int mergeColors(Flag *flags, int len, const Flag *new_flag, int old_color) {
	if (len <= 0) return -1; // Invalid arguments
	if (old_color == new_flag->color) return 0; //Nothing to do

	int a, changed = 0;
	for (a=0;a<len;++a) {
		if(flags[a].color == old_color){
			flags[a] = *new_flag;
			++changed;
		}
	}
	return changed;
}
