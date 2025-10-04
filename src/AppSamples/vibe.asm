; CP/M / ASM.COM constraints
;  - Labels / symbols must be unique after 6 chars (ASM truncates names)
;  - BDOS entry is fixed at 0005H; use CALL BDOS with C set to fn code
;  - Port constants are decimal because ASM treats leading zero hex oddly
; Simple CP/M sample app for Altair 8800 / Intel 8080
; Prints "Hello World - loop number n." ten times with 5-second pauses.

        ORG     0100H

BDOS    EQU     0005H
TMRHI   EQU     28              ; Timer high-byte port (decimal)
TMRLO   EQU     29              ; Timer low-byte port (starts timer)

START:
        MVI     A,10            ; Initialize remaining loop counter
        STA     LOPCNT
        MVI     A,1             ; Start with loop number 1
        STA     LOPNUM

MAINLP:
        LDA     LOPCNT          ; Stop after ten iterations
        ORA     A
        JZ      DONE

        LXI     D,MSGTXT        ; Print base message text
        CALL    PRSTR

        LDA     LOPNUM          ; Append current loop number
        CALL    PRDEC

        MVI     A,'.'           ; Add trailing period and newline
        CALL    PRCHR
        MVI     A,0DH
        CALL    PRCHR
        MVI     A,0AH
        CALL    PRCHR

        CALL    WAIT5           ; Wait 5,000 milliseconds (ESC cancels)
        ORA     A
        JNZ     DONE

        LDA     LOPNUM          ; Prepare next iteration
        INR     A
        STA     LOPNUM

        LDA     LOPCNT
        DCR     A
        STA     LOPCNT
        JMP     MAINLP

DONE:
        MVI     C,0             ; Return to CP/M BDOS
        CALL    BDOS

; ------------------------------------------------------------
PRSTR:                          ; DE -> '$'-terminated string
        MVI     C,9
        CALL    BDOS
        RET

PRCHR:                          ; A holds character to output
        MOV     E,A
        MVI     C,2
        CALL    BDOS
        RET

PRDEC:                          ; A holds value (1-10)
        CPI     0AH
        JZ      PRTEN
        ADI     '0'
        CALL    PRCHR
        RET

PRTEN:
        MVI     A,'1'
        CALL    PRCHR
        MVI     A,'0'
        CALL    PRCHR
        RET

WAIT5:
        MVI     A,50            ; 50 chunks * 100ms ~= 5000ms
        STA     CHCNT
W5LOOP:
        CALL    W0100           ; Wait for ~100ms
        MVI     E,0FFH          ; Poll console (no wait)
        MVI     C,6
        CALL    BDOS
        CPI     1BH
        JZ      W5ESC
        LDA     CHCNT
        DCR     A
        STA     CHCNT
        JNZ     W5LOOP
        XRA     A               ; Timer completed
        RET
W5ESC:
        MVI     A,1             ; Signal ESC pressed
        RET

W0100:
        MVI     A,0             ; High byte for ~100ms
        OUT     TMRHI
        MVI     A,64H           ; Low byte (starts timer)
        OUT     TMRLO
W1STR:
        IN      TMRLO           ; Wait for timer to start counting
        ORA     A
        JZ      W1STR
W1CHK:
        IN      TMRLO
        ORA     A
        JNZ     W1CHK
        RET

; ------------------------------------------------------------
LOPCNT:         DB      00H
LOPNUM:         DB      00H
CHCNT:          DB      00H

MSGTXT:         DB      'Vibe Coded with OpenAI Codex - Hello World - loop number ', '$'

        END     START
