// lhs and rhs with store dependent subscripts
// also tests elimination of syntactically equivalent pointer_values
// generated by b[++i] = a (b[*] == a(may) is redundant with a==b[*] (may))

#include <stdlib.h>


int main()
{
  int *a;
  int * b[10];
  int i = 5;
  a = b[i];
  b[++i] = a;
  return(0);
}
