// clm_main.c

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "clm.h"

#define BUFLEN 300

int main(int argc, char** argv)
{
	char line [BUFLEN] = {'\0'};
	int a, b;
	
	FILE *infile;
	Point **points;
	double RTs[N_SCANS] = {0};
	Flag flags[N_FLAG], latest[N_SCANS];

	int scan_idx = N_SCANS, mz_idx = 0, current_flag = 0, total_scans = 0, tail;
	int RT_step = N_SCANS/3;
	int line_ctr = 0; //DEBUG

	// Check command line arguments, print usage if wrong
	if ( argc < 3 ) {
		fprintf (stderr, "Usage: %s <input table> <output dir>\n", argv[0]);
		exit (1);
	}

	// Check output dir exists
	struct stat st;
	stat(argv[2],&st);
	if(!S_ISDIR(st.st_mode))
		infox("Output dir does not exist", -2, __FILE__, __LINE__);

	// Open input file
	infile = fopen(argv[1],"r");
	if (infile == NULL)
		infox("Cannot open input file", -2, __FILE__, __LINE__);

	// Initialize matrix
	points = Pointmatrix(N_SCANS, N_MZPOINTS);
	if (!points)
		infox ("Couldn't create matrix.", -1, __FILE__, __LINE__);

	// Initialize flags;
	for(a = 0; a<N_FLAG; ++a) {
		flags[a].color = a;
		flags[a].last_seen = -1;
	}

	while(fgets(line,BUFLEN,infile)!=NULL) {
		double RT, mz, I;
		int ret = sscanf (line, " %*d  %lf  %lf  %lf", &RT, &mz, &I);

		if (ret != 3) continue; //should we warn the user?
		if (I < I_MIN) continue;

		if (!(++line_ctr%1000))printf("line %d, RT %f, %d points\n",line_ctr,RTs[scan_idx],mz_idx); //DEBUG

		// Assume that points in CSV are already sorted by RT, and move to next
		// scan if RT changes, stepping if necessary
		if (RT != RTs[scan_idx]) {
			total_scans++;
			scan_idx++;
			if (scan_idx >= N_SCANS) {
				//printf("Stepping\n"); //DEBUG
				scan_idx = stepPointmatrix(points, N_SCANS, N_MZPOINTS, RT_step);
				if(scan_idx < 0)
					infox ("Couldn't step matrix.", -4, __FILE__, __LINE__);
				if(scan_idx != stepDoubles(RTs, N_SCANS, RT_step))
					infox ("Couldn't step RTs.", -4, __FILE__, __LINE__);

				//freshenFlags(flags, N_FLAG, total_scans-N_PREV-1, printcolor);
				int last_scan = total_scans - N_PREV - 1;
				if (last_scan > 0) {
					tail = getlatestFlags(flags, N_SCANS, latest);
					if (tail<=0)
						infox("Couldn't update last_seen",-5,__FILE__,__LINE__);
					if (writeoldClusters(points, N_SCANS, N_MZPOINTS, latest, 
						tail, last_scan, RTs, argv[2]) < 0)
						infox("Couldn't write cluster",-6,__FILE__,__LINE__);
					if (clearoldFlags(flags,N_SCANS,latest,tail,last_scan) < 0)
						infox("Couldn't update flags",-7,__FILE__,__LINE__);
				}
				//printf("Stepped\n"); //DEBUG
			}
			mz_idx = 0;
			RTs[scan_idx] = RT;
		}

		if (mz_idx >= N_MZPOINTS)
			infox ("mz_idx out of bounds. Raise N_MZPOINTS.", -3, __FILE__, __LINE__);

		points[scan_idx][mz_idx].mz = mz;
		points[scan_idx][mz_idx].I = I;

		char two_neighbours = 0;
		for (a = scan_idx-1; a >= scan_idx-N_PREV; --a) {
			if (a < 0) break;

			for (b = 0; b < N_MZPOINTS; ++b) {
				if (!points[a][b].mz) break;
				if (points[a][b].mz - mz < -MZ_DIST) continue;
				if (points[a][b].mz - mz >  MZ_DIST) break;

				if (!points[a][b].cluster_flag)
					infox("Neighbour has no cluster!", -10, __FILE__, __LINE__);

				if (!points[scan_idx][mz_idx].cluster_flag) {
					points[a][b].cluster_flag->last_seen = total_scans;
					points[scan_idx][mz_idx].cluster_flag =
						points[a][b].cluster_flag;
				} else {
					*(points[a][b].cluster_flag) =
						*(points[scan_idx][mz_idx].cluster_flag);

					// Check that last_seen is correctly updated
					if (points[a][b].cluster_flag->last_seen != total_scans ||
						points[scan_idx][mz_idx].cluster_flag->last_seen !=
						total_scans)
						infox("last_seen did not update on cluster merge", -10,
								__FILE__, __LINE__);

					two_neighbours = 1;
					break;
				}
			}
			if (two_neighbours) break;
		}
		if (!points[scan_idx][mz_idx].cluster_flag) {
			points[scan_idx][mz_idx].cluster_flag = &flags[current_flag];
			flags[current_flag].last_seen = total_scans;
			current_flag = getnextFlag(flags, N_FLAG, current_flag);
			if (current_flag < 0)
				infox("Out of cluster flags. Increase N_FLAG in cluster.h",-3,
						__FILE__, __LINE__);
		}
		mz_idx++;
	}
	// Output remaining clusters
	//freshenFlags(flags, N_FLAG, total_scans+(2*N_PREV), printcolor);
	tail = getlatestFlags(flags, N_SCANS, latest);
	if (tail<=0)
		infox("Couldn't update last_seen",-5,__FILE__,__LINE__);
	if (writeoldClusters(points, N_SCANS, N_MZPOINTS, latest, tail, 
		(total_scans + N_PREV + 1), RTs, argv[2]) < 0)
		infox("Couldn't write cluster",-6,__FILE__,__LINE__);

	// printf ("Number of cluster flags used: %d\n", current_flag); //DEBUG
	fclose(infile);
	freePointmatrix(points);
	return 0;
}
