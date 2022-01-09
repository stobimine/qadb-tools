
#include <cstdlib>
#include <getopt.h>

#define MAXINLEN 1024

#include "util.h"

int debug = 0;

char * aligntype[] = { "INS", "DEL", "SUB", "COR", NULL };

int main(int argc, char ** argv)
{
  int i,j,n,m;
  string utterid;
  string content;
  map<string,string> ref;
  map<string,string> hyp;
  char input[MAXINLEN+1];
  vector<string> utteridlist;
  vector<string> refvec;
  vector<string> hypvec;
  vector<AlignElement> alpath;

  istream * reffd = NULL;
  istream * hypfd = NULL;

  // variables for commandline parsing with getopt
  int           opt;
  extern char * optarg;
  extern int    optind, optopt;

  // parse commandline
  if (argc > 1) {
    while ((opt = getopt(argc, argv, "r:h:d")) != -1) {
      switch(opt) {
      case 'r':
	reffd = new ifstream(optarg);
        break;
      case 'h':
	hypfd = new ifstream(optarg);
	break;
      case 'd':
        debug = 5;
	break;
      default:
	cerr << "Usage: -r <ref-file> -h <hyp-file> [-d]" << endl;
      }
    }
  } else {
    return EXIT_FAILURE;
  }

  if (reffd != NULL and hypfd != NULL) {
    while (!reffd->eof() and !hypfd->eof()) {
      reffd->getline(&input[0], MAXINLEN);
      utterid = string(&input[0]);
      reffd->getline(&input[0], MAXINLEN);
      content = string(&input[0]);
      ref[utterid] = content;
      hypfd->getline(&input[0], MAXINLEN);
      utterid = string(&input[0]);
      hypfd->getline(&input[0], MAXINLEN);
      content = string(&input[0]);
      hyp[utterid] = content;
      if (find(utteridlist.begin(),utteridlist.end(),utterid) == utteridlist.end()) {
	utteridlist.push_back(utterid);
      }
    }
    n = utteridlist.size();
    for (i=0; i<n; i++) {
      refvec = split(ref[utteridlist[i]].c_str(),' ');
      hypvec = split(hyp[utteridlist[i]].c_str(),' ');
      alpath = alignment(refvec, hypvec);
      m = alpath.size();
      cerr << i+1 << " " << utteridlist[i] << endl;
      for (j=0;j<m;j++) {
	cerr << j+1 << " " << aligntype[(UINT)alpath[j].m_type] << " " << refvec[alpath[j].m_ref] << " " << hypvec[alpath[j].m_hyp] << endl;
      }
    }
    return EXIT_SUCCESS;
  } else {
    return EXIT_FAILURE;
  }
}
