float midinf(funk,aa,bb,n)
float aa,bb;
int n;
float (*funk)(); /* ANSI: float (*funk)(float); */
{
        float x,tnm,sum,del,ddel,b,a;
        static float s;
        static int it;
        int j;

        b=1.0/aa;
        a=1.0/bb;
        if (n == 1) {
                it=1;
                return (s=(b-a)*((*funk)(1.0/(0.5*(a+b)))/((0.5*(a+b))*(0.5*(a+b)))));
        } else {
                tnm=it;
                del=(b-a)/(3.0*tnm);
                ddel=del+del;
                x=a+0.5*del;
                sum=0.0;
                for (j=1;j<=it;j++) {
                        sum += ((*funk)(1.0/(x))/((x)*(x)));
                        x += ddel;
                        sum += ((*funk)(1.0/(x))/((x)*(x)));
                        x += del;
                }
                it *= 3;
                return (s=(s+(b-a)*sum/tnm)/3.0);
        }
}
