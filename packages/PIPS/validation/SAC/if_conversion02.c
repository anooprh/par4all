void foo()
{
    int i,tab[4][4];
    for(i=0;i<sizeof(tab)/sizeof(tab[0]);i++)
        if(tab[i][i]>i)tab [i]=i;
}
