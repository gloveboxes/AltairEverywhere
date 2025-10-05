; CP/M / ASM.COM constraints
;  - Labels / symbols must be unique after 6 chars (ASM truncates names)
;  - BDOS entry is fixed at 0005H; use CALL BDOS with C set to fn code
;  - Port constants are decimal because ASM treats leading zero hex oddly
; Simple CP/M sample app for Altair 8800 / Intel 8080
; Prints "Hello World - loop number n." up to 1000 times with 5-second pauses.

        ORG     0100H          ; Standard CP/M program load address

BDOS    EQU     0005H           ; Entry point for CP/M BDOS calls
TMRHI   EQU     28              ; Timer high-byte port (decimal)
TMRLO   EQU     29              ; Timer low-byte port (starts timer countdown)

START:                          ; Program entry point (CP/M jumps here)
        LXI     H,1000          ; Initialize remaining loop counter (1000 passes)
        SHLD    LOPCNT
        LXI     H,1             ; Start with loop number 1
        SHLD    LOPNUM

MAINLP:                         ; Print message, wait 5s, advance counters
        LHLD    LOPCNT          ; Stop after 1000 iterations
        MOV     A,H
        ORA     L               ; Test remaining loop count
        JZ      DONE

        LXI     D,MSGTXT        ; Print base message text via BDOS function 9
        CALL    PRSTR

        LHLD    LOPNUM          ; Append current loop number (1-1000)
        CALL    PRDEC

        MVI     A,'.'           ; Add trailing period and newline
        CALL    PRCHR
        MVI     A,0DH
        CALL    PRCHR
        MVI     A,0AH   
        CALL    PRCHR

        CALL    WAIT5           ; Wait 5,000 milliseconds (ESC cancels wait)
        ORA     A               ; Non-zero return from WAIT5 means ESC pressed
        JNZ     DONE

        LHLD    LOPNUM          ; Prepare next iteration
        INX     H
        SHLD    LOPNUM

        LHLD    LOPCNT          ; Decrement remaining loop count
        DCX     H
        SHLD    LOPCNT
        JMP     MAINLP

DONE:                           ; Gracefully return control to CP/M
        MVI     C,0             ; Return to CP/M BDOS function 0 (terminate)
        CALL    BDOS

; ------------------------------------------------------------
PRSTR:                          ; DE -> '$'-terminated string (BDOS fn 9)
        MVI     C,9
        CALL    BDOS
        RET

PRCHR:                          ; A holds character to output (BDOS fn 2)
        MOV     E,A
        MVI     C,2
        CALL    BDOS
        RET

PRDEC:                          ; HL holds value (1-1000) to print in decimal
        MVI     D,0             ; Track if we've printed a non-zero digit
        LXI     B,1000
        CALL    GETDG
        PUSH    H              ; Preserve remainder across BDOS call
        CALL    OUTDIG
        POP     H
        LXI     B,100
        CALL    GETDG
        PUSH    H
        CALL    OUTDIG
        POP     H
        LXI     B,10
        CALL    GETDG
        PUSH    H
        CALL    OUTDIG
        POP     H
        MOV     A,L            ; Ones place (always printed)
        ADI     '0'
        CALL    PRCHR
        RET

OUTDIG:                         ; Print digit in A if needed (skips leading zeros)
        MOV     E,A
        ORA     A
        JNZ     ODDO
        MOV     A,D
        ORA     A
        RZ
        MOV     A,E
ODDO:   ADI     '0'
        CALL    PRCHR
        MVI     D,1
        RET

GETDG:                          ; Count how many times BC fits in HL
        MVI     E,0
GDLOP:
        MOV     A,H
        CMP     B
        JC      GDDNE
        JNZ     GDSUB
        MOV     A,L
        CMP     C
        JC      GDDNE
GDSUB:
        MOV     A,L
        SUB     C
        MOV     L,A
        MOV     A,H
        SBB     B
        MOV     H,A
        INR     E
        JMP     GDLOP
GDDNE:
        MOV     A,E
        RET

WAIT5:
        MVI     A,50            ; 50 chunks * 100ms ~= 5000ms
        STA     CHCNT
W5LOOP:
        CALL    W0100           ; Wait for ~100ms to elapse
        MVI     E,0FFH          ; Poll console (no wait) using BDOS fn 6
        MVI     C,6
        CALL    BDOS
        CPI     1BH             ; ESC key pressed?
        JZ      W5ESC
        LDA     CHCNT
        DCR     A
        STA     CHCNT
        JNZ     W5LOOP
        XRA     A               ; Timer completed, return with A=0
        RET
W5ESC:
        MVI     A,1             ; Signal ESC pressed (non-zero return)
        RET

W0100:
        MVI     A,0             ; High byte for ~100ms duration
        OUT     TMRHI
        MVI     A,64H           ; Low byte (starts timer countdown)
        OUT     TMRLO
W1STR:
        IN      TMRLO           ; Wait for timer to start counting down
        ORA     A
        JZ      W1STR
W1CHK:
        IN      TMRLO           ; Poll until the timer reaches zero again
        ORA     A
        JNZ     W1CHK
        RET

; ------------------------------------------------------------
LOPCNT:         DW      0000H
LOPNUM:         DW      0000H
CHCNT:          DB      00H

MSGTXT:         DB      'Vibe coded with OpenAI Codex - press esc to exit - loop number ', '$'

        END     START
