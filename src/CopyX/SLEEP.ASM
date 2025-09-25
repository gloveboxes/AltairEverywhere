;Copyright (c) Microsoft Corporation. All rights reserved.
;Licensed under the MIT License.

      ORG 0100H   ;CP/M base of TPA (transient program area)
      MVI C,09H   ;Print string function
      LXI D,MSG   ;Point to waiting message
      CALL 0005H  ;Call bdos
      MVI A,2     ;Move 2 to the accumulator to set a 2 second delay
      OUT 30      ;Start timer
LOOP: IN 30       ;Get delay timer state into the accumulator
      CPI 00H     ;If accumulator equal to 0 then timer has expired
      JZ BACK     ;Jump on zero
      JMP LOOP
BACK: MVI C,09H   ;Print string function
      LXI D,PUB   ;Point to publish message
      CALL 0005H  ;Call bdos
      MVI A,0H    ;Move zero to the accumulator
      OUT 32      ;Publish to Azure IoT Central
      MVI C,09H   ;Print string function
      LXI D,FINI  ;Point to Finished message
      CALL 0005H  ;Call Bdos
      RET
MSG:  DB 'Sleeping 2 seconds$'
FINI: DB 0DH,0AH,'Finished$'
PUB:  DB 0DH,0AH,'Publishing to Azure IoT Central$' 
      END
