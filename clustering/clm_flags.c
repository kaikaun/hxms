// clm_flags.c

#include "clm.h"

// Return index to next available flag in an array of flags or <0 for error
int getnextFlag(Flag *flags, int len, int current) {
	int next;

	// Invalid arguments
	if (len<=0) return -1;
	if (current<0) return -1;
	if (current>=len) return -1;

	next = current;
	do {
		if (flags[next].last_seen == -1) return next; // Return available flag
		if (++next >= len) next -= len; // Go to next flag, wrapping around
	} while (next != current); // Stop if we come back to the starting flag
	return -2; // No available flag found
}

// Clear old flags with last_seen < scan from flags[]
// Send old colors and their latest last_seen to output() (0 return for success)
// Return number of cleared flags or <0 for error
int freshenFlags(Flag *flags, int len, int scan, int (*outflag)(Flag oldflag)) {
	Flag new[len];
	int curr_color = -1, tail = 0, cleared = 0;
	int a,b;

	// Invalid arguments
	if (len <= 0) return -1;
	if (scan < 0) return -1;

	// Populate new[] with the latest last_seen for each color and store highest
	// color number in curr_color
	for (a=0; a<len; ++a) {
		if (flags[a].last_seen == -1) continue;
		for (b=0; b<tail; ++b) {
			if (flags[a].color == new[b].color) {
				if (new[b].last_seen < flags[a].last_seen)
					new[b].last_seen = flags[a].last_seen;
				break;
			}
		}
		if (b == tail) {
			new[tail++] = flags[a];
			if (curr_color < flags[a].color) curr_color = flags[a].color;
		}
	}

	// Go through new[] and pass flags with old colors to outflag()
	for (b=0; b<tail; ++b)
		if (new[b].last_seen < scan) {
			int ret = outflag(new[b]);
			if (ret) return -2; // Output of point failed
		}

	// Mark old flags as available and give them unique new colors
	for (a=0; a<len; ++a) {
		if (flags[a].last_seen == -1) continue;
		for (b=0; b<tail; ++b) {
			if (flags[a].color == new[b].color) {
				if (new[b].last_seen < scan) {
					flags[a].color = ++curr_color;
					flags[a].last_seen = -1;
					++cleared;
				}
				break;
			}
		}
	}

	return cleared;
}
