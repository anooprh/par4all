/* #include<stdio.h> */

/* To obtain two read effects on a and b and a warning abount inneffective update of i invall02 */

typedef struct two_fields{int one; int two[10];} tf_t;

void call02(int i, int j, int y[10], int * q[10], tf_t *p)
{
  i = j +1;
  y[i] = 0;
  p->one = 1;
  p->two[j] = 2.;
  *q[i]=3;
}

main()
{
  int a;
  int b;
  int x[10];
  int * ap[10];
  tf_t s;
  tf_t *sp = &s;

  call02(a, b, x, ap, sp);
}
