/* Synthesize stubs for global pointers for intraprocedural analysis */

int * i;

int * j;

int k;

void global01()
{
  i = &k;
  j = i;
  printf("i = %p",i);
}
