// clm_utils.c

#include <stdio.h>
#include <stdlib.h>
#include "clm.h"

// Print error message and abort program with error exit value
int infox (const char *errmsg, int exitval, const char *file, int line) {
	fprintf(stderr, "%s\nExiting at %s:%d.\n", errmsg, file, line);
	exit(exitval);
}
