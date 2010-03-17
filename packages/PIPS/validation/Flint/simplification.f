      PROGRAM SIMPLIF
      INTEGER I
      REAL*4 R
      REAL*8 D
      COMPLEX*8 C
      COMPLEX*16 DC

      I = INT(-590E-2)          !-5
      I = INT(-590D-2)          !-5
      I = INT((-590E-2, 3.4E2)) !-5
      I = INT(INT(I))           !I
      I = INT(INT(INT(R)))      !INT(R)

C     R -> I
      I = 5.9                   !5
      I = 5.9E2                 !590
      I = 590E-2                !5
      I = -5.9                  !-5
      I = -5.9E2                !-590
      I = -590E-2               !-5

C     I -> R
      R = R + 5 + 5.9 + I

C     D -> R
      R = +590.9D-2             !590.9E-2 
      R = -590.9D-2             !-590.9E-2

C     R -> D
      D = +5.9E2                !5.9D2
      D = -5.9E2                !-5.9D2

C     D -> I
      I = 5.9D2                 !590
      I = 590D-2                !5
      I = -5.9D2                !-590
      I = -590D-2               !-5

      I = -R
      R = -I
      I = -D
      D = -I

C     R -> C
      C = -5.9E2                !-(5.9E2, 0.0)
      C = 590E-2                !(590E-2, 0.0)

c     D -> C
      C = -5.9D2                !-(5.9E2, 0.0)
      C = 590D-2                !(590E-2, 0.0)

C     I -> C
      C = -5                    !-(5.0, 0.0)
      C = 5                     !(5.0, 0.0)

C     C -> R
      R = (5.9D2, 3.9D2)        !5.9E2
      R = (5.9E2, 3.9)          !5.9E2
      R = (-590.9E-1, 3.4)      !-590.9E-1
      R = -(590.9E-1, 3.4)      !-590.9E-1

C     C -> D
      D = (5.9E2, 3.4E2)        !5.9D2
      D = (590E-2, 3.4E2)       !590D-2
      D = (-590E-2, 3.4E2)      !-590D-2

C     C -> I
      I = (5.9E2, 3.4E2)        !590
      I = (590E-2, 3.4E2)       !5
      I = (-590E-2, 3.4E2)      !-5

      R = SIN(R)
      D = SIN(D)
      C = SIN(C)

C     DC -> C
      C = (-5.9E2, 3.9D2)       !(-5.9E2, 3.9E2)
      C = CMPLX((-5.9E2, 3.9D2)) !(-5.9E2, 3.9E2)
      DC = (-5.9E2, 3.9D2)      !(-5.9E2, 3.9E2)

C     C -> DC
      DC = (-5.9E2, -3.9D2)     !(-5.9D2, -3.9D2)
      DC = (5.9E2, -3.9E2)      !(5.9D2, -3.9D2)
      DC = DCMPLX((5.9E2, -3.9E2))      !(5.9D2, -3.9D2)

C     DC -> D
      D = (-5.9D2, -3.9D2)      !-5.9D2

C     DC -> R
      R = (-5.9D2, -3.9D2)      !-5.9E2

C     DC -> I
      I = (-5.9D2, -3.9D2)      !-590
      
C     I -> DC
      DC = 5                    !(5.0, 0.0)
      DC = -5.9E2               !-(5.9D2, 0.0)
      DC = -5.9D2               !-(5.9D2, 0.0)

C others...
      
      I = REAL(DBLE((10.0D0,-10.0D0)))

      END






