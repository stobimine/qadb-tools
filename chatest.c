
#include <stdio.h>
#include <chasen.h>

int main(int argc, char ** argv)
{
  char * text = "������";
  char * buffer;

  buffer = chasen_sparse_tostr(text);

  printf("%s\n",buffer);
}
