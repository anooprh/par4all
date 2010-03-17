/* Check the type detection when an offset is added to a pointer via
   an array construct. This used to complete the profile of a
   partially declared function. */

void decl30(void)
{
 double foo();
 double (*x)[];
 double z;
 x = malloc(10);
 z = foo(&(*x)[0]);
}
