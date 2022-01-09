
/* -------------------------------------------------*-c++-*--
 * 
 * Question and Answer Database Management Tool
 * 
 * Copyright (c) 2006 Nara Institute of Science and Technology
 *
 * 1st Author: Tobias Cincarek
 *
 * All Rights Reserved.
 * 
 * ---------------------------------------------------------- */

#ifndef _QADB_H_
#define _QADB_H_

#include "parse.h"
#include "util.h"
#include "heap.h"
#include "irt.h"

#define MAX_BUFLEN 65536

typedef struct {
  UINT          m_ident;
  float         m_score;
  string        m_message;
  string        m_webpage;
  string        m_action;
} Response;

typedef struct {
  bool          m_active;   // usable for Q&A
  bool          m_exact;    // exact match with input query
  UINT          m_index;    // Q&A pair internal index
  UINT          m_seqlen;   // length of Question
  UINT          m_resid;    // response ID
  int           m_hypcnt;   // number of input hypothesis
  float         m_score;    // match score
  string        m_question; // example question
  string        m_response; // response sentence
  vector<UINT>  m_codeseq;  // internal morpheme code sequence
  Sentence      m_morphseq; // morpheme code sequence
} QAPair;

typedef enum { MATCH_EXLEN, MATCH_INLEN, MATCH_MAXLEN,
	       MATCH_CONF, MATCH_TFIDF, MATCH_BAYES, MATCH_KBEST } MatchMode;

class QADB
{
 public:
  QADB(string qadbfile, string respfile, UINT heapsize,
       MatchMode mm, SimOp simop);
  virtual ~QADB() {}

  // methods to load or save Q&A Database
  bool load_responses(string file);
  bool load_examples(string file);
  bool save_examples(string file);

  // load vali question set
  bool load_validata(istream * infile);

  // load morpheme confusion table
  bool load_morphconftable(string file);

  // load stopword list
  bool load_stoplist(string file);
  void save_stoplist(string file);

  // return number of Q&A pairs loaded
  UINT qadb_size(void) { return m_qaset.size(); }
  // return number of distinct response sentences loaded
  UINT resp_size(void) { return m_resid2response.size(); }
  // return number of distinct morphemes
  UINT morph_cnt(void) { return m_maxcode; }

  // LOO optimization of Q&A database using validation data set
  void valiopt(void);

  // LOO self-optimization of Q&A database
  void selfopt(void);
  void selfopt_matrix(void);
  void selfopt_cv(void);
  void selfopt_loocv(void);

  // LOO evaluation (cross-validation)
  void looeval(ostream * outfile);

  // unsupervised labeling
  void cvlabel(ostream * outfile);

  // determine best Q&A pair for given query
  QAPair retrieve(const char * query);
  QAPair retrieve(string & query) { return retrieve(query.c_str()); }
  QAPair retrieve(Sentence & query, int hypcnt = 1);
  QAPair retrieve_tfidf(Sentence & query);

  // output n-best Q&A pairs for given query
  void print_nbestresid(const char * query, int nbest = 10);
  void print_nbestresid(string & query, int nbest = 10) { print_nbestresid(query.c_str(), nbest); }
  void print_nbestresid(Sentence & query, int hypcnt = 1, int nbest = 10);

  // print Q&A pair
  void print(QAPair & pair);

 protected:
  // same surface from of morphemes in same sentence -> discernment
  Sentence & validate_sentence(Sentence & sent);
  // convert text-based kanji morphemes into number codes
  vector<UINT> sent2codeseq(Sentence & sent);
  UINT morph2code(string morph);
  string code2morph(UINT code) { return m_code2morph[code]; }

 private:
  // make index (morpheme to response ID mapping) for fast matching
  // make term-frequency inverse document-frequency matrix
  void make_index(void);
  QAPair string2qapair(const char * input);

  // mapping from morpheme (as text) to Q&A indices
  map< string, vector<UINT> >       m_morph2indexlist;
  // mapping from morpheme (as code) to Q&A indices
  map< UINT, vector<UINT> >         m_code2indexlist;
  // mapping from morpheme text to morpheme code
  map< string, UINT >               m_morph2code;
  map< UINT, string >               m_code2morph;
  // mapping from response ID to response object
  map< UINT, Response >             m_resid2response;
  // mapping from response ID to response prior
  map< UINT, float >                m_resid2prior;
  // mapping from response ID to term-frequency vector
  map< UINT, CTermVector<UINT> >    m_resid2tfvector;
  // term-frequency inverse document-frequency matrix
  CTermDocuMatrix<UINT>             m_tfidfmatrix;

  // current maximum morpheme code number
  UINT              m_maxcode;
  // set of all Q&A pairs loaded
  vector< QAPair >  m_qaset;
  // set of all vali Q&A pairs loaded
  vector< QAPair >  m_valiqaset;
  // list of response IDs
  vector< UINT >    m_residlist;

  // morpheme confusion probability table (joint, conditional probs)
  map< UINT, map <UINT, float> >    m_cftab_jp;
  map< UINT, map <UINT, float> >    m_cftab_cp;
  // list of stop words
  vector< UINT >                    m_stoplist;
  // match score mode
  MatchMode                         m_matchmode;
  // tf-idf similarity mode
  SimOp                             m_simop;

  // maximum heap size for optimization
  UINT                              m_heapsize;
};

#endif /* _QADB_H_ */
