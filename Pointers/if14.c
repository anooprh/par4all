/* Impact of conditions on points-to
 *
 * Check equality to NULL
 */

#include <stdio.h>

int if14() {
  int *p, i, j;

  p = NULL;

  if(p!=NULL)
    p = &i;
  else
    p = &j;
  
  return 0;
}
