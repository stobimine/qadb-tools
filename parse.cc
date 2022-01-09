
#include "parse.h"
#include <iostream>

extern int debug;

void parse_init(const char * cfgfile)
{
  const char * argv[] = {"-r", cfgfile, NULL};
  chasen_getopt_argv((char **)argv, stdin);
}

Sentence parse_sentence(const char * input)
{
  static int i, n;
  char * buffer;
  Morpheme morph;
  vector<string> lines;
  vector<string> tokens;
  vector<string> parts;
  string line;
  string token;
  Sentence sent;
  
  buffer = chasen_sparse_tostr((char *)input);

  if (buffer != NULL) {
    lines = split(buffer, '\n');
    n = lines.size();
  }

  for (i=0; i<n; i++) {
    tokens = split(lines[i].c_str(), '\t');
    if (tokens.size() == 0 || tokens[0] == string("EOS")) {
      if (debug == 1) cerr << endl;
      break;
    } else if (tokens.size() == 3) {
      morph.m_origin = tokens[0];
      morph.m_yomi   = tokens[0];
      morph.m_basis  = tokens[0];
      parts = split(tokens[2].c_str(),'/');
      if (parts.size() == 3) {
	morph.m_poscode  = atoi(parts[0].c_str());
	morph.m_conjform = atoi(parts[1].c_str());
	morph.m_conjtype = atoi(parts[2].c_str());
      } else {
	morph.m_poscode  = 0;
	morph.m_conjform = 0;
	morph.m_conjtype = 0;
      }
      if (morph.m_poscode <= 75) sent.push_back(morph);
    } else if (tokens.size() == 4) {
      morph.m_origin = tokens[0];
      morph.m_yomi   = tokens[1];
      morph.m_basis  = tokens[2];
      parts = split(tokens[3].c_str(),'/');
      if (parts.size() == 3) {
	morph.m_poscode  = atoi(parts[0].c_str());
	morph.m_conjform = atoi(parts[1].c_str());
	morph.m_conjtype = atoi(parts[2].c_str());
      } else {
	morph.m_poscode  = 0;
	morph.m_conjform = 0;
	morph.m_conjtype = 0;
      }
      if (morph.m_poscode <= 75) sent.push_back(morph);
    }
    if (debug == 1) cerr << tokens[0] << "+" << tokens[1] << "+" << tokens[2] << " ";
  }

  return sent;
}

