c     Bug with cr2cnf.f, modified version

      SUBROUTINE TESFRO(A,B,AF,X,Y,N,NC,C,Z)
      REAL*4 T1,T2,TMP(2),ETIME
      IMPLICIT REAL*8(A-H,O-Z)
      DIMENSION A(N,N,3),B(N,N),AF(N,N,NC),X(N),Y(N),C(N,N),Z(N)
      T1=ETIME(TMP)
      END

      function etime(tmp)
      etime=0.3
      return
      END
