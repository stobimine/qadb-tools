
#include "util.h"

extern int debug;

vector<string> split (const char * line, const char sep = ' ')
{
  vector<string> vec;
  UINT           len; 
  UINT           i, j;
  char         * str;
  string         tok;

  i = j = 0;
  len = strlen(line);
  while (i < len) {
    // skip separator(s), skip multi-byte chars
    while(i < len and line[i] == sep) {
      if (iseucmb(UBYTE(line[i])) and i+1<len)
	i += 2;
      else
	i += 1;
    }
    j = i;
    // skip non-separator(s), skip multi-byte chars
    while(j < len and line[j] != sep) {
      if (iseucmb(UBYTE(line[j])) and j+1<len)
	j += 2;
      else
	j += 1;
    }
    if (j-i > 0) {
      str = static_cast<char *>(malloc(j-i+1));
      memcpy(str, &line[i], j-i);
      str[j-i] = '\0';
      tok.assign(str);
      vec.push_back(tok);
      free(str);
      i = j;
    }
  }

  return vec;
}

inline bool iseucmb(UBYTE c)
{
  return c >= 161 and c <= 254;
}

UINT minimum_edit_distance(const vector<UINT> a, const vector<UINT> b)
{
  UINT ** matrix;
  UINT    n = a.size();
  UINT    m = b.size();
  UINT    i,j,d,v,w;

  // allocate memory
  matrix = (UINT **) malloc (sizeof(UINT) * (n+1));
  for (i=0;i<=n;i++) {
    matrix[i] = (UINT *) calloc ((m+1), sizeof(UINT));
  }
  // DP search
  for (i=1; i<=n; i++) {
    matrix[i][0] = i;
  }
  for (j=1; j<=m; j++) {
    matrix[0][j] = j;
  }
  matrix[0][0] = 0;
  for (i=1; i<=n; i++) {
    for (j=1; j<=m; j++) {
      if (a[i-1] == b[j-1]) {
	d = 1;
      } else {
	d = 0;
      }
      v = matrix[i-1][j]+1;
      w = matrix[i][j-1]+1;
      if (w < v) v = w;
      w = matrix[i-1][j-1]+1-d;
      if (w < v) v = w;
      matrix[i][j] = v;
    }
  }
  // debug info
  if (debug == 5) {
    for (i=0; i<=n; i++) {
      for (j=0; j<=m; j++)
        cerr << " " << matrix[i][j];
      cerr << endl;
    }
  }
  // get distance value from matrix
  d = matrix[n][m];
  // free memory
  for (i=0;i<=n;i++) free(matrix[i]);
  free(matrix);
  
  return d;
}

vector<AlignElement> alignment(const vector<UINT> a, const vector<UINT> b)
{
  UINT ** matrix;
  EAlignType ** path;
  UINT    n = a.size();
  UINT    m = b.size();
  UINT    i,j,d,v,w;
  AlignElement el;
  vector<AlignElement> al;

  // allocate memory
  matrix = (UINT **) malloc (sizeof(UINT) * (n+1));
  path = (EAlignType **) malloc(sizeof(EAlignType) * (n+1));
  for (i=0;i<=n;i++) {
    matrix[i] = (UINT *) calloc ((m+1), sizeof(UINT));
    path[i] = (EAlignType *) calloc((m+1), sizeof(EAlignType));
  }
  // DP search
  for (i=1; i<=n; i++) {
    matrix[i][0] = i;
    path[i][0] = ALIGN_INS;
  }
  for (j=1; j<=m; j++) {
    matrix[0][j] = j;
    path[0][j] = ALIGN_DEL;
  }
  matrix[0][0] = 0;
  path[0][0] = ALIGN_COR;
  for (i=1; i<=n; i++) {
    for (j=1; j<=m; j++) {
      if (a[i-1] == b[j-1]) {
	d = 1;
      } else {
	d = 0;
      }
      v = matrix[i-1][j]+1;
      w = matrix[i][j-1]+1;
      if (w < v) { 
	v = w;
	path[i][j] = ALIGN_INS;
      } else {
	path[i][j] = ALIGN_DEL;
      }
      w = matrix[i-1][j-1]+1-d;
      if (w < v) {
	v = w;
        if (d == 1)
	  path[i][j] = ALIGN_COR;
        else
	  path[i][j] = ALIGN_SUB;
      }
      matrix[i][j] = v;
    }
  }
  // debug info
  if (debug == 5) {
    for (i=0; i<=n; i++) {
      for (j=0; j<=m; j++)
        cerr << " " << matrix[i][j];
      cerr << endl;
    }
  }
  // determine best alignment path
  v = n; w = m;
  while (v != 0 and w != 0) {
    el.m_ref  = v-1;
    el.m_hyp  = w-1;
    el.m_type = path[v][w];
    switch(path[v][w]) {
    case ALIGN_DEL:
      v = v-1;
      break;
    case ALIGN_INS:
      w = w-1;
      break;
    case ALIGN_SUB:
    case ALIGN_COR:
      v = v-1;
      w = w-1;
      break;
    default:
      v = 0;
      w = 0;
      break;
    }
    al.insert(al.begin(), el);
  }
  // free memory
  for (i=0;i<=n;i++) {
    free(matrix[i]);
    free(path[i]);
  }
  free(matrix);
  free(path);
  
  return al;
}

vector<AlignElement> alignment(const vector<string> a, const vector<string> b)
{
  UINT ** matrix;
  EAlignType ** path;
  UINT    n = a.size();
  UINT    m = b.size();
  UINT    i,j,d,v,w;
  AlignElement el;
  vector<AlignElement> al;

  // allocate memory
  matrix = (UINT **) malloc(sizeof(UINT) * (n+1));
  path = (EAlignType **) malloc(sizeof(EAlignType) * (n+1));
  for (i=0;i<=n;i++) {
    matrix[i] = (UINT *) calloc((m+1), sizeof(UINT));
    path[i] = (EAlignType *) calloc((m+1), sizeof(EAlignType));
  }
  // DP search
  for (i=1; i<=n; i++) {
    matrix[i][0] = i;
    path[i][0] = ALIGN_INS;
  }
  for (j=1; j<=m; j++) {
    matrix[0][j] = j;
    path[0][j] = ALIGN_DEL;
  }
  matrix[0][0] = 0;
  path[0][0] = ALIGN_COR;
  for (i=1; i<=n; i++) {
    for (j=1; j<=m; j++) {
      if (a[i-1] == b[j-1]) {
	d = 1;
      } else {
	d = 0;
      }
      v = matrix[i-1][j]+1;
      w = matrix[i][j-1]+1;
      if (w < v) { 
	v = w;
	path[i][j] = ALIGN_INS;
      } else {
	path[i][j] = ALIGN_DEL;
      }
      w = matrix[i-1][j-1]+1-d;
      if (w < v) {
	v = w;
        if (d == 1)
	  path[i][j] = ALIGN_COR;
        else
	  path[i][j] = ALIGN_SUB;
      }
      matrix[i][j] = v;
    }
  }
  // debug info
  if (debug == 5) {
    for (i=0; i<=n; i++) {
      for (j=0; j<=m; j++)
        cerr << " " << matrix[i][j];
      cerr << endl;
    }
  }
  // determine best alignment path
  v = n; w = m;
  while (v != 0 and w != 0) {
    el.m_ref  = v-1;
    el.m_hyp  = w-1;
    el.m_type = path[v][w];
    switch(path[v][w]) {
    case ALIGN_DEL:
      v = v-1;
      break;
    case ALIGN_INS:
      w = w-1;
      break;
    case ALIGN_SUB:
    case ALIGN_COR:
      v = v-1;
      w = w-1;
      break;
    default:
      v = 0;
      w = 0;
      break;
    }
    al.insert(al.begin(), el);
  }
  // free memory
  for (i=0;i<=n;i++) {
    free(matrix[i]);
    free(path[i]);
  }
  free(matrix);
  free(path);
  
  return al;
}

void indicator(UINT cnt, UINT unit)
{
  if (unit == 0) {
    cerr << " " << cnt << endl;
  } else {
    if (cnt != 0) {
      if (cnt % unit == 0) cerr << ".";
      if (cnt % (10*unit) == 0) cerr << " ";
      if (cnt % (50*unit) == 0) cerr << cnt << endl;
    }
  }
}

