
#include <stdio.h>
#include <chasen.h>

int main(int argc, char ** argv)
{
  char * text = "º£Æü¤Ï";
  char * buffer;

  buffer = chasen_sparse_tostr(text);

  printf("%s\n",buffer);
}
