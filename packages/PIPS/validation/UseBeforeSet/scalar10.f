C       Uninitialized scalar variables
	PROGRAM SCALAR10
	INTEGER	ITMAX
	REAL*8	EPS
	PARAMETER        ( ITMAX = 1000)
	PARAMETER        ( EPS   = 0.5D-8 )
	REAL*8	RXM(ITMAX), RYM(ITMAX)
	REAL*8	ABX, ABY
	ABX  = ABS(RXM(ITER))
        ABY  = ABS(RYM(ITER))
        IF (ABX.LE.EPS.AND.ABY.LE.EPS)  GOTO  150
 150	CONTINUE 
        END
