/* Scalarize a pointer.
   Expected results:
   a) t[i] should be scalarized
   b) a declaration should be created for the new scalar(s)
   Note: as of today, regions are not computed for pointers
   (LD, 19 May 2009)
*/

int main(int argc, char **argv)
{
  int i, n=100;
  int *x, *y, *t;

  for (i=0 ; i<n ; i++) {
    t[i] = x[i];
    x[i] = y[i];
    y[i] = t[i];
  }
}
