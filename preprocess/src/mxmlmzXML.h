//  mxmlmzXML.h
//
//  Copyright 2012 David Khoo <davidk@bii.a-star.edu.sg>
//
//  Header file for mxmlmzXML.c

#ifndef _MXMLMZXML_H
#define _MXMLMZXML_H

#include "mxml.h"

mxml_type_t mzXML_load_cb(mxml_node_t *);
int mzXML_load_custom(mxml_node_t *, const char *);
void mzXML_destroy_custom(void *);
char *mzXML_save_custom(mxml_node_t *);
const char *mzXML_whitespace_cb(mxml_node_t *, int);
double xsduration_to_s(const char *);

FILE *openfile(const char *,const char *);

#endif /* _MXMLMZXML_H */
