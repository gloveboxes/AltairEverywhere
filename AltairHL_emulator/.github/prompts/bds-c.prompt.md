# BDS C Development on CP/M - Complete Guide

## Overview

This guide captures essential knowledge for developing C programs using the BDS C 1.6 compiler on CP/M systems, based on practical experience building a Pong game for the Altair 8800 emulator.

## BDS C 1.6 Compiler Syntax Requirements

### Function Declarations

BDS C requires **old-style C function declarations** with separate parameter lists:

```c
// CORRECT - BDS C style
int my_function(param1, param2)
int param1;
char param2;
{
    return param1 + param2;
}

// INCORRECT - Modern C style (won't compile)
int my_function(int param1, char param2) {
    return param1 + param2;
}
```

### Return Type Declarations

**All functions must explicitly declare return types**, even for `int`:

```c
// CORRECT
int main()
{
    return 0;
}

// INCORRECT (missing return type)
main()
{
    return 0;
}
```

### Variable Declarations

- All variables must be declared at the **beginning of functions** or as globals
- No variable declarations in the middle of code blocks
- BDS C doesn't support inline initialization in declarations

```c
// CORRECT
int my_function()
{
    int i, j, k;    // All variables declared at top
    char buffer[10];
    
    i = 5;          // Initialize after declaration
    // ... rest of function
}

// INCORRECT
int my_function()
{
    int i = 5;      // Inline initialization not supported
    // some code
    int j;          // Declaration in middle of function
}
```

## CP/M System Interface

### BIOS/BDOS Function Calls

Use these function prototypes for system calls:

```c
int bdos();   /* int bdos(function, de_register) */
int bios();   /* int bios(function, c_register)  */
int biosh();  /* int biosh(function, bc_register, de_register)  */
```

### Console I/O Functions

#### Character Mode Input (Immediate Response)

```c
int get_immediate_key()
{
    int ch;
    ch = bdos(6, 0xFF) & 0xFF;   /* Direct console I/O */
    return ch;                   /* Returns 0 if no key pressed */
}
```

#### Line Mode Input (Requires Enter)

```c
int get_line_key()
{
    return bdos(1, 0) & 0xFF;    /* Console input - waits for Enter */
}
```

#### Character Output

```c
int put_char(c)
char c;
{
    return bios(4, c);           /* BIOS console output */
}
```

## Terminal Control Challenges

### ANSI Escape Sequences

**Major Issue**: Modern terminal emulators (like xterm.js) may not properly interpret ANSI escape sequences sent from CP/M programs, causing them to display as text rather than execute as commands.

#### Problematic Approach

```c
// These may display as text: "ESC[2;3H" instead of positioning cursor
chput(27); chput('['); chput('2'); chput(';'); chput('3'); chput('H');
```

#### Safer Alternatives

1. **Form Feed for Screen Clear**:

```c
chput(12);  /* ASCII Form Feed - universally supported */
```

2. **BIOS Cursor Positioning** (if supported):

```c
biosh(2, 0, ((row-1) << 8) | (col-1));  /* BIOS set cursor position */
```

3. **Avoid Complex Cursor Control**: Design programs to work with sequential output when possible.

### Display Strategy Recommendations

#### For Simple Programs

Use sequential output with Form Feed clearing:

```c
int refresh_screen()
{
    chput(12);      /* Clear screen */
    draw_borders();
    draw_content();
    return 0;
}
```

#### For Interactive Programs  

Minimize screen updates and use character-level changes:

```c
/* Only update characters that have actually changed */
if (old_char != new_char) {
    position_cursor_somehow(row, col);
    chput(new_char);
    old_char = new_char;
}
```

## Memory and Performance Considerations

### Global Variables

- Prefer global variables over local ones for frequently accessed data
- BDS C has limited stack space

### String Handling

```c
// Simple string output function
int put_string(s)
char *s;
{
    while (*s) {
        chput(*s++);
    }
    return 0;
}
```

### Number Output

```c
int put_number(n)
int n;
{
    char buf[6];
    int i;
    
    if (n == 0) {
        chput('0');
        return 0;
    }
    
    i = 0;
    while (n > 0 && i < 6) {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    
    while (i--) {
        chput(buf[i]);
    }
    return 0;
}
```

## Game Development Patterns

### Game Loop Structure

```c
int main()
{
    int running, input;
    
    /* Initialize game state */
    init_game();
    
    running = 1;
    while (running) {
        /* Check for input (non-blocking) */
        input = bdos(6, 0xFF) & 0xFF;
        if (input != 0) {
            if (input == 'q') running = 0;
            handle_input(input);
        }
        
        /* Update game logic */
        update_game();
        
        /* Render (minimize screen updates) */
        render_changes();
        
        /* Simple delay */
        delay_loop();
    }
    
    cleanup();
    return 0;
}
```

### Timing and Delays

```c
int delay(ticks)
int ticks;
{
    int i, j;
    for (i = 0; i < ticks; i++) {
        for (j = 0; j < 400; j++) {
            /* Empty loop for delay */
        }
    }
    return 0;
}
```

## Common Pitfalls and Solutions

### 1. Compilation Errors

- **"Missing semicolon"**: Check function parameter declarations
- **"Undeclared identifier"**: Ensure all variables are declared as globals or at function start
- **"Redeclaration"**: Remove duplicate function definitions

### 2. Display Issues

- **Control characters visible**: ANSI sequences not supported - use simpler methods
- **Flickering display**: Minimize full-screen redraws
- **Key echo**: Keys appearing on screen before being processed

### 3. Input Problems

- **Need to press Enter**: Using line-buffered input instead of character mode
- **No response to keys**: Not using `bdos(6, 0xFF)` for immediate input

## Best Practices Summary

1. **Keep it Simple**: Use the most basic I/O methods that work reliably
2. **Test Incrementally**: Build complex programs step by step
3. **Minimize Screen Updates**: Only redraw what has changed
4. **Use Globals Wisely**: For game state and frequently accessed data
5. **Plan for Limitations**: Work within BDS C's constraints rather than against them
6. **Fallback Strategies**: Have simple alternatives when advanced features don't work

## Example Project Structure

```
MyGame/
├── main.c          // Main game loop and initialization
├── input.c         // Input handling functions  
├── graphics.c      // Display and drawing functions
├── game.c          // Game logic and state management
└── utils.c         // Utility functions (delays, string handling)
```

## Compilation Workflow

```bash
# On CP/M system
B>cc main          # Compile main.c
B>cc input         # Compile input.c
B>cc graphics      # Compile graphics.c
B>cc game          # Compile game.c
B>cc utils         # Compile utils.c
B>clink main input graphics game utils    # Link all objects
B>main             # Run the program
```

---

*This guide is based on practical experience developing a Pong game using BDS C 1.6 on CP/M in an Altair 8800 emulator environment with xterm.js terminal emulation.*
