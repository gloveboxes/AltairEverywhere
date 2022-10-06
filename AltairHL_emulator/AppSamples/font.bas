100 REM 8x8 LED Panel Demo
200 OUT 80,1 : REM Flip the 8x8 LED panel to FONT mode
300 FOR J = 1 to 10
400 FOR I = 33 TO 122
500 OUT 81, I MOD 3 : REM Cycle font color
600 OUT 85, I : REM Display character on the 8x8 LED panel
700 PRINT CHR$(I)
800 OUT 29, 250 : WAIT 29, 1, 1 : REM Pause for 250 milliseconds
900 NEXT I
1000 NEXT J
1100 OUT 80,0

