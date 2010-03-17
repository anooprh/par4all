typedef struct IMMENSE {unsigned long l,r;} immense;

float ran4(idum)
int *idum;
{
        static int newkey,iff=0;
        static immense inp,key,jot;
        static double pow[66];
        unsigned long isav,isav2;
        int j;
        double r4;
        void des();

        if (*idum < 0 || iff == 0) {
                iff=1;
                *idum %= 11979;
                if (*idum < 0) *idum += 11979;
                pow[1]=0.5;
                key.r=key.l=inp.r=inp.l=0L;
                for (j=1;j<=64;j++) {
                        *idum = ((long) (*idum)*430 +2531) % 11979;
                        isav=2*(unsigned long)(*idum)/11979;
                        if (isav) isav=0x80000000L;
                        isav2=(4*(unsigned long)(*idum)/11979) % 2;
                        if (isav2) isav2=0x80000000L;
                        if (j <= 32) {
                                key.r=(key.r >>= 1) | isav;
                                inp.r=(inp.r >>= 1) | isav2;
                        } else {
                                key.l=(key.l >>= 1) | isav;
                                inp.l=(inp.l >>= 1) | isav2;
                        }
                        pow[j+1]=0.5*pow[j];
                }
                newkey=1;
        }
        isav=inp.r & 0x80000000L;
        if (isav) isav=1L;
        if (inp.l & 0x80000000L)
                inp.r=((inp.r ^ 1L +4L +8L) << 1) | 1L;
        else
                inp.r <<= 1;
        inp.l=(inp.l << 1) | isav;
        des(inp,key,&newkey,0,&jot);
        r4=0.0;
        for (j=1;j<=24;j++) {
                if (jot.r & 1L) r4 += pow[j];
                jot.r >>= 1;
        }
        return r4;
}
