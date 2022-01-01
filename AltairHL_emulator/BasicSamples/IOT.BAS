10 PRINT
20 PRINT "Avnet onboard temperature and pressure sensor App"
30 PRINT
70 A$=""
80 C=INP(43)
90 IF C = 0 THEN GOTO 120
100 A$=A$+CHR$(C)
110 GOTO 80
120 PRINT "Temperature is ";A$;" degrees Celsius."
130 A$=""
140 C=INP(44)
150 IF C = 0 THEN GOTO 180
160 A$=A$+CHR$(C)
170 GOTO 140
180 PRINT "Air pressure is ";A$;" hPa."
210 PRINT
