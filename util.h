
#ifndef _UTIL_H_
#define _UTIL_H_

#include "typedefs.h"
#include <cstring>
#include <cstdlib>
#include <cassert>

#include <fstream>
#include <iostream>

#include <string>
#include <vector>
#include <list>

#include <map>
#include <ext/hash_map>

using namespace std;

typedef enum { ALIGN_INS, ALIGN_DEL, ALIGN_SUB, ALIGN_COR } EAlignType;

typedef struct {
  EAlignType m_type;
  UINT       m_ref;
  UINT       m_hyp;
} AlignElement;

inline bool iseucmb(UBYTE c);
void indicator(UINT cnt, UINT unit);

/* split string at desired separator character into list of tokens */
vector<string> split(const char * line, const char sep);

/* calculate the minimum edit distance */
UINT minimum_edit_distance(const vector<UINT> a, const vector<UINT> b);

/* compute alignment of two string sequences */
vector<AlignElement> alignment(const vector<UINT> a, const vector<UINT> b);
vector<AlignElement> alignment(const vector<string> a, const vector<string> b);

#endif /* _UTIL_H_ */
