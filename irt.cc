/* ------------------------------------------------------------ -*-c++-*- *\
   Information Retrieval Stuff

   Copyright (c) 2006 by Nara Institute of Science and Technology
   All Rights Reserved.

   1st Author: Tobias Cincarek

   $Id: irt.cc,v 1.1 2007/06/07 12:57:10 cincar-t Exp $
\* ---------------------------------------------------------------------- */

#include <vector>
#include <map>
#include <cmath>
#include <iostream>

#include "irt.h"

template <class ElementType>
void CTermVector <ElementType> :: add_term(ElementType term)
{
  if (find(m_keys.begin(),m_keys.end(), term) == m_keys.end()) {
    m_keys.push_back(term);
  }
  m_data[term] += 1.0;
  m_cnt++;
}

template <class ElementType>
void CTermVector <ElementType> :: add_termlist(vector<ElementType> & terms)
{
  UINT i,k;
  k = terms.size();
  for (i=0; i<k; i++) {
    add_term(terms[i]);
  }
}

template <class ElementType>
void CTermVector <ElementType> :: add_vector(CTermVector<ElementType> & tfvec)
{
  UINT i,k;
  vector<ElementType> terms;

  m_cnt += tfvec.m_cnt;
  terms = tfvec.termlist();
  k = terms.size();
  for (i=0; i<k; i++) {
    m_data[terms[i]] += tfvec[terms[i]];
  }
}

template <class ElementType>
void CTermVector <ElementType> :: print(void)
{
  UINT i,k;
  k = m_keys.size();
  for (i=0; i<k; i++) {
    cerr << " " << m_keys[i] << ":" << m_data[m_keys[i]];
  }
  cerr << endl;
}

template <class ElementType>
void CTermVector <ElementType> :: norm(void)
{
  float normfaq = 1.0 / static_cast<float>(m_cnt);
  UINT i,k;

  k = m_keys.size();
  for (i=0; i<k; i++) {
    m_data[m_keys[i]] *= normfaq;
  }
}

template <class ElementType>
vector<ElementType> & CTermVector <ElementType> :: termlist(void)
{
  return m_keys;
}

template <class ElementType>
float CTermVector <ElementType> :: operator*(CTermVector<ElementType> & vec)
{
  UINT i,k;
  float v = 0.0, a = 0.0, b = 0.0;

  switch(this->m_simop) {
  case SO_SCALAR:
    k = vec.m_keys.size();
    for (i=0; i<k; i++) {
      v += this->m_data[vec.m_keys[i]] * vec.m_data[vec.m_keys[i]];
    }
    break;
  case SO_COSINUS:
    k = vec.m_keys.size();
    for (i=0; i<k; i++) {
      v += this->m_data[vec.m_keys[i]] * vec.m_data[vec.m_keys[i]];
      a += vec.m_data[vec.m_keys[i]] * vec.m_data[vec.m_keys[i]];
    }
    k = m_keys.size();
    for (i=0; i<k; i++) {
      b += this->m_data[m_keys[i]] * this->m_data[m_keys[i]];
    }
    v = v/sqrt(a*b);
    break;
  }
  
  return v;
}

template <class ElementType>
float CTermVector <ElementType> :: operator[](ElementType & term)
{
  return m_data[term];
}

template <class ElementType>
void CTermVector <ElementType> :: modify (ElementType & term, float value)
{
  m_data[term] = value;
}

template <class ElementType>
void CTermDocuMatrix <ElementType> :: add_document(CTermVector<ElementType> & tfvec, UINT ident)
{
  UINT i,d;
  if (find(m_ident.begin(),m_ident.end(), ident) == m_ident.end()) {
    m_ident.push_back(ident);
    m_matrix[ident] = tfvec;
    m_counter++;
  } else {
    m_matrix[ident].add_vector(tfvec);
  }
}

template <class ElementType>
void CTermDocuMatrix <ElementType> :: add_stoplist(vector<ElementType> & stoptermlist)
{
  UINT i,n;
  n = stoptermlist.size();
  for (i=0; i<n; i++) {
    m_stoplist.push_back(stoptermlist[i]);
  }
}

template <class ElementType>
void CTermDocuMatrix <ElementType> :: add_stopterm(ElementType & term)
{
  m_stoplist.push_back(term);
}

template <class ElementType>
void CTermDocuMatrix <ElementType> :: del_stoplist(void)
{
  m_stoplist.clear();
}

template <class ElementType>
UINT CTermDocuMatrix <ElementType> :: retrieve(vector<ElementType> query, float * myscore)
{
  CTermVector<ElementType> tfvec(m_simop);
  tfvec.add_termlist(query);
  return retrieve(tfvec,myscore);
}

template <class ElementType>
UINT CTermDocuMatrix <ElementType> :: retrieve(CTermVector<ElementType> query, float * myscore)
{
  UINT i,j,d;
  UINT best = m_ident.front();
  float maxscore = 0.0, score = 0.0;
  CTermVector<ElementType> exvec;

  // apply stoplist to query vector
  // i.e. zero weights of corresponding terms
  d = m_stoplist.size();
  for (j=0; j<d; j++) query.modify(m_stoplist[j], 0.0);
  query.norm();

  // calculate similary between the query (vector)
  // and stored documents represented by tf-idf vectors
  for (i=0; i<m_counter; i++) {
    exvec = m_matrix[m_ident[i]];
    for (j=0; j<d; j++) exvec.modify(m_stoplist[j], 0.0);
    score = exvec * query;
    if (score > maxscore) {
      maxscore = score;
      best = m_ident[i];
    }
  }

  if (myscore != NULL) *myscore = maxscore;

  return best;
}

template <class ElementType>
CTermVector <ElementType> & CTermDocuMatrix <ElementType> :: operator[](UINT ident)
{
  return m_matrix[ident];
}

template <class ElementType>
void CTermDocuMatrix <ElementType> :: to_tfidf(void)
{
  CTermVector<ElementType> idf(m_simop);
  vector<ElementType> terms;
  UINT i,j,m;
  
  // determine document frequency for each term
  for (i=0; i<m_counter; i++) {
    terms = m_matrix[m_ident[i]].termlist();
    idf.add_termlist(terms);
  }

  // df -> idf
  terms = idf.termlist();
  m = terms.size();
  for (j=0; j<m; j++) {
    if (idf.m_data[terms[j]] > 0.0) {
      idf.m_data[terms[j]] = log(static_cast<float>(m_counter)/idf[terms[j]])+1.0;
    } else {
      idf.m_data[terms[j]] = 1.0;
    }
  }

  // tf-matrix -> tf-idf-matrix
  for (i=0; i<m_counter; i++) {
    m_matrix[m_ident[i]].norm();
    terms = m_matrix[m_ident[i]].termlist();
    m = terms.size();
    for (j=0; j<m; j++) {
      m_matrix[m_ident[i]].m_data[terms[j]] *= idf[terms[j]];
    }
  }
}
