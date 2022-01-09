/* ------------------------------------------------------------ -*-c++-*- *\
   Template class Heap 
   
   Copyright (c) 1998,2006 by Tobias Cincarek

   $Id: heap.h,v 1.1 2007/06/07 12:57:10 cincar-t Exp $ 
\* ---------------------------------------------------------------------- */

#ifndef _HEAP_H_
#define _HEAP_H_

template <class KeyType, class DataType>
class CMinHeap 
{
public:
  CMinHeap (int size) ;
  virtual ~CMinHeap () ;

  bool push (const KeyType & key, const DataType & data) ;
  bool pop (DataType * data, KeyType * key) ;
  bool front (DataType * data, KeyType * key = NULL) ;
  int  used (void) { return m_used; }

private:
  KeyType  * m_key ;
  DataType * m_data ;
  int        m_used ;
  int        m_size ;
};

template <class KeyType, class DataType>
class CMaxHeap
{
public:
  CMaxHeap (int size) ;
  virtual ~CMaxHeap () ;

  bool push (const KeyType & key, const DataType & data) ;
  bool pop (DataType * data, KeyType * key) ;
  bool front (DataType * data, KeyType * key = NULL) ;
  int  used (void) { return m_used; }

private:
  KeyType  * m_key ;
  DataType * m_data ;
  int        m_used ;
  int        m_size ;
};

#endif /* _HEAP_H_ */
