/* ------------------------------------------------------------ -*-c++-*- *\
 *
 *  Information Retrieval Stuff
 *
 *  Copyright (c) 2006 by Nara Institute of Science and Technology
 *  All Rights Reserved.
 *
 *  1st Author: Tobias Cincarek
 *
 *  $Id: irt.h,v 1.1 2007/06/07 12:57:10 cincar-t Exp $
 *
\* ---------------------------------------------------------------------- */

#ifndef _IRT_H_
#define _IRT_H_

#include "typedefs.h"
#include <vector>
#include <map>

using namespace std;

typedef enum { SO_SCALAR, SO_COSINUS } SimOp;

template <class ElementType> class CTermDocuMatrix ;

template <class ElementType>
class CTermVector
{
  friend void CTermDocuMatrix<ElementType> :: to_tfidf(void);
public:
  CTermVector(SimOp op) : m_simop(op), m_cnt(0) {}
  CTermVector() : m_simop(SO_SCALAR), m_cnt(0) {}
  virtual ~CTermVector() {}

  void add_term(ElementType term) ;
  void add_termlist(vector<ElementType> & termlist) ;
  void add_vector(CTermVector<ElementType> & vec) ;

  void norm(void);
  void print(void);

  vector<ElementType> & termlist(void) ;

  float operator*(CTermVector<ElementType> & vec) ;
  float operator[](ElementType & term) ;
  void  modify(ElementType & term, float value) ;

private:
  map<ElementType,float> m_data;
  vector<ElementType>    m_keys;
  UINT                   m_cnt;
  SimOp                  m_simop;
};

template <class ElementType>
class CTermDocuMatrix
{
public:
  CTermDocuMatrix(SimOp op = SO_SCALAR) : m_counter(0), m_simop(op) {}
  virtual ~CTermDocuMatrix() {}

  void add_stoplist(vector<ElementType> & stoptermlist);
  void add_stopterm(ElementType & term);
  void del_stoplist(void);

  void add_document(CTermVector<ElementType> & tfvec, UINT ident);

  UINT retrieve(vector<ElementType> query, float * myscore = NULL);
  UINT retrieve(CTermVector<ElementType> query, float * myscore = NULL);

  CTermVector<ElementType> & operator[](UINT ident);

  void to_tfidf(void);
  void print_matrix(void);

private:
  map< UINT, CTermVector<ElementType> > m_matrix;
  vector< UINT >                        m_ident;
  vector< ElementType >                 m_stoplist;
  UINT                                  m_counter; // # of documents
  SimOp                                 m_simop;
};

#endif /* _IRT_H_ */
