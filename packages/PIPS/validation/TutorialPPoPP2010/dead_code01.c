int dead_code01()
{
  int i;

  {
  i = 1;
  }

  {
    {
      int j = 2;

      i += j;
    }
  }
  return i;
}
