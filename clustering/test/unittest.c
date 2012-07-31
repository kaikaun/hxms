// unittest.c

#include <stdio.h>
#include "clm.h"

#define BUFLEN 100

// Tests Pointmatrix
void testPointmatrix(int rows, int cols) {
	char errbuf[BUFLEN];
	Point **mtx;
	int a,b;

	mtx = Pointmatrix(rows,cols);
	if (rows<=0 || cols<=0) {
		// Check for failure on invalid parameters
		if (mtx) {
			sprintf(errbuf, "Pointmatrix(%d,%d) succeeded when it should fail", 
					rows, cols);
			infox(errbuf, -1, __FILE__, __LINE__);
		}
	} else {
		// Check for success
		if (!mtx) {
			sprintf(errbuf, "Pointmatrix(%d,%d) failed", rows, cols);
			infox(errbuf, -2, __FILE__, __LINE__);
		}
		// Check row spacing
		for (a=1; a<rows; ++a)
			if (mtx[a]-mtx[a-1] != cols)
				infox("Matrix rows wrong distance apart", -3,__FILE__,__LINE__);

		// Check zeroing
		for (a=0; a<rows; ++a)
			for (b=0; b<cols; ++b)
				if(mtx[a][b].mz || mtx[a][b].I || mtx[a][b].cluster_flag)
					infox("New matrix not zeroed", -4, __FILE__, __LINE__);

		freePointmatrix(mtx);
	}
	printf("Pointmatrix(%d,%d) passed\n",rows,cols);
}

// Tests stepPointmatrix (matrix must be preallocated)
void teststepPointmatrix(Point** mtx, int rows, int cols, int step) {
	char errbuf[BUFLEN];
	int a,b,row_idx;

	// Fill matrix with test data
	for (a=0; a<rows; ++a) {
		for (b=0; b<cols; ++b) {
			mtx[a][b].mz = a;
			mtx[a][b].I = b;
			mtx[a][b].cluster_flag = NULL;
		}
	}

	row_idx = stepPointmatrix(mtx, rows, cols, step);

	if (step <= 0 || step > rows) {
		// Check for failure on invalid parameters
		if (row_idx >= 0) {
			sprintf(errbuf,"step = %d, rows = %d succeeded when it should fail",
					step, rows);
			infox(errbuf, -5, __FILE__, __LINE__);
		}
	} else {
		// Check for success
		if (row_idx < 0) {
			sprintf(errbuf, "step = %d, rows = %d failed", step, rows);
			infox(errbuf, -6, __FILE__, __LINE__);
		}

		// Check return value
		if (row_idx != rows-step) {
			sprintf(errbuf,"step = %d, rows = %d returned row %d not %d",
					step, rows, row_idx, rows-step);
			infox (errbuf, -7, __FILE__, __LINE__);
		}

		for (a=0; a<rows; ++a) {
			for (b=0; b<cols; ++b) {
				if (a < rows - step) {
					// Check moved area is correct
					if (mtx[a][b].mz != a+step || mtx[a][b].I != b)
						infox("stepPointmatrix did not move data correctly", -8,
								__FILE__, __LINE__);
					if (mtx[a][b].cluster_flag)
						infox("stepPointmatrix did not move data correctly", -8,
								__FILE__, __LINE__);
				} else {
					// Check new area is zeroed
					if (mtx[a][b].mz || mtx[a][b].I || mtx[a][b].cluster_flag)
						infox("stepPointmatrix did not zero new space", -9,
								__FILE__, __LINE__);
				}
			}
		}
	}
	printf("stepPointmatrix(m,%d,%d,%d) passed\n",rows,cols,step);
}

// Tests getnextFlag (flag array must be preallocated)
void testgetnextFlag(Flag *flags, int len, int curr) {
	char errbuf[BUFLEN];
	int next;
	
	if (len <= 0 || curr < 0 || curr >= len ) {
		next = getnextFlag(flags,len,curr);
		// Check for failure on invalid parameters
		if (next >= 0) {
			sprintf(errbuf, "len = %d, curr = %d succeeded when it should fail", 
					len, curr);
			infox(errbuf, -10, __FILE__, __LINE__);
		}
	} else {
		int a;

		// Check for failure when there is no available flag
		for (a=0;a<len;++a) flags[a].last_seen = 0;
		next = getnextFlag(flags,len,curr);
		if (next >= 0)
			infox("getnextFlag succeeded with no available flag", -11,
					__FILE__, __LINE__);

		// Check that an available flag can be found at any position from curr
		for (a=0;a<len;++a) {
			flags[a].last_seen = -1;
			next = getnextFlag(flags,len,curr);
			if (next != a) {
				sprintf(errbuf,"getnextFlag(f,%d,%d) returned %d not %d", 
					len, curr, next, a);
				infox(errbuf, -13, __FILE__, __LINE__);
			}
			flags[a].last_seen = 0;
		}
	}
	printf("getnextFlag(f,%d,%d) passed\n",len,curr);
}

// Dummy output function for freshenFlags that does nothing
int discard(Flag oldflag) {
	return 0;
}

// Tests freshenFlags (flag array must be preallocated)
/* void testfreshenFlags(Flag *flags, int len, int scan) {
	char errbuf[BUFLEN];
	int cleared;
	int a, b;

	if (len <= 0 || scan < 0) {
		cleared = freshenFlags(flags, len, scan, discard);
		// Check for failure on invalid parameters
		if (cleared >= 0) {
			sprintf(errbuf, "len = %d, scan = %d succeeded when it should fail", 
					len, scan);
			infox(errbuf, -14, __FILE__, __LINE__);
		}
	} else {
		// Check for clears or changes when no flags are used
		for (a=0; a<len; ++a) {
			flags[a].color = a;
			flags[a].last_seen = -1;
		}
		cleared = freshenFlags(flags, len, scan, discard);
		if (cleared != 0)
			infox("freshenFlags cleared unused flag", -15, __FILE__, __LINE__);
		for (a=0; a<len; ++a)
			if (flags[a].color != a || flags[a].last_seen != -1)
				infox("freshenFlags changed unused flag",-16,__FILE__,__LINE__);

		// Check for clears or changes when no flags are old
		for (a=0; a<len; ++a) {
			flags[a].color = a;
			flags[a].last_seen = scan;
		}
		cleared = freshenFlags(flags, len, scan, discard);
		if (cleared != 0)
			infox("freshenFlags cleared fresh flag", -15, __FILE__, __LINE__);
		for (a=0; a<len; ++a)
			if (flags[a].color != a || flags[a].last_seen != scan)
				infox("freshenFlags changed fresh flag",-16,__FILE__,__LINE__);

		if (scan > 0) {
			for(b=0;b<len;++b) {
				for (a=0; a<len; ++a) {
					flags[a].color = a;
					if (a<=b) flags[a].last_seen = 0;
					else flags[a].last_seen = scan;
				}
				cleared = freshenFlags(flags, len, scan, discard);
				// Check that old flags are cleared
				if (cleared != b+1) {
					sprintf(errbuf,"freshenFlags cleared %d not %d flags",
							cleared, b+1);
					infox(errbuf, -17, __FILE__, __LINE__);
				}
				// Check that cleared flags are correctly changed
				for (a=0; a<len; ++a) {
					if (a<=b) {
						if (flags[a].color != a+len)
							infox("freshenFlags did not update old flag color",
									-18,__FILE__,__LINE__);
						if (flags[a].last_seen != -1)
							infox("freshenFlags did not set old flag available",
									-19,__FILE__,__LINE__);
					} else {
						if (flags[a].color != a || flags[a].last_seen !=scan)
							infox("freshenFlags changed fresh flag", -16, 
									__FILE__, __LINE__);
					}
				}
			}
		}
	}
	printf("freshenFlags(f,%d,%d,o) passed\n",len,scan);
} */

int main(int argc, char** argv)
{
	int rows = 50, cols = 100, len = 1000;
	Point **points;
	Flag flags[len];

	testPointmatrix(0,0);
	testPointmatrix(-1,0);
	testPointmatrix(-1,-1);
	testPointmatrix(1,0);
	testPointmatrix(0,1);
	testPointmatrix(1,1);
	testPointmatrix(rows,cols);

	points = Pointmatrix(rows, cols);
	teststepPointmatrix(points, rows, cols, -1);
	teststepPointmatrix(points, rows, cols, 0);
	teststepPointmatrix(points, rows, cols, rows+1);
	teststepPointmatrix(points, rows, cols, rows);
	teststepPointmatrix(points, rows, cols, rows-1);
	teststepPointmatrix(points, rows, cols, rows/2);
	teststepPointmatrix(points, rows, cols, 1);
	freePointmatrix(points);

	testgetnextFlag(flags, 0, 0);
	testgetnextFlag(flags, -1, 0);
	testgetnextFlag(flags, 0, -1);
	testgetnextFlag(flags, -1, -1);
	testgetnextFlag(flags, len, -1);
	testgetnextFlag(flags, len, len);
	testgetnextFlag(flags, len, 0);
	testgetnextFlag(flags, len, len-1);

/*	testfreshenFlags(flags, 0, 0);
	testfreshenFlags(flags, -1, 0);
	testfreshenFlags(flags, 0, -1);
	testfreshenFlags(flags, -1, -1);
	testfreshenFlags(flags, len, -1);
	testfreshenFlags(flags, len, 0);
	testfreshenFlags(flags, len, 1); */

	printf("All tests passed\n");
	return 0;
}
