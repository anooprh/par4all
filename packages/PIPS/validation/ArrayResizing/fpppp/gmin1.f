      FUNCTION GMIN1 (ARG1,ARG2)
C
C     MAXIMUM VALUE OF TWO ARGUMENTS
C     NOTE THAT THIS DOES NOT UTILIZE THE FULL CAPABILITY OF THE
C     LIBRARY FUNCTIONS IN THAT THE NUMBER OF ARGUMENTS IS NOT LIMITED
C     TO TWO. (AT LEAST ON THE VAX).
C
      REAL*8 GMIN1,DMIN1,ARG1,ARG2
C
      GMIN1 = DMIN1 (ARG1,ARG2)
      RETURN
      END
