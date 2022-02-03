05 PRINT
10 PRINT "Open Weather Map IoT App"
20 REM FETCH THE LOCATION AND WEATHER DATA
25 FOR I = 1 to 1000000
26 PRINT "==================================="
27 PRINT "Reading:";I
30 OUT 32,1
40 REM PUBLISH THE TEMPERATURE
50 OUT 32,2
60 REM GET THE TEMPERATURE AS A STRING
70 A$=""
80 C=INP(45)
90 IF C = 0 THEN GOTO 120
100 A$=A$+CHR$(C)
110 GOTO 80
120 PRINT "Temperature: ";A$;" celsius"
130 A$=""
140 C=INP(46)
150 IF C = 0 THEN GOTO 180
160 A$=A$+CHR$(C)
170 GOTO 140
180 PRINT "Air pressure: ";A$;" millibars"
190 A$=""
200 C=INP(47)
210 IF C = 0 THEN GOTO 240
220 A$=A$+CHR$(C)
230 GOTO 200
240 PRINT "Humidity: ";A$;"%"
250 A$=""
260 C=INP(48)
270 IF C = 0 THEN GOTO 300
280 A$=A$+CHR$(C)
290 GOTO 260
300 PRINT "Observation: ";A$
340 REM 1000000 iterations makes for about 6 second delay when Altair built in release mode
350 FOR J = 1 to 1000000: NEXT J
400 NEXT I
