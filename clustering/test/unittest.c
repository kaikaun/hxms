// unittest.c

#include <stdio.h>
#include "../cluster.h"

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
			infox(errbuf, -1);
		}
	} else {
		// Check for success
		if (!mtx) {
			sprintf(errbuf, "Pointmatrix(%d,%d) failed", rows, cols);
			infox(errbuf, -2);
		}

		// Check row spacing
		for (a=1; a<rows; ++a)
			if (mtx[a]-mtx[a-1] != cols)
				infox("Matrix rows wrong distance apart", -3);

		// Check zeroing
		for (a=0; a<rows; ++a)
			for (b=0; b<cols; ++b)
				if(mtx[a][b].mz || mtx[a][b].I || mtx[a][b].cluster_flag)
					infox("New matrix not zeroed", -4);

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
			sprintf(errbuf, "step = %d, rows = %d succeeded when it should fail", step, rows);
			infox(errbuf, -5);
		}
	} else {
		// Check for success
		if (row_idx < 0) {
			sprintf(errbuf, "step = %d, rows = %d failed", step, rows);
			infox(errbuf, -6);
		}

		// Check return value
		if (row_idx != rows-step) {
			sprintf(errbuf, "step = %d, rows = %d returned row %d instead of %d", step, rows, row_idx, rows-step);
			infox (errbuf, -7);
		}

		for (a=0; a<rows; ++a) {
			for (b=0; b<cols; ++b) {
				if (a < rows - step) {
					// Check moved area is correct
					if (mtx[a][b].mz != a+step || mtx[a][b].I != b)
						infox("step did not move data correctly", -7);
					if (mtx[a][b].cluster_flag)
						infox("step did not move data correctly", -7);
				} else {
					// Check new area is zeroed
					if (mtx[a][b].mz || mtx[a][b].I || mtx[a][b].cluster_flag)
						infox("step did not zero new space A", -8);
				}
			}
		}
	}
	printf("stepPointmatrix(m,%d,%d,%d) passed\n",rows,cols,step);
}

int main(int argc, char** argv)
{
	Point **points;
	int rows = 50, cols = 100;

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

	printf("All tests passed\n");
	freePointmatrix(points);
	return 0;
}
