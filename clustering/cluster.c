//      cluster.c

#include <stdio.h>

#define BUFLEN 300
#define RT_INC 50
#define MZI_INC 100

typedef struct {
	float mz, I;
} mzI_32;

typedef struct {
	float RT;
	unsigned int len, alloc;
	mzI_32 mzIs[];
} Scan;

typedef struct {
	unsigned int len, alloc;
	Scan *scans[];
} Spectrum;

void addpointtoSpectrum(Spectrum *sp, float RT, float mz, float I) {
	Scan *scan = bsearch(&RT, sp.scans, sp.len, sizeof(Scan*), compRT);
	
	if (scan == NULL) {
		if (sp.len + 1 >= alloc) {
			alloc += RT_INC;
			realloc(sp.scans,sp.alloc*sizeof(Scan*));
		}
		sp.scans[sp.len] = malloc
	}
}

int compRT ( const void * pkey, const void * pelem ) {
	if (*(float*)pkey > *pelem->RT) return 1;
	if (*(float*)pkey < *pelem->RT) return -1;
	return 0;
}

int main(int argc, char** argv)
{
	FILE *infile;
	char line [BUFLEN] = {'\0'};
	float RT, mz, I;
	Spectrum spectrum;
	
	infile = openfile(argv[1],"r");
	spectrum.len = 0;
	spectrum.alloc = 0;
	
	while(fgets(line,BUFLEN,infile)!=NULL) {
		int ret = sscanf (line, " %*d  %lf  %lf  %lf", &RT, &mz, &I);
		addpointtoSpectrum(&spectrum, RT, mz, I);
	}
    
	return 0;
}

