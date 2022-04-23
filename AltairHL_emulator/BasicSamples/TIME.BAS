10 WIDTH 100
20 PRINT "===================================================="
30 PRINT "TIME DEMOS"
40 SECONDS% = 1

50 WHILE 1
60 PRINT "==============================================="

70 PORT = 41 : GOSUB 160 : PRINT chr$(27) + "[91;22;24m" + "System tick count: ";RSTRING$;chr$(27) + "[0m"
80 PORT = 42 : GOSUB 160 : PRINT chr$(27) + "[92;22;24m" + "UTC date and time: ";RSTRING$;chr$(27) + "[0m"
90 PORT = 43 : GOSUB 160 : PRINT chr$(27) + "[94;22;24m" + "Local date and time: ";RSTRING$; chr$(27) + "[0m"

130 GOSUB 230

140 WEND
150 END

160 REM SUBROUTINE READS STRING DATA FROM PORT UNTIL NULL CHARACTER
170 RSTRING$ = ""
175 OUT PORT, 0
180 C=INP(200)
190 IF C = 0 THEN RETURN
200 RSTRING$ = RSTRING$ + CHR$(C)
210 GOTO 180

220 REM PAUSE FOR N NUMBER OF SECONDS
230 PRINT "" : PRINT "Pause";SECONDS%;"seconds."
240 OUT 30, SECONDS%
250 WAIT 30, 1, 1
260 RETURN

