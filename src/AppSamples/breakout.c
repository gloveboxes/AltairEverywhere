/*
 * VT100/xterm.js arrow key test for BDS C on CP/M.
 * Moves an 'X' onint draw_instructions()
{
    cursor_move(1, 1);
    cputs("VT100/xterm.js Ball and Paddle Game\r\n");
    cputs("LEFT/RIGHT cursor keys move ---- (paddle). Keep the ball from going out!\r\n");
    cputs("Press Q to quit.\r\n");

    return 0;
}en using arrow keys.
 * Displays raw key byte sequences for debugging.
 * Q to quit.
 */

/* Key Codes */
#define KEY_ESC      27
#define CTRL_C       0x03
#define RIGHT_CURSOR 4
#define LEFT_CURSOR  19

/* Screen Boundaries */
#define MIN_ROW 6
#define MAX_ROW 30
#define MIN_COL 5
#define MAX_COL 60

/* Globals for key sequence debugging */
int g_last_seq[4];
int g_seq_len;
int g_last_key_code;

/* Two independent objects */
int x_row, x_col; /* Ball position */
int y_row, y_col; /* Paddle position */

/* Ball physics */
int ball_dx, ball_dy; /* Ball direction: -1 or 1 */
int score;

/* Random number tracking */
int last_random; /* Last random number generated for display */

/* Ball speed control */
int bounce_count;  /* Number of bounces in current game */
int speed_counter; /* Counter for variable speed timing */
int base_speed;    /* Base speed cycles (4 = 100ms) */

/* Timing counters for cooperative multitasking */
int ball_counter;   /* Counter for ball movement timing */
int paddle_counter; /* Counter for paddle input timing */
int status_counter; /* Counter for status display timing */

/* Collision handling */
int paddle_needs_redraw; /* Flag to force paddle redraw after collision */

/* Timer library functions */
int x_delay();
int x_tmrset();
int x_tmrexp();

/* --- Console I/O --- */

int chput(c)
char c;
{
    return bios(4, c);
}

int cputs(s)
char *s;
{
    while (*s)
        chput(*s++);
    return 0;
}

int putnum(n)
int n;
{
    char buf[6];
    int i;
    if (n == 0)
    {
        chput('0');
        return 0;
    }
    i = 0;
    while (n > 0 && i < 6)
    {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    while (i--)
        chput(buf[i]);
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
    chput(KEY_ESC);
    cputs("[0m");
    cursor_move(1, 1);
    return 0;
}

int disable_local_echo()
{
    chput(KEY_ESC);
    cputs("[?12l");
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

int draw_walls()
{
    int i;

    /* Draw top wall */
    cursor_move(MIN_ROW - 1, MIN_COL - 1);
    for (i = MIN_COL - 1; i <= MAX_COL + 1; i++)
    {
        chput('-');
    }

    /* Draw side walls */
    for (i = MIN_ROW; i <= MAX_ROW; i++)
    {
        cursor_move(i, MIN_COL - 1);
        chput('|');
        cursor_move(i, MAX_COL + 1);
        chput('|');
    }

    /* Draw bottom boundary (no wall - where ball goes out) */
    cursor_move(MAX_ROW + 1, MIN_COL - 1);
    for (i = MIN_COL - 1; i <= MAX_COL + 1; i++)
    {
        chput('.');
    }

    return 0;
}

/* --- Drawing --- */

int draw_instructions()
{
    cursor_move(1, 1);
    cputs("VT100/xterm.js Ball and Paddle Game\r\n");
    cputs("LEFT/RIGHT keys move ---- (paddle). Keep the ball from going out!\r\n");
    cputs("Press ESC to quit.\r\n");
    return 0;
}

int draw_x_marker(row, col)
int row;
int col;
{
    cursor_move(row, col);
    chput('O');
    return 0;
}

int erase_x_marker(row, col)
int row;
int col;
{
    cursor_move(row, col);
    chput(' ');
    return 0;
}

int draw_y_marker(row, col)
int row;
int col;
{
    cursor_move(row, col);
    cputs("------"); /* Draw entire paddle in one operation */
    return 0;
}

int erase_y_marker(row, col)
int row;
int col;
{
    cursor_move(row, col);
    cputs("      "); /* Erase entire paddle in one operation */
    return 0;
}

/* --- Status & Debugging --- */

int print_last_sequence()
{
    int i;
    if (g_seq_len == 0)
    {
        cputs("n/a");
        return 0;
    }
    for (i = 0; i < g_seq_len; i++)
    {
        putnum(g_last_seq[i]);
        if (i + 1 < g_seq_len)
            cputs(" ");
    }
    return 0;
}

int update_status()
{
    cursor_move(5, 1);
    cputs("Score: ");
    putnum(score);
    cputs("   Bounces: ");
    putnum(bounce_count);
    cputs(" ");
    return 0;
}

/* --- Game Logic --- */

int check_paddle_hit()
{
    /* Check if ball is at paddle row and within paddle range */
    if (x_row == y_row && x_col >= y_col && x_col <= y_col + 5)
    {
        return 1;
    }
    return 0;
}

int check_future_paddle_hit(future_row, future_col)
int future_row;
int future_col;
{
    if (ball_dy > 0 && future_row == y_row && future_col >= y_col && future_col <= y_col + 5)
    {
        return 1;
    }
    return 0;
}

int start_game_loop_timer()
{
    /* Start a 50ms game loop timer */
    x_tmrset(50);
    return 0;
}

int is_game_loop_timer_expired()
{
    /* Check if the 50ms timer has expired - returns 1 when expired */
    return (x_tmrexp() == 0);
}

int should_move_ball()
{
    int speed_cycles;
    int effective_bounces;

    /* Cap bounces at 20 for maximum speed limit */
    effective_bounces = bounce_count;
    if (effective_bounces > 20)
        effective_bounces = 20;

    /* Calculate current speed: base speed reduced by bounce count */
    speed_cycles = base_speed - (effective_bounces / 10);
    if (speed_cycles < 1)
        speed_cycles = 1; /* Minimum speed: 1 cycle (50ms) */

    /* Adjust for 50ms timer resolution */
    return (ball_counter >= speed_cycles);
}

int abs(n)
int n;
{
    return (n < 0) ? -n : n;
}

int string_to_int(s)
char *s;
{
    int result;
    int sign;

    result = 0;
    sign   = 1;

    /* Skip leading spaces */
    while (*s == ' ')
        s++;

    /* Check for sign */
    if (*s == '-')
    {
        sign = -1;
        s++;
    }
    else if (*s == '+')
    {
        s++;
    }

    /* Convert digits */
    while (*s >= '0' && *s <= '9')
    {
        result = result * 10 + (*s - '0');
        s++;
    }

    return result * sign;
}

int get_random()
{
    char random_str[16];
    int i;
    int ch;
    int result;

    /* Trigger hardware random number generation */
    outp(44, 1);

    /* Read the random number string from port 200 */
    i = 0;
    while (i < 15)
    { /* Leave room for null terminator */
        ch = inp(200);
        if (ch == 0)
            break; /* Null terminator - end of string */
        random_str[i] = ch;
        i++;
    }
    random_str[i] = 0; /* Null terminate the string */

    /* Convert string to integer and make it positive */
    result = string_to_int(random_str);
    return abs(result); /* Always return positive number (0-32000) */
}

int update_ball_position()
{
    int next_row;
    int next_col;

    /* Predict next position */
    next_row = x_row + ball_dy;
    next_col = x_col + ball_dx;

    /* Ball bounces off top */
    if (next_row <= MIN_ROW)
    {
        next_row = MIN_ROW;
        ball_dy  = 1;
        /* No bounce count increase for wall bounces */
    }

    /* Ball bounces off sides */
    if (next_col <= MIN_COL)
    {
        next_col = MIN_COL;
        ball_dx  = 1;
        /* No bounce count increase for wall bounces */
    }
    else if (next_col >= MAX_COL)
    {
        next_col = MAX_COL;
        ball_dx  = -1;
        /* No bounce count increase for wall bounces */
    }

    /* Check for paddle collision after accounting for wall clamps */
    if (check_future_paddle_hit(next_row, next_col))
    {
        ball_dy = -1;           /* Always bounce up from paddle */
        next_row = y_row - 1;   /* Immediately move ball up one row to clear paddle */
        score++;
        bounce_count++;         /* Only count paddle hits for speed increase */
        paddle_needs_redraw = 1;/* Force paddle redraw to fix blank space */
    }

    /* Commit position update */
    x_row = next_row;
    x_col = next_col;

    /* Ball goes out at bottom */
    if (x_row > MAX_ROW)
    {
        /* Hardware RNG provides true randomness - no entropy mixing needed */

        /* Reset ball position with random starting conditions */
        x_row        = MIN_ROW; /* Start from very top */
        last_random  = get_random();
        x_col        = MIN_COL + 5 + (last_random % (MAX_COL - MIN_COL - 10)); /* Random horizontal position */
        last_random  = get_random();
        ball_dx      = (last_random & 1) ? -1 : 1; /* Random horizontal direction */
        ball_dy      = 1;                          /* Always start going down */
        bounce_count = 0;                          /* Reset speed to default for new ball */
        ball_counter = 0;                          /* Reset ball timing counter for fresh start */
        base_speed   = 2;                          /* Ensure base speed is reset to default (2 cycles = 100ms) */
        if (score > 0)
            score--; /* Lose a point */
        return 1;    /* Signal that ball was reset - force immediate counter reset */
    }
    return 0;
}

int handle_paddle_input()
{
    int ch;
    int paddle_moved;

    paddle_moved = 0;

    /* Check for user input (non-blocking) */
    if (check_key_ready())
    {
        ch              = bdos(6, 0xFF) & 0xFF;
        g_last_key_code = ch;

        if (ch == KEY_ESC || ch == CTRL_C)
        {
            return -1; /* Signal to quit game */
        }
        else if (ch == LEFT_CURSOR)
        { /* Move paddle left - cursor left key */
            if (y_col > MIN_COL + 3)
                y_col -= 3; /* Move 3 spaces */
            else if (y_col > MIN_COL)
                y_col--; /* Move 1 space if near edge */
            paddle_moved = 1;
        }
        else if (ch == RIGHT_CURSOR)
        { /* Move paddle right - cursor right key */
            if (y_col <= MAX_COL - 8)
                y_col += 3; /* Move 3 spaces */
            else if (y_col < MAX_COL - 5)
                y_col++; /* Move 1 space if near edge */
            paddle_moved = 1;
        }
    }

    return paddle_moved;
}

int check_key_ready()
{
    /* Check if key is available without blocking */
    return (bdos(11) & 0xFF);
}

/* --- Main Program --- */

int main()
{
    int ch;
    int old_x_row, old_x_col;
    int old_y_row, old_y_col;
    int running;

    g_seq_len       = 0;
    g_last_key_code = 0;

    /* Hardware RNG is ready to use - no seed initialization needed */

    /* Initialize game state */
    x_row       = MIN_ROW; /* Ball starts at top */
    last_random = get_random();
    x_col       = MIN_COL + 5 + (last_random % (MAX_COL - MIN_COL - 10)); /* Random start position */
    y_row       = 28;                                                     /* Paddle moved 2 rows down */
    y_col       = 30;                                                     /* Paddle starts in middle of narrower area */
    last_random = get_random();
    ball_dx     = (last_random & 1) ? -1 : 1; /* Random horizontal direction */
    ball_dy     = 1;                          /* Ball moves down */
    score       = 0;

    /* Initialize speed system */
    bounce_count = 0;  /* Start with no bounces */
    base_speed   = 2;  /* Base speed: 2 cycles = 100ms (50ms per cycle) */

    /* Initialize game loop timer and counters */
    ball_counter        = 0;
    paddle_counter      = 0;
    status_counter      = 0;
    paddle_needs_redraw = 0; /* Initialize collision redraw flag */
    start_game_loop_timer();

    hide_cursor();
    clear_screen();
    disable_local_echo();
    draw_instructions();
    draw_walls();
    update_status();

    /* Draw both objects */
    draw_x_marker(x_row, x_col);
    draw_y_marker(y_row, y_col);

    running = 1;
    while (running)
    {
        old_x_row = x_row;
        old_x_col = x_col;
        old_y_row = y_row;
        old_y_col = y_col;

        /* Check if game loop timer has expired - cooperative multitasking */
        if (is_game_loop_timer_expired())
        {
            ball_counter++;
            paddle_counter++;
            status_counter++;

            /* Handle paddle input every 1 cycles (1 * 50ms = 50ms) */
            if (paddle_counter >= 1)
            {
                ch = handle_paddle_input();
                if (ch == -1)
                {
                    running = 0; /* Quit game */
                }
                paddle_counter = 0; /* Reset paddle counter */
            }

            /* Hardware RNG used - no seed updates needed */

            /* Move ball when counter reaches the required cycles */
            if (should_move_ball())
            {
                update_ball_position();
                ball_counter = 0; /* Reset ball counter after movement */
            }

            start_game_loop_timer(); /* Restart 50ms timer */
        }

        /* Only redraw objects that moved */
        if (x_row != old_x_row || x_col != old_x_col)
        {
            erase_x_marker(old_x_row, old_x_col);
            draw_x_marker(x_row, x_col);
        }
        if (y_row != old_y_row || y_col != old_y_col || paddle_needs_redraw)
        {
            erase_y_marker(old_y_row, old_y_col);
            draw_y_marker(y_row, y_col);
            paddle_needs_redraw = 0; /* Reset flag after redraw */
        }

        /* Update status every 20 cycles (20 * 50ms = 1000ms) */
        if (status_counter >= 20)
        {
            update_status();
            status_counter = 0; /* Reset status counter */
        }
        /* No delay loop needed - timing controlled by cooperative multitasking */
    }

    clear_screen();
    show_cursor();
    cputs("Final Score: ");
    putnum(score);
    cputs("\r\nGoodbye!\r\n");
    return 0;
}
