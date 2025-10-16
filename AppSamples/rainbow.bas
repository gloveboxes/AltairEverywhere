100 DIM C(3): C[0] = 91 : C[1] = 92 : C[2] = 94
200 M = 0
300 FOR I = 0 TO 999999!
400 FOR J = 0 TO 999999!
500 PRINT CHR$(27) + "[" + MID$(STR$(C[M]), 2) + ";22;24m";
600 PRINT "Loop: ";I;" Counter: ";J
700 M = M + 1 : IF M > 2 THEN M = 0
800 NEXT J
900 NEXT I

