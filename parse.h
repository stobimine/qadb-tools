
#ifndef _PARSE_H_
#define _PARSE_H_

#include "typedefs.h"
#include "util.h"
#include "chasen.h"

typedef struct
{
  string       m_origin;
  string       m_yomi;
  string       m_basis;
  int          m_poscode;
  int          m_conjform;
  int          m_conjtype;
} Morpheme;

typedef vector<Morpheme> Sentence;

void parse_init(const char * cfgfile);

Sentence parse_sentence(const char * input);

#endif /* _PARSE_H_ */
