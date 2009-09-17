#include <stdlib.h>
void *safe_malloc(size_t n)
{
    void * ptr = malloc(n*sizeof(*ptr));
    if(!ptr)
        exit(EXIT_FAILURE);
    else
        return ptr;
}
