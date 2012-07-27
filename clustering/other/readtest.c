//      readtest.c

#include <stdio.h>

#define BUFLEN 300

int main(int argc, char** argv)
{
	FILE *infile;
	char line [BUFLEN] = {'\0'};
	
	infile = fopen(argv[1],"r");
	while(fgets(line,BUFLEN,infile)!=NULL) {
		double RT, mz, I;
		sscanf (line, " %*d  %lf  %lf  %lf", &RT, &mz, &I);
	}
	
	return 0;
}
