/* Simplified version of decl24. Bug with const and volatile
   declaration */

/* But lost for void... The internal representation
   cannot keep the information. */
const void * next_data;
volatile void * previous_data;
