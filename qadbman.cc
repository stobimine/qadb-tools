/* ---------------------------------------------- --*-c++-*--
 * 
 *  Question and Answer Database Management Tool
 * 
 *  Copyright (c) 2006 Nara Institute of Science and Technology
 *
 *  1st Author: Tobias Cincarek
 *
 *  All Rights Reserved.
 * 
 * ---------------------------------------------------------- */

#include "qadbman.h"

int debug = 0;

int main(int argc, char ** argv)
{
  QADB *     mydb = NULL;
  QAPair     qapair;
  char       input[MAXINLEN+1];
  bool       optimize  = false;
  bool       validate  = false;
  bool       looeval   = false;
  bool       labelmode = false;
  bool       loocvopt  = false;
  bool       cvopt     = false;
  istream *  infile = &cin;
  ostream *  outfile = &cout;
  UINT       iocnt = 0;
  UINT       heapsize = 100;
  MatchMode  matchmode = MATCH_MAXLEN;
  SimOp      simop = SO_COSINUS;

  // variables for commandline parsing with getopt
  int           opt;
  extern char * optarg;
  extern int    optind, optopt;

  // variables for commandline arguments
  const char *  respfile = NULL;
  const char *  qadbfile = NULL;
  const char *  targetfile = NULL;
  const char *  chacfgfile = NULL;
  const char *  morphtable = NULL;
  const char *  stopwlist = NULL;
  int   nbestout = 0;
  int   optiter = 0;

  // parse commandline
  if (argc > 1) {
    while ((opt = getopt(argc, argv, "g:k:b:x:t:c:r:q:a:i:o:m:n:sfdvehpu")) != -1) {
      switch(opt) {
      case 'u':
        // unsupervised labeling of queries
	labelmode = true;
	break;
      case 'g':
        // debug mode
        debug = atoi(optarg);
        break;
      case 'h':
	// help message
	help(argv[0]);
	return EXIT_SUCCESS;
	break;
      case 'c':
	// chasen config file
	chacfgfile = optarg;
	break;
      case 't':
	// morpheme confusion table
	morphtable = optarg;
	matchmode = MATCH_CONF;
	break;
      case 'x':
	// file with stopword list
	stopwlist = optarg;
	break;
      case 'q':
	// file with input queries
	infile = new ifstream(optarg);
	break;
      case 'a':
	// file for retrieval results
	outfile = new ofstream(optarg);
	break;
      case 'r':
	// file with response sentences
	respfile = optarg;
	break;
      case 'i':
	// input file with Q&A pairs
	qadbfile = optarg;
	break;
      case 'o':
	// output file with optimized Q&A pairs
	targetfile = optarg;
	break;
      case 'n':
	// n-best response output
	nbestout = atoi(optarg);
	break;
      case 'k':
	heapsize = atoi(optarg);
	break;
      case 'm':
	// match mode
	switch(atoi(optarg)) {
	case 1:
	  matchmode = MATCH_EXLEN;
	  break;
	case 2:
	  matchmode = MATCH_INLEN;
	  break;
	case 3:
	  matchmode = MATCH_MAXLEN;
	  break;
	case 4:
	  matchmode = MATCH_TFIDF;
	  break;
	case 5:
	  matchmode = MATCH_KBEST;
	  break;
	}
	break;
      case 'b':
	switch(atoi(optarg)) {
	case 1:
	  simop = SO_SCALAR;
	  break;
	case 2:
	  simop = SO_COSINUS;
	  break;
	}
	break;
      case 'p':
	// derive and use response prior probability
	matchmode = MATCH_BAYES;
	break;
      case 's':
	// LOO self-optimization of Q&A pairs
	optimize = true;
	break;
      case 'v':
	// LOO vali-optimization of Q&A pairs
	validate = true;
	break;
      case 'e':
	// LOO-CV evaluation
	looeval = true;
	break;
      case 'd':
	// CV self-optimization (heuristic)
	cvopt = true;
	break;
      case 'f':
	// LOO-CV self-optimization
	loocvopt = true;
	break;
      }
    }
  } else {
    help(argv[0]);
    goto exit_success;
  }

  // init chasen
  parse_init(chacfgfile);

  // read response sentence and Q&A database
  if (qadbfile != NULL && respfile != NULL) {
    mydb = new QADB(string(qadbfile), string(respfile),
		    heapsize, matchmode, simop);
  } else {
    cerr << "Error: cannot read QADB and response sentences." << endl;
    goto exit_failure;
  }

  cerr << "QADB: " << mydb->qadb_size() << " Q&A entries loaded." << endl;
  cerr << "QADB: " << mydb->resp_size() << " distinct responses." << endl;
  cerr << "QADB: " << mydb->morph_cnt() << " distinct morphemes." << endl;

  // read stopword list (for tf-idf scoring)
  if (stopwlist != NULL && matchmode == MATCH_TFIDF)
    mydb->load_stoplist(string(stopwlist));

  // read morpheme confusion table [experimental]
  if (morphtable != NULL)
    mydb->load_morphconftable(string(morphtable));

  // self-optimization of Q&A database
  if (optimize) {
    cerr << "Self-Optimization:" << endl;
    mydb->selfopt();
    if (targetfile != NULL)
      mydb->save_examples(string(targetfile));
    goto exit_success;
  }

  // cross-vali self-optimization of Q&A database
  if (cvopt) {
    cerr << "Cross-Vali Self-Optimization:" << endl;
    mydb->load_validata(infile);
    mydb->selfopt_cv();
    if (targetfile != NULL)
      mydb->save_examples(string(targetfile));
    goto exit_success;
  }

  // leave-one-out cross-vali self-optimization of Q&A database
  if (loocvopt) {
    cerr << "Leave-One-Out Cross-Vali Self-Optimization:" << endl;
    mydb->load_validata(infile);
    mydb->selfopt_loocv();
    if (targetfile != NULL)
      mydb->save_examples(string(targetfile));
    goto exit_success;
  }

  // optimization of Q&A database (mode=1,2,3) or stopword list (mode=4)
  // using an extra validation data set
  if (validate) {
    if (mydb->load_validata(infile)) {
      if (matchmode == MATCH_TFIDF) {
	cerr << "Stoplist Vali-Optimization:" << endl;
	mydb->valiopt();
	if (stopwlist != NULL)
	  mydb->save_stoplist(string(stopwlist));
      } else {
	cerr << "Vali-Optimization:" << endl;
	mydb->valiopt();
	if (targetfile != NULL)
	  mydb->save_examples(string(targetfile));
      }
      goto exit_success;
    } else {
      goto exit_failure;
    }
  }

  // leave-one-out cross-vali evaluation
  // looeval() assumes that the order of Q&A pairs in
  // -i database and the -q validation set is identical
  if (looeval) {
    if (mydb->load_validata(infile)) {
      cerr << "Leave-One-Out Cross-Vali Evaluation:" << endl;
      mydb->looeval(outfile);
      goto exit_success;
    } else {
      goto exit_failure;
    }
  }

  // unsupervised cross-vali labeling mode
  if (labelmode) {
    if (mydb->load_validata(infile)) {
      cerr << "Unsupervised CV-Labeling Mode:" << endl;
      mydb->cvlabel(outfile);
      goto exit_success;
    } else {
      goto exit_failure;
    }
  }
  
  // - read queries from standard input
  // - retrieve best matching Q&A pair
  // - write response ID and response message to standard output
  while (!infile->eof()) {
    infile->getline(&input[0], MAXINLEN);
    if (strlen(&input[0]) == 0) continue;
    if (nbestout) {
      mydb->print_nbestresid(&input[0],nbestout);
    } else {
      qapair = mydb->retrieve(&input[0]);
      *outfile << qapair.m_resid << " " << qapair.m_score << " " << qapair.m_exact << " " << qapair.m_response << " " << qapair.m_question << " " << input << endl;
    }
    iocnt += 1;
    indicator(iocnt,100);
  }
  indicator(iocnt,0);
  cerr << iocnt << " input queries processed." << endl;

 exit_failure:
  if (mydb) delete mydb;
  if (infile != &cin and infile != NULL) delete infile;
  if (outfile != &cout and outfile != NULL) delete outfile;
  return EXIT_FAILURE;

 exit_success:
  if (mydb) delete mydb;
  if (infile != &cin and infile != NULL) delete infile;
  if (outfile != &cout and outfile != NULL) delete outfile;
  return EXIT_SUCCESS;
}

void help (const char * command)
{
  cerr << endl;
  cerr << "Question & Answer Database Evaluation and Optimization Tool V1.5" << endl;
  cerr << endl;
  cerr << "Copyright (c) 2006-2007 Nara Institute of Science and Technology" << endl;
  cerr << "Copyright (c) 2006-2007 Tobias Cincarek" << endl;
  cerr << endl;
  cerr << "All Rights Reserved." << endl;
  cerr << endl;
  cerr << "$Id: qadbman.cc,v 1.3 2007/06/17 16:21:05 cincar-t Exp $" << endl;
  cerr << endl;
  cerr << "Usage: " << command << endl << endl;
  cerr << "  -m <int:mode>    1:exlen, 2:inlen, [3:maxlen], 4:tf-idf, 5:kbest [EXP]" << endl;
  cerr << "  -b <int:dist>    1:scalar, [2:cosinus] (only for tf-idf)" << endl;
  cerr << "  -n <int:nbest>   output n-best response identifiers (mode=1,2,3) [EXP]" << endl;
  cerr << "  -r <file:resp>   file with response sentences" << endl;
  cerr << "  -i <file:qadb>   question and answer database (in)" << endl;
  cerr << "  -o <file:qadb>   question and answer database (out)" << endl;
  cerr << "  -p <bool>        use response prior (mode=3) [EXP]" << endl;
  cerr << "  -q <file:query>  file with test/vali queries (in)" << endl;
  cerr << "  -a <file:hypo>   file with response hypotheses (out)" << endl;
  cerr << "  -t <file:table>  file with morpheme confusion table [EXP]" << endl;
  cerr << "  -x <file:stop>   list of stopwords (only for tf-idf)" << endl;
  cerr << "  -c <config>      chasenrc configuration file" << endl;
  cerr << "  -k <int:hpsize>  heap size during optimization [100]" << endl;
  cerr << "  -s <bool>        LOO self-optimization of qadb" << endl;
  cerr << "  -d <bool>        CV self-optimization of qadb (heuristic)" << endl;
  cerr << "  -f <bool>        LOO-CV self-optimization of qadb [EXP]" << endl;
  cerr << "  -e <bool>        LOO-CV evaluation (same order for -i,-q)" << endl;
  cerr << "  -v <bool>        LOO vali-optimization of qadb (mode=1,2,3)" << endl;
  cerr << "                   LOO vali-optimization of stopwords (mode=4)" << endl;
  cerr << "  -u <bool>        unsupervised cross-vali labeling of queries" << endl;
  cerr << "  -g <int:debug>   1:chasen 2:input/output 3:conftab 4:specific" << endl;
  cerr << endl;  
}

