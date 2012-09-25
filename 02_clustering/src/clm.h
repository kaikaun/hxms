// clm.h

#define MZ_DIST 0.02
#define I_MIN 5

#define N_SCANS 600
#define N_MZPOINTS 20000
#define N_FLAG 200000
#define N_PREV 2
#define MIN_CLUSTER_SIZE 20
#define MAX_MERGES 0

typedef struct {
	int color, last_seen;
} Flag;

typedef struct {
	double mz, I;
	Flag *cluster_flag; // Pointer to cluster point belongs to (NULL if none)
} Point;

typedef struct {
	int dim1, dim2, base;
	double *RTs;
	Point **pts;
} Matrix;

int infox (const char*, int, const char*, int);
int appendfile (const char*, const char*);

int getnextFlag(const Flag*, int, int);
int getlatestFlags(const Flag*, int, Flag*);
int freshenFlags(Flag*, int, const Flag*, int);
int writeClusters(Point**,int,int,const Flag*,int,int,const double*,int,int);
int clearoldFlags(Flag*, int, const Flag*, int, int, int);
int mergeColors(Flag*, int, const Flag*, int);

Point** Pointmatrix(int, int);
void freePointmatrix(Point**);
int stepPointmatrix(Point**, int, int, int);
int stepDoubles(double*, int, int);
