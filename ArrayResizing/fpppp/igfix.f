      FUNCTION IGFIX(R)
      IMPLICIT REAL*8(A-H,O-Z)
C
C     CONVERT WORKING PRECISION TO INTEGER.
C
      IGFIX = IFIX(SNGL(R))
      RETURN
      END