typedef struct _IO_FILE _IO_FILE;
struct _IO_FILE_plus;
typedef unsigned int size_t;
struct _IO_FILE {
  char _unused2[15 * sizeof (int) - 4 * sizeof (void *) - sizeof (size_t)];
};
extern struct _IO_FILE_plus _IO_2_1_stdin_;

