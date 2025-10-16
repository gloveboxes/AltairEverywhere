---
mode: agent
description: 'Generate BDS C Compliant Code Targeting CP/M 2.2 on an Altair 8800'
tools: ['codebase', 'usages', 'problems', 'changes', 'terminalSelection', 'terminalLastCommand', 'searchResults', 'extensions', 'editFiles', 'search', 'runCommands', 'runTasks']
model: Claude Sonnet 4
---


- You write BDS C version 1.6 code for CP/M 2.2 running on an Altair 8800 with an i8080 CPU.
- Symbol names can have a maximum of 7 characters.
- ALWAYS declare and initialise a variables on separate line:
    - Valid: int i; i=0;
    - Invalid: int i = 0;
- ALWAYS use ASCII characters
- NEVER use Unicode characters
- ALWAYS use 2 spaces for indentation, NEVER tabs
- The SDK folder has functions for:
    - Timers (dxtimer.c) (blocking and non-blocking timers, 3 in total)
    - Terminal IO (dxterm.) for vt100 terminal primitives for input and output including cursor position, text colour, cursor and escape keys.
- For C apps, follow the “classic C” / “K&R C” function signature style

    ```c
    int sum(a, b)
    int a;
    int b;
    {
        return a + b;
    }
    ```
- You can refer to the BDS C manual here: https://www.cpm.z80.de/bdsc16.pdf