// clm.h

#define MZ_DIST 0.05
#define I_MIN 5

#define N_SCANS 500
#define N_MZPOINTS 20000
#define N_FLAG 100000
#define N_PREV 3
#define MIN_CLUSTER_SIZE 20

typedef struct {
	int color, last_seen;
} Flag;

typedef struct {
	double mz, I;
	Flag *cluster_flag; // Pointer to cluster point belongs to (NULL if none)
} Point;

int infox (const char*, int, const char*, int);

int getnextFlag(const Flag*, int, int);
int getlatestFlags(const Flag*, int, Flag*);
int writeoldClusters(Point**, int, int, const Flag*, int, int, const double*, 
						const char*);
int clearoldFlags(Flag*, int, const Flag*, int, int);
// int freshenFlags(Flag *, int, int, int (*output)(Flag));

Point** Pointmatrix(int, int);
void freePointmatrix(Point**);
int stepPointmatrix(Point**, int, int, int);
int stepDoubles(double*, int, int);
