/*
 * Snake Game for Altair 8800 - VT100/xterm.js compatible
 * BDS C 1.6 on CP/M
 *
 * Classic Snake game using arrow keys for movement.
 * Collect food (*) to grow longer and increase score.
 * Don't hit walls or yourself!
 *
 * Controls:
 * - Arrow keys: Change direction
 * - ESC: Quit game
 *
 * Based on breakout.c pattern for BDS C compatibility
 */

/* BDS C library hooks */
int inp();    /* int inp(port); */
int outp();   /* int outp(port, value); */

/* Timer library functions */
int x_tmrset(); /* int x_tmrset(timer, ms); */
int x_tmrexp(); /* int x_tmrexp(timer); */

/* Terminal library functions */
int x_putch(); /* int x_putch(c); */
int x_puts();  /* int x_puts(s); */
int x_numpr(); /* int x_numpr(n); */
int x_curmv(); /* int x_curmv(row, col); */
int x_clrsc(); /* int x_clrsc(); */
int x_hidcr(); /* int x_hidcr(); */
int x_shwcr(); /* int x_shwcr(); */
int x_keyck(); /* int x_keyck(); */
int x_keygt(); /* int x_keygt(); */
int x_isesc(); /* int x_isesc(code); */
int x_isup();  /* int x_isup(code); */
int x_isdn();  /* int x_isdn(code); */
int x_islt();  /* int x_islt(code); */
int x_isrt();  /* int x_isrt(code); */
int x_setcol(); /* int x_setcol(code); */
int x_rstcol(); /* int x_rstcol(); */

/* Color constants */
#define XC_BLK 30   /* Black */
#define XC_RED 31   /* Red */
#define XC_GRN 32   /* Green */
#define XC_YEL 33   /* Yellow */
#define XC_BLU 34   /* Blue */
#define XC_MAG 35   /* Magenta */
#define XC_CYN 36   /* Cyan */
#define XC_WHT 37   /* White */
#define XC_RST 0    /* Reset all attributes */

/* Timer configuration */
#define TIMER_ID 2  /* Use timer 2 */
#define TIMER_MS 25 /* 25ms game loop timer */

/* Screen Boundaries */
#define MIN_ROW 6
#define MAX_ROW 25
#define MIN_COL 5
#define MAX_COL 75

/* Snake Settings */
#define MAX_SNAKE_LENGTH 200
#define INITIAL_LENGTH   3

/* Game States */
#define GAME_PLAYING 0
#define GAME_OVER    1
#define GAME_QUIT    2

/* Directions */
#define DIR_NONE  0
#define DIR_UP    1
#define DIR_DOWN  2
#define DIR_LEFT  3
#define DIR_RIGHT 4

/* Snake body coordinates */
int sn_row[MAX_SNAKE_LENGTH];
int sn_col[MAX_SNAKE_LENGTH];
int sn_len;
int sn_dir;
int nxt_dir;

/* Food position */
int food_row, food_col;
int food_exists;

/* Game state */
int gm_st;
int score;
int sp_lvl;

/* Timing */
int gm_cnt;
int in_cnt;
int st_cnt;

/* --- Game Display --- */

int draw_walls()
{
    int i;

    /* Set green color for walls */
    x_setcol(XC_GRN);

    /* Draw top wall */
    x_curmv(MIN_ROW - 1, MIN_COL - 1);
    for (i = MIN_COL - 1; i <= MAX_COL + 1; i++)
    {
        x_putch('#');
    }

    /* Draw side walls */
    for (i = MIN_ROW; i <= MAX_ROW; i++)
    {
        x_curmv(i, MIN_COL - 1);
        x_putch('#');
        x_curmv(i, MAX_COL + 1);
        x_putch('#');
    }

    /* Draw bottom wall */
    x_curmv(MAX_ROW + 1, MIN_COL - 1);
    for (i = MIN_COL - 1; i <= MAX_COL + 1; i++)
    {
        x_putch('#');
    }

    /* Reset color */
    x_rstcol();
    return 0;
}

int draw_instructions()
{
    x_curmv(1, 1);
    x_puts("Snake Game for Altair 8800 (Enable Character Mode: Ctrl+L)");
    x_curmv(2, 1);
    x_puts("Arrow keys to move, ESC to quit. Don't hit walls or yourself!");
    x_curmv(3, 1);
    x_puts("Eat food (*) to grow and increase score.");
    x_curmv(4, 1);
    x_puts("------------------------------------------------------------------");
    return 0;
}

int dr_seg(row, col, is_head)
int row;
int col;
int is_head;
{
    /* Set red color for snake */
    x_setcol(XC_RED);
    x_curmv(row, col);
    if (is_head)
    {
        x_putch('O'); /* Head */
    }
    else
    {
        x_putch('o'); /* Body */
    }
    /* Reset color */
    x_rstcol();
    return 0;
}

int er_pos(row, col)
int row;
int col;
{
    x_curmv(row, col);
    x_putch(' ');
    return 0;
}

int draw_food(row, col)
int row;
int col;
{
    /* Set blue color for food */
    x_setcol(XC_BLU);
    x_curmv(row, col);
    x_putch('*');
    /* Reset color */
    x_rstcol();
    return 0;
}

int snake_update_status()
{
    x_curmv(5, 1);
    x_puts("Score: ");
    x_numpr(score);
    x_puts("   Length: ");
    x_numpr(sn_len);
    x_puts("   Speed: ");
    x_numpr(sp_lvl);
    x_puts("                    ");
    return 0;
}

/* --- Random Number Generation --- */

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
    {
        ch = inp(200);
        if (ch == 0)
            break;
        random_str[i] = ch;
        i++;
    }
    random_str[i] = 0;

    /* Convert string to integer and make it positive */
    result = string_to_int(random_str);
    return abs(result);
}

/* --- Food Management --- */

int is_occ(row, col)
int row;
int col;
{
    int i;

    /* Check if position conflicts with snake body */
    for (i = 0; i < sn_len; i++)
    {
        if (sn_row[i] == row && sn_col[i] == col)
        {
            return 1;
        }
    }
    return 0;
}

int place_food()
{
    int attempts;
    int food_r, food_c;

    attempts = 0;
    while (attempts < 50)
    { /* Try up to 50 times */
        food_r = MIN_ROW + (get_random() % (MAX_ROW - MIN_ROW + 1));
        food_c = MIN_COL + (get_random() % (MAX_COL - MIN_COL + 1));

        if (!is_occ(food_r, food_c))
        {
            food_row    = food_r;
            food_col    = food_c;
            food_exists = 1;
            draw_food(food_row, food_col);
            return 1;
        }
        attempts++;
    }
    return 0; /* Failed to place food */
}

/* --- Input Handling --- */

int snake_handle_input()
{
    int key;

    if (!x_keyck())
        return 0;

    key = x_keygt();
    if (!key)
        return 0;

    if (x_isesc(key))
    {
        gm_st = GAME_QUIT;
        return 1;
    }

    /* Handle direction changes - prevent reversing into self */
    if (x_isup(key) && sn_dir != DIR_DOWN)
    {
        nxt_dir = DIR_UP;
    }
    else if (x_isdn(key) && sn_dir != DIR_UP)
    {
        nxt_dir = DIR_DOWN;
    }
    else if (x_islt(key) && sn_dir != DIR_RIGHT)
    {
        nxt_dir = DIR_LEFT;
    }
    else if (x_isrt(key) && sn_dir != DIR_LEFT)
    {
        nxt_dir = DIR_RIGHT;
    }

    return 0;
}

/* --- Game Logic --- */

int init_snake()
{
    int i;

    sn_len  = INITIAL_LENGTH;
    sn_dir  = DIR_RIGHT;
    nxt_dir = DIR_RIGHT;

    /* Place snake in center, moving right */
    for (i = 0; i < sn_len; i++)
    {
        sn_row[i] = (MIN_ROW + MAX_ROW) / 2;
        sn_col[i] = (MIN_COL + MAX_COL) / 2 - i;
    }

    /* Draw initial snake */
    for (i = 0; i < sn_len; i++)
    {
        dr_seg(sn_row[i], sn_col[i], (i == 0));
    }

    return 0;
}

int move_snake()
{
    int head_r, head_c;
    int i;
    int ate_food;

    /* Update direction */
    sn_dir = nxt_dir;

    /* Calculate new head position */
    head_r = sn_row[0];
    head_c = sn_col[0];

    if (sn_dir == DIR_UP)
    {
        head_r--;
    }
    else if (sn_dir == DIR_DOWN)
    {
        head_r++;
    }
    else if (sn_dir == DIR_LEFT)
    {
        head_c--;
    }
    else if (sn_dir == DIR_RIGHT)
    {
        head_c++;
    }

    /* Check wall collision */
    if (head_r < MIN_ROW || head_r > MAX_ROW || head_c < MIN_COL || head_c > MAX_COL)
    {
        gm_st = GAME_OVER;
        return 0;
    }

    /* Check self collision */
    for (i = 0; i < sn_len; i++)
    {
        if (sn_row[i] == head_r && sn_col[i] == head_c)
        {
            gm_st = GAME_OVER;
            return 0;
        }
    }

    /* Check food collision */
    ate_food = 0;
    if (food_exists && head_r == food_row && head_c == food_col)
    {
        ate_food    = 1;
        food_exists = 0;
        score += 10;

        /* Increase speed every 50 points */
        if (score % 50 == 0 && sp_lvl < 10)
        {
            sp_lvl++;
        }
    }

    /* Move snake body */
    if (!ate_food)
    {
        /* Erase tail */
        er_pos(sn_row[sn_len - 1], sn_col[sn_len - 1]);

        /* Shift body positions */
        for (i = sn_len - 1; i > 0; i--)
        {
            sn_row[i] = sn_row[i - 1];
            sn_col[i] = sn_col[i - 1];
        }
    }
    else
    {
        /* Snake grows - shift body and increase length */
        if (sn_len < MAX_SNAKE_LENGTH)
        {
            for (i = sn_len; i > 0; i--)
            {
                sn_row[i] = sn_row[i - 1];
                sn_col[i] = sn_col[i - 1];
            }
            sn_len++;
        }
    }

    /* Set new head position */
    sn_row[0] = head_r;
    sn_col[0] = head_c;

    /* Redraw snake head and body */
    dr_seg(sn_row[0], sn_col[0], 1); /* Head */
    if (sn_len > 1)
    {
        dr_seg(sn_row[1], sn_col[1], 0); /* Neck (was head) */
    }

    return 1;
}

int gt_spd()
{
    int base_cycles;
    int speed_cycles;

    /* Base speed: 80 cycles = 2000ms (2 seconds), gets faster with level */
    base_cycles  = 80;
    speed_cycles = base_cycles - (sp_lvl * 5);
    if (speed_cycles < 16)
        speed_cycles = 16; /* Minimum: 400ms */
    return speed_cycles;
}

/* --- Game Over Display --- */

int show_game_over()
{
    x_curmv(15, 30);
    x_puts("GAME OVER!");
    x_curmv(16, 25);
    x_puts("Final Score: ");
    x_numpr(score);
    x_curmv(17, 25);
    x_puts("Final Length: ");
    x_numpr(sn_len);
    x_curmv(18, 25);
    x_puts("Press ESC to quit");
    return 0;
}

/* --- Main Program --- */

int main()
{
    int key;

    /* Initialize game state */
    gm_st       = GAME_PLAYING;
    score       = 0;
    sp_lvl      = 1;
    food_exists = 0;

    /* Initialize timing counters */
    gm_cnt = 0;
    in_cnt = 0;
    st_cnt = 0;

    /* Set up display */
    x_clrsc();
    x_hidcr();
    draw_instructions();
    draw_walls();

    /* Initialize snake */
    init_snake();

    /* Place first food */
    place_food();

    /* Initial status display */
    snake_update_status();

    /* Main game loop */
    while (gm_st == GAME_PLAYING)
    {
        /* Start timer for this loop iteration */
        x_tmrset(TIMER_ID, TIMER_MS);

        /* Handle input every cycle */
        in_cnt++;
        if (in_cnt >= 1)
        { /* Check input every cycle */
            snake_handle_input();
            in_cnt = 0;
        }

        /* Move snake based on speed */
        gm_cnt++;
        if (gm_cnt >= gt_spd())
        {
            if (gm_st == GAME_PLAYING)
            {
                move_snake();
            }
            gm_cnt = 0;
        }

        /* Place new food if needed */
        if (!food_exists && gm_st == GAME_PLAYING)
        {
            place_food();
        }

        /* Update status display */
        st_cnt++;
        if (st_cnt >= 20)
        { /* Update every 500ms */
            snake_update_status();
            st_cnt = 0;
        }

        /* Wait for timer to expire */
        while (x_tmrexp(TIMER_ID) && gm_st == GAME_PLAYING)
        {
            /* Check for quit during wait */
            if (x_keyck())
            {
                snake_handle_input();
            }
        }
    }

    /* Game over or quit */
    if (gm_st == GAME_OVER)
    {
        show_game_over();

        /* Wait for quit key */
        while (1)
        {
            if (x_keyck())
            {
                key = x_keygt();
                if (x_isesc(key))
                    break;
            }
        }
    }

    /* Cleanup */
    x_curmv(27, 1);
    x_shwcr();
    x_puts("Thanks for playing Snake!\r\n");

    return 0;
}
