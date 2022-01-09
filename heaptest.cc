
#include "heap.cc"
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char ** argv)
{
  string input;
  long   value, key;
  CMaxHeap<long,long> heap1(1000);
  CMaxHeap<long,long> heap2(1000);

  while (cin >> input) {
    value = atol(input.c_str());
    heap1.push(value,value);
    heap2.push(value,value);
  }

  while (heap1.pop(&value,&key)) {
    cout << value << endl;
    heap2.pop(&value,&key);
    heap2.push(value,value);
  }

  while (heap2.pop(&value,&key)) {
    cout << value << endl;
  }

  return 0;
}
