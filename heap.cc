/* ------------------------------------------------------------ -*-c++-*- *\
   Template Class CHeap 

   Copyright (c) 1998,2006 by Tobias Cincarek 
   All Rights Reserved.

   $Id: heap.cc,v 1.1 2007/06/07 12:57:10 cincar-t Exp $ 
\* ---------------------------------------------------------------------- */

#include <climits>
#include <cstdlib>
#include "heap.h"

template <class KeyType, class DataType>
CMinHeap <KeyType, DataType> :: CMinHeap (int size)
{
  m_size   = size ;
  m_key    = new KeyType [m_size+1] ;
  m_data   = new DataType [m_size+1] ;
  m_used   = 0 ;
  m_key[0] = LONG_MIN ;
}

template <class KeyType, class DataType>
CMinHeap <KeyType, DataType> :: ~CMinHeap ()
{
  if (m_key)  delete [] m_key ;
  if (m_data) delete [] m_data ;
}

template <class KeyType, class DataType>
bool CMinHeap <KeyType, DataType> :: push (const KeyType & key, const DataType & data)
{
  if (m_used + 1 > m_size) return false ;
  
  int k = ++m_used ;

  while (m_key[k/2] > key and k > 1) {
    m_key[k]  = m_key[k/2] ;
    m_data[k] = m_data[k/2] ;
    k = k/2 ;
  }
  
  m_key[k]  = key ;
  m_data[k] = data ;

  return true;
}

template <class KeyType, class DataType>
bool CMinHeap <KeyType, DataType> :: pop (DataType * data, KeyType * key)
{
  if (m_used == 0) return false ;

  *data = m_data[1] ;
  *key  = m_key[1] ;

  int k = 2 ;
  for (;;) {
    if (k >= m_used) break; 
    if (k+1 < m_used and m_key[k] > m_key[k+1]) k++ ;
    if (m_key[m_used] > m_key[k]) {
      m_key[k/2]  = m_key[k] ;
      m_data[k/2] = m_data[k] ;
    } else {
      break ;
    }
    k *= 2;
  }

  m_key[k/2]  = m_key[m_used] ;
  m_data[k/2] = m_data[m_used] ;
  m_used -- ;

  return true ;
}

template <class KeyType, class DataType>
bool CMinHeap <KeyType, DataType> :: front (DataType * data, KeyType * key)
{
  if (m_used == 0) {
    return false ;
  } else {
    *data = m_data[1] ;
    if (key != NULL) *key = m_key[1] ;
    return true ;
  }
}

template <class KeyType, class DataType>
CMaxHeap <KeyType, DataType> :: CMaxHeap (int size)
{
  m_size   = size ;
  m_key    = new KeyType [m_size+1] ;
  m_data   = new DataType [m_size+1] ;
  m_used   = 0 ;
  m_key[0] = LONG_MAX ;
}

template <class KeyType, class DataType>
CMaxHeap <KeyType, DataType> :: ~CMaxHeap ()
{
  if (m_key)  delete [] m_key ;
  if (m_data) delete [] m_data ;
}

template <class KeyType, class DataType>
bool CMaxHeap <KeyType, DataType> :: push (const KeyType & key, const DataType & data)
{
  if (m_used + 1 > m_size) return false ;
  
  int k = ++m_used ;

  while (m_key[k/2] < key and k > 1) {
    m_key[k]  = m_key[k/2] ;
    m_data[k] = m_data[k/2] ;
    k = k/2 ;
  }
  
  m_key[k]  = key ;
  m_data[k] = data ;

  return true;
}

template <class KeyType, class DataType>
bool CMaxHeap <KeyType, DataType> :: pop (DataType * data, KeyType * key)
{
  if (m_used == 0) return false ;

  *data = m_data[1] ;
  *key  = m_key[1] ;

  int k = 2 ;
  for (;;) {
    if (k >= m_used) break; 
    if (k+1 < m_used and m_key[k] < m_key[k+1]) k++ ;
    if (m_key[m_used] < m_key[k]) {
      m_key[k/2]  = m_key[k] ;
      m_data[k/2] = m_data[k] ;
    } else {
      break ;
    }
    k *= 2;
  }

  m_key[k/2]  = m_key[m_used] ;
  m_data[k/2] = m_data[m_used] ;
  m_used -- ;

  return true ;
}

template <class KeyType, class DataType>
bool CMaxHeap <KeyType, DataType> :: front (DataType * data, KeyType * key)
{
  if (m_used == 0) {
    return false ;
  } else {
    *data = m_data[1] ;
    if (key != NULL) *key = m_key[1] ;
    return true ;
  }
}
