/*
 * VT100/xterm.js arrow key test for BDS C on CP/M.
 * Moves an 'X' on the screen using arrow keys.
 * Displays raw key byte sequences for debugging.
 * Q to quit.
 */

/* BDS C library hooks */
int bdos();
int bios();

/* Key Codes */
#define KEY_ESC   27
#define KEY_UP    1001
#define KEY_DOWN  1002
#define KEY_LEFT  1003
#define KEY_RIGHT 1004

/* Screen Boundaries */
#define MIN_ROW 8
#define MAX_ROW 20
#define MIN_COL 5
#define MAX_COL 70

/* Globals for key sequence debugging */
int g_last_seq[4];
int g_seq_len;
int g_last_key_code;

/* --- Console I/O --- */

int chput(c)
char c;
{
    return bios(4, c);
}

int cputs(s)
char *s;
{
    while (*s) chput(*s++);
    return 0;
}

int putnum(n)
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
    while (i--) chput(buf[i]);
    return 0;
}

/* --- VT100/xterm.js Screen Control --- */

int cursor_move(row, col)
int row;
int col;
{
    chput(KEY_ESC);
    cputs("[");
    putnum(row);
    cputs(";");
    putnum(col);
    cputs("H");
    return 0;
}

int clear_screen()
{
    chput(KEY_ESC);
    cputs("[2J");
    cursor_move(1, 1);
    return 0;
}

int hide_cursor()
{
    chput(KEY_ESC);
    cputs("[?25l");
    return 0;
}

int show_cursor()
{
    chput(KEY_ESC);
    cputs("[?25h");
    return 0;
}

/* --- Drawing --- */

int draw_instructions()
{
    cursor_move(1, 1);
    cputs("VT100/xterm.js Arrow Key Test\r\n");
    cputs("Use arrow keys to move the X. Press Q to quit.\r\n");
    cputs("---------------------------------------------\r\n");
    return 0;
}

int draw_marker(row, col)
int row;
int col;
{
    cursor_move(row, col);
    chput('X');
    return 0;
}

int erase_marker(row, col)
int row;
int col;
{
    cursor_move(row, col);
    chput(' ');
    return 0;
}

/* --- Status & Debugging --- */

int print_last_sequence()
{
    int i;
    if (g_seq_len == 0) {
        cputs("n/a");
        return 0;
    }
    for (i = 0; i < g_seq_len; i++) {
        putnum(g_last_seq[i]);
        if (i + 1 < g_seq_len) cputs(" ");
    }
    return 0;
}

int update_status(row, col)
int row;
int col;
{
    int printable;
    cursor_move(4, 1);
    cputs("Position: ");
    putnum(row);
    cputs(", ");
    putnum(col);
    cputs("          ");

    cursor_move(5, 1);
    cputs("Last Key Bytes: ");
    print_last_sequence();
    cputs("          ");

    cursor_move(6, 1);
    cputs("Decoded: ");
    putnum(g_last_key_code);
    printable = 0;
    if (g_last_key_code >= 32 && g_last_key_code <= 126) printable = 1;
    if (printable) {
        cputs(" ('");
        chput(g_last_key_code);
        cputs("')");
    }
    if (g_last_key_code == KEY_UP) cputs(" (UP)");
    else if (g_last_key_code == KEY_DOWN) cputs(" (DOWN)");
    else if (g_last_key_code == KEY_LEFT) cputs(" (LEFT)");
    else if (g_last_key_code == KEY_RIGHT) cputs(" (RIGHT)");
    else if (g_last_key_code == KEY_ESC) cputs(" (ESC)");
    cputs("            ");

    cursor_move(row, col);
    return 0;
}

/* --- Input Handling --- */

int get_immediate_char()
{
    int ch;
    ch = 0;
    while (ch == 0) {
        ch = bdos(6, 0xFF) & 0xFF; /* Direct, non-blocking console I/O */
    }
    return ch;
}

int read_input_key()
{
    int ch;
    int next;
    int final;

    ch = get_immediate_char();
    g_seq_len = 1;
    g_last_seq[0] = ch;

    if (ch == KEY_ESC) {
        /* Check for a following byte, but don't wait forever */
        if ((bdos(11) & 0xFF) == 0) return KEY_ESC;
        next = get_immediate_char();
        
        g_last_seq[1] = next;
        g_seq_len = 2;

        if (next == '[') {
            if ((bdos(11) & 0xFF) == 0) return KEY_ESC;
            final = get_immediate_char();
            
            g_last_seq[2] = final;
            g_seq_len = 3;

            if (final == 'A') return KEY_UP;
            if (final == 'B') return KEY_DOWN;
            if (final == 'C') return KEY_RIGHT;
            if (final == 'D') return KEY_LEFT;
        }
        return KEY_ESC;
    }
    return ch;
}

/* --- Main Program --- */

int main()
{
    int ch;
    int row, col;
    int old_row, old_col;
    int running;

    g_seq_len = 0;
    g_last_key_code = 0;

    hide_cursor();
    clear_screen();
    draw_instructions();

    row = 12;
    col = 40;
    draw_marker(row, col);
    update_status(row, col);

    running = 1;
    while (running) {
        ch = read_input_key();
        g_last_key_code = ch;

        old_row = row;
        old_col = col;

        if (ch == 'q' || ch == 'Q') {
            running = 0;
        }
        else if (ch == KEY_UP || ch == 5) { /* CTRL+E */
            if (row > MIN_ROW) row--;
        }
        else if (ch == KEY_DOWN || ch == 24) { /* CTRL+X */
            if (row < MAX_ROW) row++;
        }
        else if (ch == KEY_LEFT || ch == 19) { /* CTRL+S */
            if (col > MIN_COL) col--;
        }
        else if (ch == KEY_RIGHT || ch == 4) { /* CTRL+D */
            if (col < MAX_COL) col++;
        }

        if (row != old_row || col != old_col) {
            erase_marker(old_row, old_col);
            draw_marker(row, col);
        }

        update_status(row, col);
    }

    show_cursor();
    cursor_move(23, 1);
    cputs("\r\nGoodbye!\r\n");
    return 0;
}