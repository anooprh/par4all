/* Transformers in the second loop nest are wrong: 0==-1 is always
   returned

   If the first loop nest is commented out, the problem disappears.
 */

int main ()
{
  float a[10][10][10][10][10];
  int i,j,k,l,m;
  float x;
  float b[10][10];
  i = 0;
  j = 0;
  k = 0;
  l = 0;
  m = 0;
  x = 2.12;

  for (i = 0; i < 10; i++) {
    for (j = 0; j < 10; j++) {
      b[i][j] = 0.0;
    }
  }

  //i = i; // to check the poscondition
  /*
  for (i = 0; i < 10; i++) {
    for (j = 0; j < 10; j++) {
      // comments 2
      float z;
      z = 0.0;
      for (k = 0; k < 10; k++) {
	z = k * 2.0;
	for (l = 0; l < 10; l++) {
	  for (m = 0; m < 10; m++) {
	    float y;
	    //comments 3
	    // this break privatization
	    y = 2.0;
	    // comments 4
	    y = 3.5 + x + z;
	    a[i][j][k][l][m] = x*y;
	  }
	}
      }
    }
  }
  */
  return 0;
}
