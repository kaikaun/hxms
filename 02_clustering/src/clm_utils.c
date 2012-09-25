// clm_utils.c

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "clm.h"

// Print error message and abort program with error exit value
int infox (const char *errmsg, int exitval, const char *file, int line) {
	fprintf(stderr, "%s\nExiting at %s:%d.\n", errmsg, file, line);
	exit(exitval);
}

// Append one file to another, return bytes written or <0 on error
int appendfile (const char *in_fn, const char *out_fn) {
	if (!in_fn || !out_fn) return -1;
	int written = 0;

	int in_fd = open(in_fn, O_RDONLY);
	if (in_fd<0) infox("Couldn't open append source",-50,__FILE__,__LINE__);
	int out_fd = open(out_fn, O_WRONLY);
	if (out_fd<0) infox("Couldn't open append target",-51,__FILE__,__LINE__);
	char buf[8192];

	while (1) {
		ssize_t result = read(in_fd, &buf[0], sizeof(buf));
		if (!result) break;
		if (result<0) infox("Couldn't read append source",-52,__FILE__,__LINE__);
		if (write(out_fd, &buf[0], result) != result)
			infox("Couldn't write append target",-53,__FILE__,__LINE__);
		written += result;
	}

	close(in_fd);
	close(out_fd);
	return written;
}
