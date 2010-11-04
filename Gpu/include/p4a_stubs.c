#include <stdlib.h>

/* A small implementation of the runtime used by the code generated by the
   kernel_load_store and isolate_statement
*/


/* To copy scalars */
void * P4A_copy_from_accel(size_t element_size,
			   void *host_address,
			   void *accel_address) {
  size_t i;
  char * cdest = host_address;
  char * csrc = accel_address;
  for(i = 0; i < element_size; i++)
    cdest[i] = csrc[i];
  return host_address;
}


void * P4A_copy_to_accel(size_t element_size,
			 void *host_address,
			 void *accel_address) {
  size_t i;
  char * cdest = accel_address;
  char * csrc = host_address;
  for(i = 0; i < element_size; i++)
    cdest[i] = csrc[i];
  return accel_address;
}


/* To copy parts of 1D arrays */
void * P4A_copy_from_accel_1d(size_t element_size,
                              size_t d1_size,
			      size_t d1_block_size,
                              size_t d1_offset,
                              void *host_address,
                              void *accel_address) {
  size_t i;
  char * cdest = d1_offset*element_size + (char *)host_address;
  char * csrc = accel_address;
  for(i = 0; i < d1_block_size*element_size; i++)
    cdest[i] = csrc[i];
  return host_address;
}


void * P4A_copy_to_accel_1d(size_t element_size,
			    size_t d1_size,
                            size_t d1_block_size,
                            size_t d1_offset,
                            void *host_address,
                            void *accel_address) {
  size_t i;
  char * cdest = accel_address;
  char * csrc = d1_offset*element_size + (char *)host_address;
  for(i = 0; i < d1_block_size*element_size; i++)
    cdest[i] = csrc[i];
  return accel_address;
}


/* To copy parts of 2D arrays */
void * P4A_copy_from_accel_2d(size_t element_size,
                              size_t d1_size, size_t d2_size,
                              size_t d1_block_size, size_t d2_block_size,
                              size_t d1_offset, size_t d2_offset,
                              void *host_address,
                              void *accel_address) {
  size_t i, j;
  char * cdest = d2_offset*element_size + (char*)host_address;
  char * csrc = (char*)accel_address;
  for(i = 0; i < d1_block_size; i++)
    for(j = 0; j < d2_block_size*element_size; j++)
      cdest[(i + d1_offset)*element_size*d2_size + j] =
        csrc[i*element_size*d2_block_size + j];
  return host_address;
}


void * P4A_copy_to_accel_2d(size_t element_size,
                            size_t d1_size, size_t d2_size,
                            size_t d1_block_size, size_t d2_block_size,
                            size_t d1_offset,   size_t d2_offset,
                            void *host_address,
                            void *accel_address) {
  size_t i, j;
  char * cdest = (char *)accel_address;
  char * csrc = d2_offset*element_size + (char *)host_address;
  for(i = 0; i < d1_block_size; i++)
    for(j = 0; j < d2_block_size*element_size; j++)
      cdest[i*element_size*d2_block_size + j] =
        csrc[(i + d1_offset)*element_size*d2_size + j];
  return accel_address;
}

/* Allocate memory on the accelerator */
void P4A_accel_malloc(void **ptr, size_t n) {
    if(n) *ptr=malloc(n);
    else *ptr=NULL;
}

/* Deallocate memory on the accelerator */
void P4A_accel_free(void *ptr) {
    free(ptr);
}
