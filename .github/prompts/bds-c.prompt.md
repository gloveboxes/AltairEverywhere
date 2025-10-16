---
mode: agent
description: 'Generate BDS C Compliant Code Targeting CP/M 2.2 on an Altair 8800'
tools: ['codebase', 'usages', 'problems', 'changes', 'terminalSelection', 'terminalLastCommand', 'searchResults', 'extensions', 'editFiles', 'search', 'runCommands', 'runTasks']
model: Claude Sonnet 4
---

- You write BDS C version 1.6 code for CP/M 2.2 running on an Altair 8800 with an i8080 CPU.
- Symbols including variable names and function names **MUST** be 7 or less characters.
- ALWAYS declare and initialise a variables on separate line:
    - **Valid**: int i; i=0;
    - **Invalid**: int i = 0;
- ALWAYS use ASCII characters
- NEVER use Unicode characters
- ALWAYS use 2 spaces for indentation, NEVER tabs
- The SDK folder has functions for:
    - Timers (dxtimer.c) (blocking and non-blocking timers, 3 in total)
    - Terminal IO (dxterm.c) for vt100 terminal primitives for input and output including cursor position, text colour, cursor and escape keys.
    - The vt100 terminal is provided by xterm.js, refer to https://github.com/xtermjs/xterm.js/ for details.
- For C apps, follow the “classic C” / “K&R C” function signature style

    ```c
    int sum(a, b)
    int a;
    int b;
    {
        return a + b;
    }
    ```

- Refer to the following example applications in the AppSamples folder for guidance:
    - weather.c - a simple weather app that uses the serial port
    - tetris.c - a more complex app that uses the terminal IO functions
    - breakout.c - a game that uses the terminal IO functions and timers
    - onboard.c - a simple app that reports on PiSense Hat onboard sensors
- Never assume BDS C syntax, if in doubt, refer to the BDS C Compiler reference is included the AppSamples folder
- Always refer to the CP/M BDOS API for system calls, it is included in the AppSamples folder
- For long integers see sdk/LONG.c and link with LONG.CRL on the Altair
