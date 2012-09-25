//  mxmlmzXML.c
//
//  Copyright 2012 David Khoo <davidk@bii.a-star.edu.sg>
//
//  Functions for loading mzXML files in mxml

#define B64ENCODEMAXDESTLENGTH(n) ((n)*2)
#define B64DECODEMAXDESTLENGTH(n) ((n)*3/4+1)

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe32(x) OSSwapHostToBigInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#else
#include <endian.h>
#endif

#include <stdint.h>
#include <assert.h>
#include "mxmlmzXML.h"
#include "b64/cdecode.h"
#include "b64/cencode.h"
#include "easyzlib.h"

// Custom mzXML data loading callback function
mxml_type_t mzXML_load_cb(mxml_node_t *node) {
	char *name = (char *)mxmlGetElement(node);

	if (!strcmp(name, "peaks")) return (MXML_CUSTOM);
	else return (MXML_OPAQUE);
}

// Custom mzXML peak data load function
int mzXML_load_custom(mxml_node_t *node, const char *data) {
	mxml_node_t *parent;
	char *decoded;
	long length;
	base64_decodestate state;

	parent = mxmlGetParent(node);
	decoded = malloc(B64DECODEMAXDESTLENGTH(strlen(data)));
	base64_init_decodestate(&state);
	length = base64_decode_block(data, strlen(data), (char *)decoded, &state);
	decoded = realloc(decoded, length);

	// Decompress zlib compressed peak lists
	if (!strcmp(mxmlElementGetAttr(parent,"compressionType"),"zlib")) {
		unsigned char *decomped;
		long pnDestLen;

		pnDestLen = atoi(mxmlElementGetAttr(parent,"compressedLen"));
		decomped = malloc(pnDestLen);
		if(ezuncompress(decomped, &pnDestLen, (unsigned char *)decoded, length) < 0) {
			fprintf(stderr,"zlib decompression failed.\n");
			return (-1);
		}
		length = pnDestLen;
		free(decoded);
		decoded = (char *)decomped;
	}

	// Convert to host byteorder and sort by m/z
	if (!strcmp(mxmlElementGetAttr(parent,"precision"),"32")) {
		uint32_t *u32 = (uint32_t *)decoded;
		assert(0 == length % 8); // Whole number of pairs?
		for(int i = 0; i < length/4; ++i) u32[i] = be32toh(u32[i]);
	} else {
		uint64_t *u64 = (uint64_t *)decoded;
		assert(0 == length % 16); // Whole number of pairs?
		for(int i = 0; i < length/8; ++i) u64[i] = be64toh(u64[i]);
	}

	mxmlSetCustom(node,decoded,mzXML_destroy_custom);
	mxmlElementSetAttrf(parent,"compressedLen","%lu",length);
	
	return (0);
}

// Custom mzXML peak data destructor function
void mzXML_destroy_custom(void *data) {
	free(data);
}

// Custom mzXML peak data save function
char *mzXML_save_custom(mxml_node_t *node) {
	mxml_node_t *parent;
	char *coded, *decoded;
	long length;
	base64_encodestate state;

	parent = mxmlGetParent(node);
	length = atoi(mxmlElementGetAttr(parent,"compressedLen"));
	decoded = (char *)mxmlGetCustom(node);
	
	int compress_peaks = 0;
	if (!strcmp(mxmlElementGetAttr(parent,"compressionType"),"zlib"))
		compress_peaks = 1;

	// Convert to network (big-endian) byteorder
	if (!strcmp(mxmlElementGetAttr(parent,"precision"),"32")) {
		uint32_t *u32 = (uint32_t *)decoded;
		for(int i = 0; i < length/4; ++i) u32[i] = htobe32(u32[i]);
	} else {
		uint64_t *u64 = (uint64_t *)decoded;
		for(int i = 0; i < length/8; ++i) u64[i] = htobe64(u64[i]);
	}

	// Compress peak list using zlib
	if (compress_peaks) {
		long pnDestLen = EZ_COMPRESSMAXDESTLENGTH(length);
		unsigned char *comped = malloc(pnDestLen);
		if(ezcompress(comped,&pnDestLen,(unsigned char*)decoded,length) < 0) {
			fprintf(stderr,"zlib compression failed.\n");
			exit( 130 );
		}
		comped = realloc(comped,pnDestLen);
		length = pnDestLen;
		decoded = (char *)comped;
	}
	
	// Base64 encode peak list
	coded = malloc(B64ENCODEMAXDESTLENGTH(length));
	base64_init_encodestate(&state);
	length = base64_encode_block(decoded, length, coded, &state);
	length += base64_encode_blockend(coded+length, &state);
	coded = realloc(coded, length);

	if (compress_peaks) free(decoded);
	return (coded);
}

// mzXML whitespace callback function
const char *mzXML_whitespace_cb(mxml_node_t *node, int where) {
	switch (where) {
		case MXML_WS_BEFORE_OPEN:
		case MXML_WS_BEFORE_CLOSE:
			return (NULL);
		case MXML_WS_AFTER_OPEN:
		case MXML_WS_AFTER_CLOSE:
			return ("\n");
		default:
			// Serious API error!
			fprintf(stderr, "Callback state invalid!");
			exit( 129 );
	}
}

// Converts XML xs:duration strings to seconds (without validation)
double xsduration_to_s(const char *str) {
#define MINUTE_S 60
#define HOUR_S   60*MINUTE_S
#define DAY_S    24*HOUR_S
#define MONTH_S  30*DAY_S // 30 day months
#define YEAR_S   12*MONTH_S // 360 day years

	double s = 0;
	int n = 0; // start of number substring
	int t = 0; // passed T in string?

	for (int i = 1; i < strlen(str); ++i) { // skip first P character
		switch (str[i]) {
			case 'T':
				t = 1; // Passed T
				n = i + 1;
				break;
			case 'S':
				s += atof(&str[n]);
				n = i + 1;
				break;
			case 'M':
				if (t) s += atof(&str[n]) * MINUTE_S;
				else   s += atof(&str[n]) * MONTH_S;
				n = i + 1;
				break;
			case 'H':
				s += atof(&str[n]) * HOUR_S;
				n = i + 1;
				break;
			case 'D':
				s += atof(&str[n]) * DAY_S;
				n = i + 1;
				break;
			case 'Y':
				s += atof(&str[n]) * YEAR_S;
				n = i + 1;
				break;
			default:
				break;
		}
	}
	return s;
}

// Attempt to open file, reporting and aborting on error
FILE *openfile(const char *filename, const char *mode) {
	FILE *handle;
	
	handle = fopen(filename,mode);
	if (!handle) {
		fprintf(stderr,"Could not open %s\n",filename);
		exit(8);
	}
	return handle;
}
