// clm_main.c

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include "clm.h"

#define BUFLEN 300

int main(int argc, char** argv)
{
	char line [BUFLEN] = {'\0'};
	int a, b;

	FILE *infile;
	Point **points;
	double RTs[N_SCANS] = {0};
	Flag flags[N_FLAG], latest[N_FLAG];

	int scan_idx = -1, mz_idx = 0, current_flag = 0, scan_base = 0;
	int curr_color = 0, RT_step = N_SCANS/3;
	//int line_ctr = 0; //DEBUG

	// Check command line arguments, print usage if wrong
	if ( argc < 3 ) {
		fprintf (stderr, "Usage: %s <input table> <output dir>\n", argv[0]);
		exit (1);
	}

	// Ensure output dir does not already exist
	struct stat st;
	if (stat(argv[2],&st) == 0) {
		sprintf(line, "Output dir %s already exists", argv[2]);
		infox(line, -2, __FILE__, __LINE__);
	}

	// Create output dir
	if (mkdir(argv[2], (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) != 0)
		infox("Creation of output dir failed", -2, __FILE__, __LINE__);
	stat(argv[2],&st);
	if (!S_ISDIR(st.st_mode))
		infox("Creation of output dir failed", -2, __FILE__, __LINE__);

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
		flags[a].color = curr_color++;
		flags[a].last_seen = -1;
	}

	// Store current dir and change to output dir
	char *cwd = getcwd(NULL,0);
	if (!cwd) infox("Couldn't getcwd!",-255,__FILE__,__LINE__);
	if (chdir(argv[2]) == -1) infox("Couldn't chdir!",-254,__FILE__,__LINE__);

	while(fgets(line,BUFLEN,infile)!=NULL) {
		double RT, mz, I;
		int ret = sscanf (line, " %*d  %lf  %lf  %lf", &RT, &mz, &I);

		if (ret != 3) continue; //should we warn the user?
		if (I < I_MIN) continue;

		//if (!(++line_ctr%1000))printf("line %d, scan %d, RT %f, %d points\n",
		//			line_ctr,scan_base+scan_idx,RTs[scan_idx],mz_idx); //DEBUG

		// Assume that points in CSV are already sorted by RT, and move to next
		// scan if RT changes, stepping if necessary
		if (RT != RTs[scan_idx]) {
			scan_idx++;
			if (scan_idx >= N_SCANS) {
				int last_scan = scan_base + scan_idx - N_PREV - 2;
				if (last_scan > 0) {
					// Write out old clusters
					int tail = getlatestFlags(flags, N_FLAG, latest);
					if (tail <= 0)
						infox("Couldn't get latest flags",-5,__FILE__,__LINE__);
					if (freshenFlags(flags,N_FLAG,latest,tail) < 0)
						infox("Couldn't freshen flags",-5,__FILE__,__LINE__);
					if (writeClusters(points,N_SCANS,N_MZPOINTS,latest,tail,
						last_scan,RTs,scan_base,MIN_CLUSTER_SIZE) < 0)
						infox("Couldn't write cluster",-6,__FILE__,__LINE__);

					// Clear old flags
					curr_color=clearoldFlags(flags,N_FLAG,latest,tail,last_scan,
												curr_color);
					if (curr_color < 0)
						infox("Couldn't update flags",-7,__FILE__,__LINE__);

					// Write out the parts of current clusters that would be
					// lost on stepping (i.e. long clusters)
					if (writeClusters(points,RT_step,N_MZPOINTS,latest,tail,
						(scan_base+scan_idx+1),RTs,scan_base,0) < 0)
						infox("Couldn't write cluster",-6,__FILE__,__LINE__);

					// Update current_flag
					current_flag = getnextFlag(flags, N_FLAG, current_flag);
					if (current_flag < 0)
						infox("Out of cluster flags. Increase N_FLAG.", -3,
								__FILE__, __LINE__);
				}

				scan_idx = stepPointmatrix(points, N_SCANS, N_MZPOINTS,RT_step);
				if(scan_idx < 0)
					infox ("Couldn't step matrix.", -4, __FILE__, __LINE__);
				if(scan_idx != stepDoubles(RTs, N_SCANS, RT_step))
					infox ("Couldn't step RTs.", -4, __FILE__, __LINE__);
				scan_base += RT_step;
			}
			mz_idx = 0;
			RTs[scan_idx] = RT;
		} else {
			mz_idx++;
			if (mz_idx >= N_MZPOINTS)
				infox ("mz_idx out of bounds. Raise N_MZPOINTS.", -3, __FILE__,
						__LINE__);
		}

		points[scan_idx][mz_idx].mz = mz;
		points[scan_idx][mz_idx].I = I;

		int merges = 0;
		for (a = scan_idx-1; a >= scan_idx-N_PREV ; --a) {
			if (a < 0) break;

			for (b = 0; b < N_MZPOINTS; ++b) {
				if (!points[a][b].mz) break;
				if (points[a][b].mz - mz < -MZ_DIST) continue;
				if (points[a][b].mz - mz >  MZ_DIST) break;

				if (!points[a][b].cluster_flag)
					infox("Neighbour has no cluster!", -10, __FILE__, __LINE__);

				if (!points[scan_idx][mz_idx].cluster_flag) {
					points[a][b].cluster_flag->last_seen = scan_base + scan_idx;
					points[scan_idx][mz_idx].cluster_flag =
						points[a][b].cluster_flag;
				} else {
					if (points[scan_idx][mz_idx].cluster_flag->color !=
						points[a][b].cluster_flag->color) {
						// Merge clusters
						// Check if file for old color exists
						char old_fn[13];
						sprintf(old_fn,"%06d.clust",
								points[a][b].cluster_flag->color);
						struct stat st;
						if (!stat(old_fn,&st)) {
							char new_fn[13];
							sprintf(new_fn,"%06d.clust",
									points[scan_idx][mz_idx].cluster_flag->color);
							if (stat(new_fn,&st)) {
							// If file for new color does not exist, just rename
							printf("Renaming %s to %s\n",old_fn,new_fn);//DEBUG
								if(rename(old_fn,new_fn)) {
									sprintf(line,"Could not rename %s to %s\n",
											old_fn,new_fn);
									infox(line,-40,__FILE__,__LINE__);
								}
							} else {
							// Otherwise append old file to new file then delete
							printf("Appending %s to %s\n",old_fn,new_fn);//DEBUG
								if (appendfile(old_fn,new_fn) < 0) {
									sprintf(line,"Could not append %s to %s\n",
											old_fn,new_fn);
									infox(line,-41,__FILE__,__LINE__);
								}
								remove(old_fn);
							}
						}

						if (mergeColors(flags,N_FLAG,
							points[scan_idx][mz_idx].cluster_flag,
							points[a][b].cluster_flag->color) < 0)
							infox("Could not merge",-8,__FILE__,__LINE__);
						if (++merges > MAX_MERGES) break;
					}
				}
			}
			if (merges > MAX_MERGES) break;
		}
		if (!points[scan_idx][mz_idx].cluster_flag) {
			points[scan_idx][mz_idx].cluster_flag = &(flags[current_flag]);
			flags[current_flag].last_seen = scan_base+scan_idx;
			current_flag = getnextFlag(flags, N_FLAG, current_flag);
			if (current_flag < 0)
				infox("Out of cluster flags. Increase N_FLAG.",-3,
						__FILE__, __LINE__);
		}
	}

	// Output remaining clusters
	tail = getlatestFlags(flags, N_FLAG, latest);
	if (tail < 0)
		infox("Couldn't get latest flags",-5,__FILE__,__LINE__);
	else if (tail > 0)
		if (freshenFlags(flags,N_FLAG,latest,tail) < 0)
			infox("Couldn't freshen flags",-5,__FILE__,__LINE__);
		if (writeClusters(points,N_SCANS,N_MZPOINTS,latest,tail,
			(scan_base+scan_idx+1),RTs,scan_base,MIN_CLUSTER_SIZE) < 0)
			infox("Couldn't write cluster",-6,__FILE__,__LINE__);

	// printf ("Number of cluster flags used: %d\n", current_flag); //DEBUG
	fclose(infile);
	freePointmatrix(points);
	if (chdir(cwd) == -1)
		infox("Couldn't chdir!",-254,__FILE__,__LINE__);
	free(cwd);
	return 0;
}
