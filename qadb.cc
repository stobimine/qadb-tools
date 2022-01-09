
/* ---------------------------------------------------------*-c++-*--
 * 
 *  Question and Answer Database Management Tool
 * 
 *  Copyright (c) 2006-2007 Nara Institute of Science and Technology
 *  Copyright (c) 2006-2007 Tobias Cincarek
 *
 *  All Rights Reserved.
 * 
 * ------------------------------------------------------------------ */

#include "qadb.h"
#include "heap.cc"
#include "irt.cc"

extern int debug;

// Constructor

QADB :: QADB (string qadbfile, string respfile, UINT hs = 100,
	      MatchMode mm = MATCH_MAXLEN, SimOp so = SO_COSINUS)
  : m_maxcode(0), m_tfidfmatrix(so), m_heapsize(hs),
    m_matchmode(mm), m_simop(so)
{
  load_responses(respfile);
  load_examples(qadbfile);
}

// load question-answer pairs

bool QADB :: load_examples (string file)
{
  ifstream * infile = new ifstream(file.c_str());
  char buffer[MAX_BUFLEN+1];
  vector<string> tokens;
  QAPair         pair;
  UINT           cnt = 0;
  UINT           i, n;

  cerr << "Loading Database:" << endl;
  m_morph2code.clear();
  while (!infile->eof()) {
    // Input Format: <Ident> <Example Question>
    infile->getline(&buffer[0], MAX_BUFLEN);
    if (strlen(&buffer[0]) == 0) continue;    
    if (buffer[0] == '#' || buffer[0] == ' ' || strlen(buffer) < 4) continue;
    tokens = split(&buffer[0], ' ');
    if (tokens.size() == 2) {
      pair.m_active   = true;
      // set number of hypotheses to one (in case example is used as query)
      pair.m_hypcnt   = 1;
      // response identifier
      pair.m_resid    = atoi(tokens[0].c_str());
      // example question
      pair.m_question = tokens[1];
      // corresponding response message
      pair.m_response = m_resid2response[pair.m_resid].m_message;
      // morphological analysis of example question
      pair.m_morphseq = parse_sentence(tokens[1].c_str());
      // morpheme sequence length
      pair.m_seqlen   = pair.m_morphseq.size();
      // add counter to identical morphemes
      if (m_matchmode != MATCH_TFIDF and m_matchmode != MATCH_CONF)
	pair.m_morphseq = validate_sentence(pair.m_morphseq);
      // convert morpheme (string) sequence to code sequence
      pair.m_codeseq  = sent2codeseq(pair.m_morphseq);
      // reference index
      pair.m_index = cnt;
      // register Q&A pair
      m_qaset.push_back(pair);
      // make list of response IDs and count response IDs
      if (find(m_residlist.begin(),m_residlist.end(),pair.m_resid) == m_residlist.end()) {
	m_residlist.push_back(pair.m_resid);
	m_resid2prior[pair.m_resid]  = 1.0;
      } else {
	m_resid2prior[pair.m_resid] += 1.0;
      }
      cnt += 1;
      indicator(cnt, 100);
    }
  }
  infile->close();
  indicator(cnt, 0);

  // calculate response prior probabilities
  n = m_residlist.size();
  for (i=0; i<n; i++) {
    m_resid2prior[m_residlist[i]] /= static_cast<float>(cnt);
  }

  // make index, i.e. mapping from morphemes to Q&A indentifiers
  make_index();

  if (infile) delete infile;
  return true;
}

// save Q&A database

bool QADB :: save_examples (string file)
{
  UINT i,n;
  ofstream * outfile = new ofstream(file.c_str());
  
  n = qadb_size();
  for (i=0; i<n; i++) {
    if (m_qaset[i].m_active) {
      *outfile << m_qaset[i].m_resid << " " << m_qaset[i].m_question << endl;
    }
  }
  outfile->close();

  if (outfile) delete outfile;
  return true;
}

// load validation data (text or speech recognition result)

bool QADB :: load_validata (istream * infile)
{
  char           buffer[MAX_BUFLEN+1];
  vector<UINT>   codeseq;
  Sentence       morphseq;
  vector<string> tokens;
  vector<string> hypvec;
  UINT           i;
  QAPair         pair;
  UINT           cnt = 0;

  cerr << "Loading Validation Set:" << endl;
  while (!infile->eof()) {    
    // split into <RESID> <N-BEST-HYP>
    infile->getline(&buffer[0], MAX_BUFLEN);
    if (strlen(&buffer[0]) == 0) continue;
    tokens = split(&buffer[0], ' ');
    if (tokens.size() == 2) {
      // extract n-best recognition hypothesis
      pair = string2qapair(tokens[1].c_str());
      // response identifier and message
      pair.m_resid = atoi(tokens[0].c_str());
      // input query string
      pair.m_question = tokens[1];
      // response message string
      pair.m_response = m_resid2response[pair.m_resid].m_message;
      // reference index
      pair.m_index    = cnt;
    } else {
      pair.m_active   = false;
      pair.m_index    = cnt;
      pair.m_resid    = atoi(tokens[0].c_str());
      pair.m_question = string("");
      pair.m_response = string("");
      pair.m_seqlen   = 0;
      pair.m_hypcnt   = 0;
      pair.m_morphseq.clear();
      pair.m_codeseq.clear();
    }
    // add pair to internal validation set
    m_valiqaset.push_back(pair);
    // counter
    cnt += 1;
    indicator(cnt, 100);
  }
  indicator(cnt, 0);

  return true;
}

// load morpheme confusion table

bool QADB :: load_morphconftable (string file)
{
  ifstream *      infile = new ifstream(file.c_str());
  char            buffer[MAX_BUFLEN+1] = { 0 };
  char            separator[3] = {' ','\t','\0'};
  char *          strp;
  UINT            i, j, n, cnt=0;
  UINT            refc, hypc;
  char            str[12] = { 0 };
  string          refm, hypm;
  float           prob;
  map<UINT,float> marginal;
  float           entropy=0.0,error=0.0;

  m_morph2code[string("*INS*")] = 0;
  m_morph2code[string("*DEL*")] = 0;

  cerr << "Loading Confusion Table:" << endl;
  while (!infile->eof()) {
    // getline from file
    infile->getline(&buffer[0], MAX_BUFLEN);
    // skip zero length lines
    if (strlen(&buffer[0]) == 0) continue;
    // counting
    cnt += 1;
    indicator(cnt, 100);
    // first string is reference symbol
    strp = strtok(&buffer[0],&separator[0]);
    refm = string(strp);
    // convert to internal code
    refc = morph2code(refm);
    // parse one or more (morpheme, score) pairs
    while(strp != NULL) {
      // confused morpheme
      strp = strtok(NULL,&separator[0]);
      if (strp != NULL) {
	hypm = string(strp);
	// internal code conversion
	hypc = morph2code(hypm);
	// morpheme probability (joint probability)
	strp = strtok(NULL,&separator[0]);
	if (strp != NULL) {
	  prob = atof(strp);
	  // debugging information
	  if (debug == 3)
	    cerr << " P(" << refm << "," << hypm << ")=" << prob;
	  // joint probability
	  m_cftab_jp[refc][hypc] = prob;
	  // sum up to obtain marginal probability
	  marginal[refc] += prob;
	}
      }
    }
  }
  indicator(cnt, 0);
  if (infile) delete infile;
  if (debug == 3) cerr << endl;
  // calculate lower bound of error probability
  prob = 1.0;
  for (i=0;i<=m_maxcode;i++) {
    for (j=0;j<=m_maxcode;j++) {
      if (m_cftab_jp[i][j] != 0.0 and marginal[j] != 0.0) {
	// calculate conditonal confusion probability
	m_cftab_cp[i][j] = m_cftab_jp[i][j] / marginal[i];
	// calculate conditional entropy of confusion
	entropy -= m_cftab_jp[i][j] * log(m_cftab_cp[i][j]);
	// determine smallest conditional probability
	prob = (m_cftab_cp[i][j] < prob) ? m_cftab_cp[i][j] : prob;
      }
    }
  }
  m_cftab_cp[0][0] = prob;
  // calculate lower bound on reconstruction error probability
  // known as [Fano's inequality]
  error = (entropy - 1.0) / (log(static_cast<float>(m_maxcode)) - 1.0);
  cerr << "H(hyp|ref) = " << entropy << ", P(error) >= " << error << endl;

  return true;
}

// load list of stopwords (only used for tf-idf scoring)

bool 
QADB :: load_stoplist(string file)
{
  ifstream *     infile = new ifstream(file.c_str());
  char           buffer[MAX_BUFLEN+1] = { 0 };
  char           str[12] = { 0 };
  UINT           cnt=0, code;

  cerr << "Loading Stopword List:" << endl;

  while (!infile->eof()) {
    infile->getline(&buffer[0], MAX_BUFLEN);
    if (strlen(&buffer[0]) == 0) continue;
    code = morph2code(string(&buffer[0]));
    m_stoplist.push_back(code);
    cnt += 1;
    indicator(cnt, 100);
  }
  indicator(cnt, 0);
  infile->close();
  if (infile) delete infile;

  m_tfidfmatrix.add_stoplist(m_stoplist);

  return true;
}

// save list of stopwords (e.g. after optimization)

void
QADB :: save_stoplist(string file)
{
  UINT i,k;
  ofstream * outfile = new ofstream(file.c_str());
  
  k = m_stoplist.size();
  for (i=0; i<k; i++) {
    *outfile << m_code2morph[m_stoplist[i]] << endl;
  }
  outfile->close();
  if (outfile) delete outfile;
}

// make indices for faster access:
// morphemes -> response IDs
// tf-idf matrix

void QADB :: make_index (void)
{
  UINT i, k, n, m, resid;
  string token;

  cerr << "Making Term->Entry Index:" << endl;

  m_morph2indexlist.clear();
  m_code2indexlist.clear();

  // mapping from morphemes (as text or code) to response ID list
  n = qadb_size();
  for (i=0; i<n; i++) {
    if (i > 0) indicator(i, 1000);
    if (m_qaset[i].m_active) {
      m = m_qaset[i].m_morphseq.size();
      for (k=0; k<m; k++) {
	token = m_qaset[i].m_morphseq[k].m_origin;
	m_morph2indexlist[token].push_back(i);
	m_code2indexlist[m_qaset[i].m_codeseq[k]].push_back(i);
      }
      // make a tf-vector for each question set
      // corresponding to the same response identifier
      m_resid2tfvector[m_qaset[i].m_resid].add_termlist(m_qaset[i].m_codeseq);
      // m_resid2tfvector[i].add_termlist(m_qaset[i].m_codeseq);
    }
  }
  indicator(n, 0);

  if (m_matchmode == MATCH_TFIDF) {
    cerr << "Making TF/IDF Matrix:" << endl;
    // make tf-idf matrix
    k = m_residlist.size();
    // k = qadb_size();
    for (i=0; i<k; i++) {
      if (i>0) indicator(i, 1000);
      resid = m_residlist[i];
      m_tfidfmatrix.add_document(m_resid2tfvector[resid], resid);
      // m_tfidfmatrix.add_document(m_resid2tfvector[i], i);
    }
    m_tfidfmatrix.to_tfidf();
    indicator(k, 0);
  }
}

// load list of response sentences

bool QADB :: load_responses (string file)
{
  ifstream * infile = new ifstream(file.c_str());
  char buffer[MAX_BUFLEN+1];
  vector<string> tokens;
  UINT resid;

  while (!infile->eof()) {
    infile->getline(&buffer[0], MAX_BUFLEN);
    if (buffer[0] == '#' || buffer[0] == ' ' || strlen(buffer) < 4) continue;
    tokens = split(&buffer[0], ' ');
    if (tokens.size() == 2) {
      resid = static_cast<UINT>(atoi(tokens[0].c_str()));
      m_resid2response[resid].m_message = tokens[1];
      m_resid2response[resid].m_ident   = resid;
    }
  }
  infile->close();

  if (infile) delete infile;
  return true;
}

void QADB :: print(QAPair & pair)
{
  UINT k;

  for (k=0; k<pair.m_seqlen; k++) {
    cerr << pair.m_morphseq[k].m_origin << " ";
  }
  cerr << endl;
  for (k=0; k<pair.m_seqlen; k++) {
    cerr << pair.m_codeseq[k] << " ";
  }
  cerr << endl;
}

// evaluation with leave-one-out cross-validation (LOO-CV)
// assumes same order of queries in training and validation data

void QADB :: looeval(ostream * outfile)
{
  UINT j,n,c;
  QAPair qapair;
  float rate;

  n = qadb_size();

  if (n == m_valiqaset.size()) {
    for (c=0, j=0; j<n; j++) {
      if (j > 0) indicator(j, 100);
      m_qaset[j].m_active = false;
      qapair = retrieve(m_valiqaset[j].m_morphseq, m_valiqaset[j].m_hypcnt);
      if (qapair.m_resid == m_valiqaset[j].m_resid) c++;
      m_qaset[j].m_active = true;
      *outfile << qapair.m_resid << " " << qapair.m_score << " " << qapair.m_response << endl;
    }
    indicator(j, 0);
    rate = static_cast<float>(c)/static_cast<float>(n);
    cerr << "LOO Response Accuracy: " << rate << " [" << n << "]" << endl;
  } else {
    cerr << "Error: Size of QADB (" << n << ") and Vali-Set (";
    cerr << m_valiqaset.size() << ") are different." << endl;
  }
}

// unsupervised cross-vali labeling

void QADB :: cvlabel(ostream * outfile)
{
  CMaxHeap<float,UINT> * tmpheap = NULL;
  map <UINT,float> count;
  map <UINT,float> score;
  UINT    a,i,j,k,n,m,c;
  QAPair  qapair;
  float   rate;
  float   maxrate = 0.0;
  UINT  * save_index = NULL;
  float * save_score = NULL;
  bool    success = false;
  UINT    bestresid = 0;
  UINT    inc=0,dec=0,eqr=0;

  cerr << "Making Score Heap..." << endl;

  n = qadb_size();
  m = m_valiqaset.size();

  save_index = new UINT[n+m];
  save_score = new float[n+m];

  // remember query to database question match scores using a heaplist
  for (c=0,i=0; i<n; i++) {
    if (i > 0) indicator(i, 10);
    tmpheap = new CMaxHeap<float,UINT>(static_cast<int>(n));
    qapair = retrieve(m_qaset[i].m_morphseq, m_qaset[i].m_hypcnt);
    for (j=0; j<n; j++) {
      if (i != j) tmpheap->push(m_qaset[j].m_score, j);
    }
    tmpheap->pop(&save_index[i], &save_score[i]);
    delete tmpheap;
    if (m_qaset[save_index[i]].m_resid == m_qaset[i].m_resid) c++;
  }
  indicator(i, 0);

  maxrate = static_cast<float>(c)/static_cast<float>(n);
  cerr << "Initial RA=" << (maxrate*100.0) << endl;

  cerr << "Unsupervised CV Labeling" << endl;
  for (a=0,j=0; j<m; j++) {
    // initialization
    for (k=0; k<m_residlist.size(); k++) {
      count[m_residlist[k]] = 0;
      score[m_residlist[k]] = 0.0;
    }
    // find best matching example question in the database
    qapair = retrieve(m_valiqaset[j].m_morphseq, m_valiqaset[j].m_hypcnt);
    for (c=0,i=0; i<n; i++) {
      if (m_qaset[i].m_score >= save_score[i]) {
	// score higher than best matching LOO example
	// count occurrence of the response identifiers
	count[m_qaset[i].m_resid] += 1;
	score[m_qaset[i].m_resid] += 1.0;
	if (m_qaset[save_index[i]].m_resid == m_qaset[i].m_resid)
	  score[m_qaset[save_index[i]].m_resid] += 1.0;
      } else {
	// score lower than best matching LOO example
        if (m_qaset[save_index[i]].m_resid == m_qaset[i].m_resid) c++;
      }
    }
    // find best response ID
    // finally reset counters for next query
    bestresid = m_residlist[0];
    for (k=1; k<m_residlist.size(); k++) {
      if (score[m_residlist[k]] > score[bestresid]) {
	 bestresid = m_residlist[k];
      }
    }
    // calculate new response accuracy
    rate = static_cast<float>(c+count[bestresid])/static_cast<float>(n);
    if (rate > maxrate) {
      maxrate = rate;
      // adding example query increases response accuracy
      cerr << "+";
      if (debug) cerr << " " << (100.0*rate) << endl;
      inc ++;
      *outfile << bestresid << " " << m_valiqaset[j].m_question << endl;
      for (i=0; i<n; i++) {
        if (m_qaset[i].m_score > save_score[i]) {
          save_score[i] = m_qaset[i].m_score;
          save_index[i] = n+a;
        }
      }
      save_score[n+a] = qapair.m_score;
      save_index[n+a] = qapair.m_index;
      qapair = m_valiqaset[j];
      qapair.m_active = false;
      qapair.m_index  = n+a;
      qapair.m_resid  = bestresid;
      m_qaset.push_back(qapair);
      a++;
    } else if (rate < maxrate) {
      // adding example query decreases response accuracy
      cerr << "-";
      if (debug) cerr << " " << (100.0*rate) << endl;
      dec ++;
    } else {
      // adding example query does not effect response accuracy
      cerr << "=";
      if (debug) cerr << " " << (100.0*rate) << endl;
      eqr ++;
    }
    // if (j > 0) indicator(j, 100);
  }
  indicator(j, 0);

  // statistics
  cerr << inc << " (+), " << dec << " (-), " << eqr << " (=)" << endl;
}

// LOO optimization of QADB using validation data set

void QADB :: valiopt(void)
{
  UINT i,j,k,l,n,m,c,t;
  QAPair qapair;
  float rate = 0.0;
  float maxrate = 0.0;
  float score;
  CMaxHeap<float,UINT> ** heap = NULL;
  CMaxHeap<float,UINT> * tmpheap = NULL;
  UINT * save_index = NULL;
  float * save_score = NULL;
  bool success = false;
  UINT exclude = 0;
  int d;
  map < UINT, vector<UINT> > c2i;

  // extra variables for tf-idf stop-term list optimization
  CTermVector<UINT> ** tfvecs = NULL;
  UINT code,best;
  bool progress = true;
  UINT loops = 0;

  n = qadb_size();
  m = m_valiqaset.size();

  if (m_matchmode == MATCH_TFIDF) {
    // construct all tf-vectors in advance
    cerr << "Making Query Vectors:" << endl;
    tfvecs = static_cast<CTermVector<UINT> **>(calloc(m, sizeof(CTermVector<UINT> *)));
    for (c=0,i=0; i<m; i++) {
      indicator(i, 100);
      tfvecs[i] = new CTermVector<UINT>(m_simop);
      tfvecs[i]->add_termlist(m_valiqaset[i].m_codeseq);
      best = m_tfidfmatrix.retrieve(*tfvecs[i]);
      if (m_valiqaset[i].m_resid == best) c++;
    }
    // make morpheme code -> query index mapping for faster optimization
    for (i=0; i<m; i++) {
      k = m_valiqaset[i].m_codeseq.size();
      for (j=0; j<k; j++) {
	code = m_valiqaset[i].m_codeseq[j];
	if (find(c2i[code].begin(),c2i[code].end(),i) == c2i[code].end()) {
	  c2i[code].push_back(i);
        }
      }
    }
    indicator(m,0);
    // debug information
    if (debug == 4) {
      for (j=1; j<=m_maxcode; j++)
        cerr << m_code2morph[j] << "(" << c2i[j].size() << ")";
      cerr << endl;
    }
    // initial performance
    maxrate = static_cast<float>(c)/static_cast<float>(m);
    // optimize stopterm list for tf-idf
    cerr << "Initial RA=" << (100.0*maxrate) << endl;
    while (progress) {
      progress = false;
      loops += 1;
      cerr << "Iteration " << loops << ":" << endl;
      for (j=1; j<=m_maxcode; j++) {
	if (find(m_stoplist.begin(),m_stoplist.end(),j) == m_stoplist.end()) {
	  m_tfidfmatrix.del_stoplist();
	  m_tfidfmatrix.add_stoplist(m_stoplist);
	  k = c2i[j].size();
	  for (d=0,i=0;i<k;i++) {
	    best = m_tfidfmatrix.retrieve(*tfvecs[c2i[j][i]]);
	    if (m_valiqaset[c2i[j][i]].m_resid == best) d--;
          }
	  m_tfidfmatrix.add_stopterm(j);
	  for (i=0;i<k;i++) {
	    best = m_tfidfmatrix.retrieve(*tfvecs[c2i[j][i]]);
	    if (m_valiqaset[c2i[j][i]].m_resid == best) d++;
  	  }
	  rate = static_cast<float>(c+d)/static_cast<float>(m);
	  // add term to stoplist if performance increased
	  if (rate > maxrate) { 
	    c += d;
	    maxrate = rate;
	    m_stoplist.push_back(j);
	    exclude += 1;
	    progress = true;
	    cerr << "+";
	  } else if (rate == maxrate) {
	    cerr << "=";
	  } else {
	    cerr << "-";
	  }
	  if (debug == 4) cerr << abs(d);
	} else {  
          cerr << "*";
	}	
      }
      indicator(m_maxcode, 0);
      cerr << "Iteration " << loops << ": RA=" << (100.0*maxrate);
      cerr << " (" << exclude << " terms excluded)" << endl;
    }
    m_tfidfmatrix.del_stoplist();
    m_tfidfmatrix.add_stoplist(m_stoplist);
    cerr << exclude << " terms excluded (" << loops << " iterations)." << endl;
    cerr << "Final RA=" << (100.0*maxrate) << endl;
    // free memory
    for (i=0; i<m; i++) delete tfvecs[i];
    free(tfvecs);
    cerr << endl;
  } else {
    cerr << "Making Score Heap..." << endl;
    save_index = new UINT[n];
    save_score = new float[n];
    heap = static_cast<CMaxHeap<float,UINT> **>(calloc(m,sizeof(CMaxHeap<float,UINT> *)));
    // remember query to question match scores using list of heaps
    for (c=0,i=0; i<m; i++) {
      indicator(i, 10);
      tmpheap = new CMaxHeap<float,UINT>(static_cast<int>(n));
      qapair = retrieve(m_valiqaset[i].m_morphseq, m_valiqaset[i].m_hypcnt);
      for (j=0; j<n; j++) {
	tmpheap->push(m_qaset[j].m_score, j);
      }
      if (m_heapsize >= n) {
	heap[i] = tmpheap;
      } else {
	heap[i] = new CMaxHeap<float,UINT>(static_cast<int>(m_heapsize));
	for (j=0; j<m_heapsize; j++) {
	  tmpheap->pop(&save_index[0], &save_score[0]);
	  heap[i]->push(save_score[0], save_index[0]);
	}
	delete tmpheap;
      }
      if (qapair.m_resid == m_valiqaset[i].m_resid) c++;
    }
    indicator(i, 0);
    // initial response accuracy
    maxrate = static_cast<float>(c)/static_cast<float>(m);
    cerr << "Before Optimization: RA=" << (100.0*maxrate) << endl;
    for (i=0; i<n; i++) {
      m_qaset[i].m_active = false;
      for (c=0,j=0; j<m; j++) {
	// get id of best-matching item from heap
	success = heap[j]->front(&t);
	k = 0;
	while (success and not m_qaset[t].m_active) {
	  heap[j]->pop(&save_index[k], &save_score[k]);
	  k++;
	  success = heap[j]->front(&t);
	}
	if (success and m_qaset[t].m_active and m_qaset[t].m_resid == m_valiqaset[j].m_resid) c++;
	// push back items popped from heap
	while (k-- > 0) heap[j]->push(save_score[k], save_index[k]);
      }
      rate = static_cast<float>(c)/static_cast<float>(m);
      if (rate > maxrate) {
	maxrate = rate;
	cerr << "+";
	exclude += 1;
      } else if (rate == maxrate) {
	m_qaset[i].m_active = true;
	cerr << "=";
      } else {
	m_qaset[i].m_active = true;
	cerr << "-";
      }
    }
    cerr << endl;
    cerr << exclude << " Items Excluded." << endl;
    cerr << "After Optimization: RA=" << (100.0*maxrate) << endl;
    // free memory
    for (i=0; i<m; i++) delete heap[i];
    free(heap);
    delete [] save_score;
    delete [] save_index;
  }
}

// LOO self-optimization of QADB
// slow since data structure is (unsorted) matrix
// (unused and obsolete function)

void QADB :: selfopt_matrix (void)
{
  UINT i,j,k,n,c,t;
  QAPair qapair;
  float rate;
  float maxrate = 0.0;
  float ** matrix = NULL;
  float s;
  UINT exclude = 0;

  cerr << "Making Score Matrix ..." << endl;

  // matrix to remember match score for each query and question pair
  n = qadb_size();
  matrix = static_cast<float **>(calloc(n, sizeof(float *)));
  for (c=0,j=0; j<n; j++) {
    if (j > 0) indicator(j, 10);
    matrix[j] = static_cast<float *>(calloc(n, sizeof(float)));
    qapair = retrieve(m_qaset[j].m_morphseq, m_qaset[j].m_hypcnt);
    for (i=0; i<n; i++) matrix[j][i] = m_qaset[i].m_score;
    if (qapair.m_resid == m_qaset[j].m_resid) c++;
  }
  indicator(j, 0);
  // initial response accuracy
  maxrate = static_cast<float>(c)/static_cast<float>(n);

  cerr << "Before Optimization: RA=" << (100.0*maxrate) << endl;
  cerr << "Optimizing ..." << endl;
  for (i=0; i<n; i++) {
    m_qaset[i].m_active = false;
    for (c=0,j=0; j<n; j++) {
      s = 0.0;
      t = 0;
      for (k=0; k<n; k++) {
	// score table-lookup
	if (matrix[j][k] > s and m_qaset[k].m_active) {
	  s = matrix[j][k];
	  t = k;
	}
      }
      if (m_qaset[t].m_active and m_qaset[j].m_resid == m_qaset[t].m_resid) c++;
    }
    rate = static_cast<float>(c)/static_cast<float>(n);
    if (rate > maxrate) {
      maxrate = rate;
      cerr << "+"; 
      exclude += 1;
    } else if (rate == maxrate) {
      m_qaset[i].m_active = true;
      cerr << "=";
    } else {
      m_qaset[i].m_active = true;
      cerr << "-";
    }
  }
  cerr << endl;

  cerr << exclude << " Items Excluded." << endl;
  cerr << "After Optimization: RA=" << (100.0*maxrate) << endl;

  for (i=0; i<n; i++)
    free(matrix[i]);
  free(matrix);
}

// LOO self-optimization of QADB

void QADB :: selfopt (void)
{
  UINT i,j,k,n,c;
  QAPair qapair;
  float rate;
  float maxrate = 0.0;
  CMaxHeap<float,UINT> ** heap = NULL;
  CMaxHeap<float,UINT> * tmpheap = NULL;
  UINT * save_index = NULL;
  float * save_score = NULL;
  UINT t;
  bool  success = false;
  UINT exclude = 0;

  cerr << "Making Score Heap..." << endl;

  n = qadb_size();

  save_index = new UINT[n];
  save_score = new float[n];

  heap = static_cast<CMaxHeap<float,UINT> **>(calloc(n,sizeof(CMaxHeap<float,UINT> *)));

  // remember query to question match scores using of list of heaps
  for (c=0,i=0; i<n; i++) {
    if (i > 0) indicator(i, 10);
    tmpheap = new CMaxHeap<float,UINT>(static_cast<int>(n));
    qapair = retrieve(m_qaset[i].m_morphseq, m_qaset[i].m_hypcnt);
    for (j=0; j<n; j++) {
      tmpheap->push(m_qaset[j].m_score, j);
    }
    if (m_heapsize >= n) {
      heap[i] = tmpheap;
    } else {
      heap[i] = new CMaxHeap<float,UINT>(static_cast<int>(m_heapsize));
      for (j=0; j<m_heapsize; j++) {
	tmpheap->pop(&save_index[0], &save_score[0]);
	heap[i]->push(save_score[0], save_index[0]);
      }
      delete tmpheap;
    }
    // assert(qapair.m_resid == m_qaset[t].m_resid);
    if (qapair.m_resid == m_qaset[i].m_resid) c++;
  }
  indicator(i, 0);
  // initial response accuracy
  maxrate = static_cast<float>(c)/static_cast<float>(n);

  cerr << "Before Optimization: RA=" << (100.0*maxrate) << endl;
  cerr << "Optimizing ..." << endl;
  for (i=0; i<n; i++) {
    m_qaset[i].m_active = false;
    for (c=0,j=0; j<n; j++) {
      // get id of best-matching item from heap
      success = heap[j]->front(&t);
      k = 0;
      while (success and not m_qaset[t].m_active) {
	heap[j]->pop(&save_index[k], &save_score[k]);
	k++;
	success = heap[j]->front(&t);
      }
      if (success and m_qaset[t].m_active and m_qaset[t].m_resid == m_qaset[j].m_resid) c++;
      // push back items popped from heap
      while (k-- > 0) heap[j]->push(save_score[k], save_index[k]);
    }
    rate = static_cast<float>(c)/static_cast<float>(n);
    if (rate > maxrate) {
      maxrate = rate;
      cerr << "+";
      exclude += 1;
    } else if (rate == maxrate) {
      m_qaset[i].m_active = true;
      cerr << "=";
    } else {
      m_qaset[i].m_active = true;
      cerr << "-";
    }
  }
  cerr << endl;
  cerr << exclude << " Items Excluded." << endl;
  cerr << "After Optimization: RA=" << (100.0*maxrate) << endl;

  for (i=0; i<n; i++) delete heap[i];
  free(heap);
  delete [] save_score;
  delete [] save_index;
}

// Self-Optimization with Cross-Validation (CV)

void QADB :: selfopt_cv (void)
{
  UINT t,i,j,k,n,m,c;
  QAPair qapair;
  float rate;
  float maxrate = 0.0;
  CMaxHeap<float,UINT> ** heap = NULL;
  CMaxHeap<float,UINT> * tmpheap = NULL;
  UINT * save_index = NULL;
  float * save_score = NULL;
  bool success = false;
  bool state;
  UINT exclude = 0;

  cerr << "Making Score Heap..." << endl;

  n = qadb_size();
  m = m_valiqaset.size();

  // check integrity of database and validation data
  if (n == m) {
    for (c=0,i=0; i<n; i++) {
      if (m_qaset[i].m_resid == m_valiqaset[i].m_resid) {
	c++;
      } else {
	cerr << m_qaset[i].m_resid << " " << m_valiqaset[i].m_resid << endl;
      }
    }
  } else {
    c = 0;
  }
  // reject mismatching data sets
  if (c != n or c != m) {
    cerr << "Error: Q&A DB (-i) and Validation Data (-q) Mismatch" << endl;
    cerr << "Error: C=" << c << " D=" << n << " V=" << m << endl;
    return;
  }

  save_index = new UINT[n];
  save_score = new float[n];

  heap = static_cast<CMaxHeap<float,UINT> **>(calloc(n,sizeof(CMaxHeap<float,UINT> *)));

  // initialization of the list of heap structure
  // make mapping of queries to ranklists of matching example questions
  // example questions are ranked by the matchscore with the query
  for (c=0,i=0; i<n; i++) {
    m_qaset[i].m_active = false;
    qapair = retrieve(m_valiqaset[i].m_morphseq, m_valiqaset[i].m_hypcnt);
    // mark datum as 'active' (initialization)
    m_qaset[i].m_active = true;
    // mark datum as 'dispensible' (initialization)
    m_valiqaset[i].m_active = false;
    // make heap for ranking example questions
    tmpheap = new CMaxHeap<float,UINT>(static_cast<int>(n));
    for (j=0; j<n; j++) {
      if (i != j)
	tmpheap->push(m_qaset[j].m_score, j);
    }
    // memory reduction by reducing heap size
    if (m_heapsize >= n) {
      heap[i] = tmpheap;
    } else {
      heap[i] = new CMaxHeap<float,UINT>(static_cast<int>(m_heapsize));
      for (j=0; j<m_heapsize; j++) {
	tmpheap->pop(&save_index[0], &save_score[0]);
	heap[i]->push(save_score[0], save_index[0]);
      }
      delete tmpheap;
    }
    // increase counter for correctly classified queries
    if (qapair.m_resid == m_valiqaset[i].m_resid) c++;
    if (i > 0) indicator(i, 10);
  }
  indicator(i, 0);

  // initial response accuracy
  rate = static_cast<float>(c)/static_cast<float>(n);

  cerr << "Before Optimization: RA=" << (100.0*rate) << endl;
  cerr << "Optimizing ..." << endl;
  // optimization with cross-validation
  for (c=0,i=0; i<n; i++) {
    // find best matching example question with the correct response
    success = heap[i]->front(&t);
    k = 0;
    while (success and m_qaset[t].m_resid != m_qaset[i].m_resid) {
      heap[i]->pop(&save_index[k], &save_score[k]);
      k++;
      success = heap[i]->front(&t);
    }
    if (success and m_qaset[t].m_resid == m_qaset[i].m_resid) {
      // if an example question with correct response was found
      if (k > 0) {
	for (j=0; j<k; j++) {
	  // restore heap (necessary for final evaluation)
	  heap[i]->push(save_score[j],save_index[j]);
	  // deactivate interfering data
	  m_qaset[save_index[j]].m_active = false;
	}
	// remember 'indispensible' data
	m_valiqaset[t].m_active = true;
	cerr << "+";
      } else {
	// remember 'indispensible' data
	m_valiqaset[t].m_active = true;
	cerr << "=";
      }
      c++;
    } else {
      // restore heap (necessary for final evaluation)
      while (k-- > 0) {
	heap[i]->push(save_score[k], save_index[k]);
      }
      cerr << "-";
    }
  }
  cerr << endl;
  maxrate = static_cast<float>(c)/static_cast<float>(n);

  // determine number of items finally excluded
  for (i=0; i<n; i++) {
    // re-activate interfering data if it is 'indispensible'
    if (m_valiqaset[i].m_active == true)
      m_qaset[i].m_active = true;
    // count deactivated data
    if (m_qaset[i].m_active == false)
      exclude++;
  }
  cerr << exclude << " Items Excluded." << endl;

  // final evaluation of response accuracy
  for (c=0,i=0; i<n; i++) {
    success = heap[i]->front(&t);
    k = 0;
    while (success and m_qaset[t].m_active == false) {
      heap[i]->pop(&save_index[k], &save_score[k]);
      k++;
      success = heap[i]->front(&t);
    }
    // while (k-- > 0) {
    //   // restore heap
    //   heap[i]->push(save_score[k], save_index[k]);
    // }
    if (m_qaset[t].m_active and m_qaset[t].m_resid == m_valiqaset[i].m_resid) c++;
  }

  maxrate = static_cast<float>(c)/static_cast<float>(n);
  cerr << "Before Optimization: RA=" << (100.0*rate) << endl;
  cerr << "After Optimization: RA=" << (100.0*maxrate) << endl;

  for (i=0; i<n; i++) delete heap[i];
  free(heap);

  delete [] save_score;
  delete [] save_index;
}

// Leave-One-Out Self-Optimization with Cross-Validation (CV)

void QADB :: selfopt_loocv (void)
{
  UINT t,i,j,k,n,m,c;
  QAPair qapair;
  float initrate = 0.0;
  float rate;
  CMaxHeap<float,UINT> ** heap = NULL;
  CMaxHeap<float,UINT> * tmpheap = NULL;
  UINT * save_index = NULL;
  int * weight = NULL;
  float * save_score = NULL;
  bool success = false;
  bool state;
  UINT exclude = 0;

  cerr << "Making Score Heap..." << endl;

  n = qadb_size();
  m = m_valiqaset.size();

  // check integrity of database and validation data
  if (n == m) {
    for (c=0,i=0; i<n; i++) {
      if (m_qaset[i].m_resid == m_valiqaset[i].m_resid) {
	c++;
      } else {
	cerr << m_qaset[i].m_resid << " " << m_valiqaset[i].m_resid << endl;
      }
    }
  } else {
    c = 0;
  }
  // reject mismatching data sets
  if (c != n or c != m) {
    cerr << "Error: Q&A DB (-i) and Validation Data (-q) Mismatch" << endl;
    cerr << "Error: C=" << c << " D=" << n << " V=" << m << endl;
    return;
  }

  save_index = new UINT[n];
  save_score = new float[n];
  weight = new int[n];

  heap = static_cast<CMaxHeap<float,UINT> **>(calloc(n,sizeof(CMaxHeap<float,UINT> *)));

  // initialization of the list of heap structure
  // make mapping of queries to ranklists of matching example questions
  // example questions are ranked by the matchscore with the query
  for (c=0,i=0; i<n; i++) {
    weight[i] = 0;
    // deactivate datum for cross-validation
    m_qaset[i].m_active = false;
    qapair = retrieve(m_valiqaset[i].m_morphseq, m_valiqaset[i].m_hypcnt);
    // mark datum as 'active' (initialization)
    m_qaset[i].m_active = true;
    // make heap for ranking example questions
    tmpheap = new CMaxHeap<float,UINT>(static_cast<int>(n));
    for (j=0; j<n; j++) {
      if (i != j)
	tmpheap->push(m_qaset[j].m_score, j);
    }
    // reduce memory usage by reducing the heap size
    // actually, only the highest ranked example needs to be stored
    // consequenly, a heap size m_heapsize = 1 would suffice
    if (m_heapsize >= n) {
      heap[i] = tmpheap;
    } else {
      heap[i] = new CMaxHeap<float,UINT>(static_cast<int>(m_heapsize));
      for (j=0; j<m_heapsize; j++) {
	tmpheap->pop(&save_index[0], &save_score[0]);
	heap[i]->push(save_score[0], save_index[0]);
      }
      delete tmpheap;
    }
    // increase counter for correctly classified queries
    if (qapair.m_resid == m_valiqaset[i].m_resid) c++;
    if (i > 0) indicator(i, 10);
  }
  indicator(i, 0);

  // initial response accuracy
  initrate = static_cast<float>(c)/static_cast<float>(n);

  cerr << "Before Optimization: RA=" << (100.0*initrate) << endl;
  cerr << "Optimizing:" << endl;
  // leave-one-out optimization with cross-validation
  // i: example to be left out
  for (i=0; i<n; i++) {
    if (i>0) indicator(i, 1000);
    // j: cycle through all data
    for (c=0,j=0; j<n; j++) {
      success = heap[j]->front(&t);
      if (success) {
	if (t == i) {
	  heap[j]->pop(&save_index[0], &save_score[0]);
	  success = heap[j]->front(&t);
	  heap[j]->push(save_score[0], save_index[0]);
	  if (success) {
	    if (m_valiqaset[j].m_resid == m_qaset[i].m_resid) {
	      // increase weight if example is important
	      if (m_valiqaset[j].m_resid != m_qaset[t].m_resid) weight[i]++;
	    } else {
	      // decrease weight if example has negative effect
	      if (m_valiqaset[j].m_resid == m_qaset[t].m_resid) weight[i]--;
	    }
	  }
	}
      }
    }
  }
  indicator(n, 0);
  for (i=0; i<n; i++) {
    if (weight[i] < 0) {
      m_qaset[i].m_active = false;
      exclude++;
    }
  }
  cerr << exclude << " Items Excluded." << endl;

  // final evaluation of response accuracy
  for (c=0,i=0; i<n; i++) {
    success = heap[i]->front(&t);
    k = 0;
    while (success and m_qaset[t].m_active == false) {
      heap[i]->pop(&save_index[k], &save_score[k]);
      k++;
      success = heap[i]->front(&t);
    }
    // while (k-- > 0) {
    //   // restore heap
    //   heap[i]->push(save_score[k], save_index[k]);
    // }
    if (success and m_qaset[t].m_active and m_qaset[t].m_resid == m_valiqaset[i].m_resid) c++;
  }

  rate = static_cast<float>(c)/static_cast<float>(n);
  cerr << "Before Optimization: RA=" << (100.0*initrate) << endl;
  cerr << "After Optimization: RA=" << (100.0*rate) << endl;

  for (i=0; i<n; i++) delete heap[i];
  free(heap);

  delete [] save_score;
  delete [] save_index;
  delete [] weight;
}

// retrieve function with fast-matching algorithm
// generate a response for given query string

QAPair QADB :: retrieve (const char * query)
{
  QAPair pair = string2qapair(query);
  return retrieve(pair.m_morphseq, pair.m_hypcnt);
}

void QADB :: print_nbestresid (const char * query, int nbest)
{
  QAPair pair = string2qapair(query);
  print_nbestresid(pair.m_morphseq, pair.m_hypcnt, nbest);
}

QAPair QADB :: retrieve (Sentence & query, int hypcnt)
{
  vector<UINT> codeseq;
  UINT i,j,k,l,n,m,r,s,c;
  float inlen, exlen, maxlen;
  float f, score;
  float maxscore = 0.0;
  string token;
  UINT resid, len, best = 0;
  UINT * mtcnts = NULL;
  map<UINT,float> resid2score;
  CMaxHeap<float,UINT> * heap = NULL;
  vector<AlignElement> alignpath;
  QAPair pair;

  // some preparations
  n = qadb_size();
  codeseq = sent2codeseq(query);
  len = query.size();
  inlen = static_cast<float>(len);

  // table-based fast matching algorithm
  if (m_matchmode != MATCH_TFIDF) {
    mtcnts = static_cast<UINT *>(calloc(n, sizeof(UINT)));
    for (j=0; j<len; j++) {
      m = m_code2indexlist[codeseq[j]].size();
      for (k=0; k<m; k++) {
	l = m_code2indexlist[codeseq[j]][k];
	if (m_qaset[l].m_active) mtcnts[l]++;
      }
    }
  }
  
  // match mode dependent processing
  switch(m_matchmode) {
  case MATCH_KBEST:
    heap = new CMaxHeap<float,UINT>(n);
    for (i=0;i<n;i++) {
      exlen = static_cast<float>(m_qaset[i].m_seqlen * hypcnt);
      maxlen = (inlen > exlen) ? inlen : exlen;
      // prefer higher match counts / longer examples (heuristic)
      m_qaset[i].m_score = pow(static_cast<double>(mtcnts[i]),1.0001) / maxlen;
      heap->push(m_qaset[i].m_score, i);
      mtcnts[i] = 0;
    }
    heap->front(&best, &maxscore);
    pair = m_qaset[best];
    for (i=0; i<n; i++) {
      heap->pop(&c, &score);
      resid = m_qaset[c].m_resid;
      if (mtcnts[resid] < 5) {
	resid2score[resid] += score;
	mtcnts[resid] += 1;
      }
    }
    maxscore = 0.0;
    m = m_residlist.size();
    for (j=0;j<m;j++) {
      resid = m_residlist[j];
      score = resid2score[resid]/static_cast<float>(mtcnts[resid]);
      if (score > maxscore) {
	maxscore = score;
	best = resid;
      }
    }
    pair.m_score = maxscore;
    pair.m_resid = best;
    break;
  case MATCH_TFIDF:
    pair = retrieve_tfidf(query);
    break;
  case MATCH_CONF:
    // experimental
    // employ morpheme confusion scores
    // use only single best hypothesis
    for (i=0; i<n; i++) {
      alignpath = alignment(m_qaset[i].m_codeseq, codeseq);
      len = alignpath.size();
      score = 0.0;
      for (j=0;j<len;j++) {
	if (alignpath[j].m_type == ALIGN_COR || alignpath[j].m_type == ALIGN_SUB) {
	  r = m_qaset[i].m_codeseq[alignpath[j].m_ref];
	  s = codeseq[alignpath[j].m_hyp];
	} else if (alignpath[j].m_type == ALIGN_INS) {
	  r = 0;
	  s = codeseq[alignpath[j].m_hyp];
	} else if (alignpath[j].m_type == ALIGN_DEL) {
	  r = m_qaset[i].m_codeseq[alignpath[j].m_ref];
	  s = 0;
	}
	if (m_cftab_cp[r][s] != 0.0) {
	  score += log(m_cftab_cp[r][s]);
	} else {
	  score += log(m_cftab_cp[0][0]);
	}
      }
      if (score != 0.0) {
	m_qaset[i].m_score = exp(score / static_cast<float>(len));
      } else {
	m_qaset[i].m_score = mtcnts[i] / static_cast<float>(maxlen);
      }
      if (m_qaset[i].m_score > maxscore) {
	maxscore = m_qaset[i].m_score;
	best = i;
      }
    }
    pair = m_qaset[best];
    // debug output for best-matching example
    if (m_matchmode == MATCH_CONF and debug == 3) {
      cerr << "E" << best << " SCORE=" << m_qaset[best].m_score << endl;
      alignpath = alignment(m_qaset[best].m_codeseq, codeseq);
      len = alignpath.size();
      for (j=0;j<len;j++) {
	if (alignpath[j].m_type == ALIGN_COR || alignpath[j].m_type == ALIGN_SUB) {
	  r = m_qaset[best].m_codeseq[alignpath[j].m_ref];
	  s = codeseq[alignpath[j].m_hyp];
	  if (m_cftab_cp[r].size() > 0) {
	    cerr << "P(" << m_qaset[best].m_morphseq[alignpath[j].m_ref].m_origin;
	    cerr << "|" << query[alignpath[j].m_hyp].m_origin << ")=";
	    cerr << m_cftab_cp[r][s] << " ";
	  }
	}
      }
      cerr << endl;
    }
    break;
  case MATCH_MAXLEN:
    for (i=0; i<n; i++) {
      exlen = static_cast<float>(m_qaset[i].m_seqlen * hypcnt);
      maxlen = (inlen > exlen) ? inlen : exlen;
      // prefer higher match counts / longer examples (heuristic)
      m_qaset[i].m_score = pow(static_cast<double>(mtcnts[i]),1.0001) / maxlen;
      if (m_qaset[i].m_score > maxscore) {
	maxscore = m_qaset[i].m_score;
	best = i;
      }
      if (inlen == exlen && inlen == mtcnts[i])
	m_qaset[i].m_exact = true;
      else
	m_qaset[i].m_exact = false;
    }
    pair = m_qaset[best];
    break;
  case MATCH_EXLEN:
    for (i=0; i<n; i++) {
      exlen = static_cast<float>(m_qaset[i].m_seqlen * hypcnt);
      maxlen = (inlen > exlen) ? inlen : exlen;
      m_qaset[i].m_score = static_cast<float>(mtcnts[i]) / exlen;
      if (m_qaset[i].m_score > maxscore) {
	maxscore = m_qaset[i].m_score;
	best = i;
      }
      if (inlen == exlen && inlen == mtcnts[i])
	m_qaset[i].m_exact = true;
      else
	m_qaset[i].m_exact = false;
    }
    pair = m_qaset[best];
    break;
  case MATCH_INLEN:
    for (i=0; i<n; i++) {
      exlen = static_cast<float>(m_qaset[i].m_seqlen * hypcnt);
      maxlen = (inlen > exlen) ? inlen : exlen;
      m_qaset[i].m_score = static_cast<float>(mtcnts[i]) / inlen;
      if (m_qaset[i].m_score > maxscore) {
	maxscore = m_qaset[i].m_score;
	best = i;
      }
      if (inlen == exlen && inlen == mtcnts[i])
	m_qaset[i].m_exact = true;
      else
	m_qaset[i].m_exact = false;
    }
    pair = m_qaset[best];
    break;
  case MATCH_BAYES:
    for (i=0; i<n; i++) {
      exlen = static_cast<float>(m_qaset[i].m_seqlen * hypcnt);
      maxlen = (inlen > exlen) ? inlen : exlen;
      // experimental
      // prefer higher match counts / longer examples (heuristic)
      score = pow(static_cast<double>(mtcnts[i]),1.0001) / maxlen;
      m_qaset[i].m_score = score * m_resid2prior[m_qaset[i].m_resid];
      if (m_qaset[i].m_score > maxscore) {
	maxscore = m_qaset[i].m_score;
	best = i;
      }
      if (inlen == exlen && inlen == mtcnts[i])
	m_qaset[i].m_exact = true;
      else
	m_qaset[i].m_exact = false;
    }
    pair = m_qaset[best];
    break;
  default:
    best = 0;
    maxscore = 0;
    for (i=0; i<n; i++) {
      m_qaset[i].m_score = 0.0;
      m_qaset[i].m_exact = false;
    }
    pair = m_qaset[best];
    break;
  }

  if (mtcnts) free(mtcnts);
  if (heap) delete heap;

  return pair;
}

void QADB :: print_nbestresid (Sentence & query, int hypcnt, int nbest)
{
  vector<UINT> codeseq;
  UINT i,j,k,l,n,m,r,s,c;
  float inlen, exlen, maxlen;
  float f, score;
  float maxscore = 0.0;
  string token;
  UINT resid, len, best = 0;
  UINT * mtcnts;
  float * scores;
  CMaxHeap<float,UINT> * heap; 
  
  n = qadb_size();
  codeseq = sent2codeseq(query);
  len = query.size();

  heap = new CMaxHeap<float,UINT>(n);
  mtcnts = new UINT[n];

  // table-based fast matching algorithm
  for (j=0; j<len; j++) {
    m = m_code2indexlist[codeseq[j]].size();
    for (k=0; k<m; k++) {
      l = m_code2indexlist[codeseq[j]][k];
      if (m_qaset[l].m_active) mtcnts[l]++;
    }
  }

  inlen = static_cast<float>(len);
  for (i=0; i<n; i++) {
    exlen = static_cast<float>(m_qaset[i].m_seqlen * hypcnt);
    switch(m_matchmode) {
    case MATCH_MAXLEN:
      maxlen = (inlen > exlen) ? inlen : exlen;
      // prefer higher match counts / longer examples (heuristic)
      score  = pow(static_cast<double>(mtcnts[i]),1.0001) / maxlen;
      break;
    case MATCH_EXLEN:
      score = static_cast<float>(mtcnts[i]) / exlen;
      break;
    case MATCH_INLEN:
      score = static_cast<float>(mtcnts[i]) / inlen;
      break;
    default:
      assert(0);
      break;
    }
    heap->push(score, m_qaset[i].m_resid);
  }

  i = 0;
  while (heap->used()>0 &&  i<nbest) {
    heap->pop(&resid,&score);
    if (i==0)
      cout << resid << ":" << score;
    else
      cout << "/" << resid << ":" << score;
    i++;
  }
  cout << endl;

  delete heap;
  delete [] mtcnts;
}

// tf-idf-matrix-based retrieve function

QAPair QADB :: retrieve_tfidf (Sentence & query)
{
  UINT i,n,best;
  float score;
  CTermVector<UINT> tfvec(m_simop);
  QAPair pair;
  
  // pair.m_morphseq = query;
  pair.m_codeseq = sent2codeseq(query);
  tfvec.add_termlist(pair.m_codeseq);

  best = m_tfidfmatrix.retrieve(tfvec, &score);
  pair.m_score  = score;
  pair.m_resid  = best;
  pair.m_index  = 0;
  pair.m_active = true;
  pair.m_exact  = false;
  pair.m_question = string("*");
  pair.m_response = m_resid2response[best].m_message;
  
  // m_qaset[best].m_score = score;
  // return m_qaset[best];

  return pair;
}

// helper functions

QAPair QADB :: string2qapair(const char * input)
{
  vector<UINT>   codeseq;
  Sentence       morphseq;
  vector<string> tokens;
  vector<string> hypvec;
  UINT           i,j;
  QAPair         pair;

  // n-best recognition hypothesis
  // split into <1-BEST HYP> <2-BEST HYP> ...
  hypvec = split(input,'|');
  // preprocessing of all hypotheses
  if (hypvec.size() > 0) {
    pair.m_hypcnt = hypvec.size();
    pair.m_active = true;
    pair.m_seqlen = 0;
    // do for each hypotheses
    for (i=0; i<hypvec.size(); i++) {
      // morphological analysis 
      morphseq = parse_sentence(hypvec[i].c_str());
      if (m_matchmode != MATCH_TFIDF and m_matchmode != MATCH_CONF) {
        // add counter to identical morphemes
        // in order to avoid double matching
	morphseq = validate_sentence(morphseq);
      }
      // convert morpheme (string) sequence to code sequence
      codeseq = sent2codeseq(morphseq);
      // count number of morphemes
      pair.m_seqlen += morphseq.size();
      for (j=0; j<morphseq.size(); j++) {
	pair.m_morphseq.push_back(morphseq[j]);
	pair.m_codeseq.push_back(codeseq[j]);
      }
      // use only single best recognition hypothesis
      // for confusion probability based scoring
      if (m_matchmode == MATCH_CONF) return pair;
    }
    return pair;
  } else {
    pair.m_hypcnt = 0;
    pair.m_active = false;
    pair.m_seqlen = 0;
    return pair;
  }
}

// convert morpheme sequence into an internal code sequence

vector<UINT> QADB :: sent2codeseq(Sentence & sent)
{
  UINT i;
  UINT n = sent.size();
  vector<UINT> codeseq(n);

  for (i=0; i<n; i++)
    codeseq[i] = morph2code(sent[i].m_origin);

  return codeseq;
}

// convert morpheme string to internal code

inline UINT QADB :: morph2code(string morph)
{
  if (m_morph2code[morph] == 0) {
    m_maxcode++;
    m_morph2code[morph] = m_maxcode;
    m_code2morph[m_maxcode] = morph;
    return m_maxcode;
  } else {
    return m_morph2code[morph];
  }
}

// avoid double matching of the same morpheme

Sentence & QADB :: validate_sentence(Sentence & sent)
{
  UINT i,n;
  map<string,UINT> morphcnt;
  char str[12];

  n = sent.size();
  for (i=0; i<n; i++) morphcnt[sent[i].m_origin] = 0;
  for (i=0; i<n; i++) {
    morphcnt[sent[i].m_origin] += 1;
    if (morphcnt[sent[i].m_origin] >= 2) {
      sprintf(&str[0],":%u",morphcnt[sent[i].m_origin]);
      sent[i].m_origin += string(&str[0]);
    }
  }

  return sent;
}

