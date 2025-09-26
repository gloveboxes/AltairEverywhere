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
 * - Q: Quit game
 * 
 * Based on breakout.c pattern for BDS C compatibility
 */

/* BDS C library hooks */
int bdos();
int bios();
int inp();
int outp();

/* Key Codes */
#define KEY_ESC   27
#define KEY_UP    5
#define KEY_DOWN  24
#define KEY_LEFT  19
#define KEY_RIGHT 4

/* Screen Boundaries */
#define MIN_ROW 6
#define MAX_ROW 25
#define MIN_COL 5
#define MAX_COL 75

/* Snake Settings */
#define MAX_SNAKE_LENGTH 200
#define INITIAL_LENGTH 3

/* Game States */
#define GAME_PLAYING 0
#define GAME_OVER 1
#define GAME_QUIT 2

/* Directions */
#define DIR_NONE 0
#define DIR_UP 1
#define DIR_DOWN 2
#define DIR_LEFT 3
#define DIR_RIGHT 4

/* Snake body coordinates */
int snake_row[MAX_SNAKE_LENGTH];
int snake_col[MAX_SNAKE_LENGTH];
int snake_length;
int snake_direction;
int next_direction;

/* Food position */
int food_row, food_col;
int food_exists;

/* Game state */
int game_state;
int score;
int speed_level;

/* Timing */
int game_counter;
int input_counter;
int status_counter;

/* Input handling globals */
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
    if (n < 0) {
        chput('-');
        n = -n;
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
    chput(KEY_ESC);
    cputs("[0m");
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

/* --- Game Display --- */

int draw_walls()
{
    int i;
    
    /* Draw top wall */
    cursor_move(MIN_ROW - 1, MIN_COL - 1);
    for (i = MIN_COL - 1; i <= MAX_COL + 1; i++) {
        chput('#');
    }
    
    /* Draw side walls */
    for (i = MIN_ROW; i <= MAX_ROW; i++) {
        cursor_move(i, MIN_COL - 1);
        chput('#');
        cursor_move(i, MAX_COL + 1);
        chput('#');
    }
    
    /* Draw bottom wall */
    cursor_move(MAX_ROW + 1, MIN_COL - 1);
    for (i = MIN_COL - 1; i <= MAX_COL + 1; i++) {
        chput('#');
    }
    
    return 0;
}

int draw_instructions()
{
    cursor_move(1, 1);
    cputs("Snake Game for Altair 8800 (Enable Character Mode: Ctrl+L)");
    cursor_move(2, 1);
    cputs("Arrow keys to move, Q to quit. Don't hit walls or yourself!");
    cursor_move(3, 1);
    cputs("Eat food (*) to grow and increase score.");
    cursor_move(4, 1);
    cputs("------------------------------------------------------------------");
    return 0;
}

int draw_snake_segment(row, col, is_head)
int row;
int col;
int is_head;
{
    cursor_move(row, col);
    if (is_head) {
        chput('O');  /* Head */
    } else {
        chput('o');  /* Body */
    }
    return 0;
}

int erase_position(row, col)
int row;
int col;
{
    cursor_move(row, col);
    chput(' ');
    return 0;
}

int draw_food(row, col)
int row;
int col;
{
    cursor_move(row, col);
    chput('*');
    return 0;
}

int update_status()
{
    cursor_move(5, 1);
    cputs("Score: ");
    putnum(score);
    cputs("   Length: ");
    putnum(snake_length);
    cputs("   Speed: ");
    putnum(speed_level);
    cputs("                    ");
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
    sign = 1;
    
    /* Skip leading spaces */
    while (*s == ' ') s++;
    
    /* Check for sign */
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    
    /* Convert digits */
    while (*s >= '0' && *s <= '9') {
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
    while (i < 15) {
        ch = inp(200);
        if (ch == 0) break;
        random_str[i] = ch;
        i++;
    }
    random_str[i] = 0;
    
    /* Convert string to integer and make it positive */
    result = string_to_int(random_str);
    return abs(result);
}

/* --- Food Management --- */

int is_position_occupied(row, col)
int row;
int col;
{
    int i;
    
    /* Check if position conflicts with snake body */
    for (i = 0; i < snake_length; i++) {
        if (snake_row[i] == row && snake_col[i] == col) {
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
    while (attempts < 50) {  /* Try up to 50 times */
        food_r = MIN_ROW + (get_random() % (MAX_ROW - MIN_ROW + 1));
        food_c = MIN_COL + (get_random() % (MAX_COL - MIN_COL + 1));
        
        if (!is_position_occupied(food_r, food_c)) {
            food_row = food_r;
            food_col = food_c;
            food_exists = 1;
            draw_food(food_row, food_col);
            return 1;
        }
        attempts++;
    }
    return 0;  /* Failed to place food */
}

/* --- Input Handling --- */

int check_key_ready()
{
    return (bdos(11) & 0xFF);
}

int get_immediate_char()
{
    return (bdos(6, 0xFF) & 0xFF);
}

int read_input_key()
{
    int ch;
    
    g_seq_len = 0;
    g_last_key_code = 0;
    
    if (!check_key_ready()) return 0;
    
    ch = get_immediate_char();
    g_last_seq[0] = ch;
    g_seq_len = 1;
    g_last_key_code = ch;
    
    /* Return the key directly - Altair system provides decoded values */
    return ch;
}

int handle_input()
{
    int key;
    
    if (!check_key_ready()) return 0;
    
    key = read_input_key();
    
    if (key == 'q' || key == 'Q') {
        game_state = GAME_QUIT;
        return 1;
    }
    
    /* Handle direction changes - prevent reversing into self */
    if (key == KEY_UP && snake_direction != DIR_DOWN) {
        next_direction = DIR_UP;
    } else if (key == KEY_DOWN && snake_direction != DIR_UP) {
        next_direction = DIR_DOWN;
    } else if (key == KEY_LEFT && snake_direction != DIR_RIGHT) {
        next_direction = DIR_LEFT;
    } else if (key == KEY_RIGHT && snake_direction != DIR_LEFT) {
        next_direction = DIR_RIGHT;
    }
    
    return 0;
}

/* --- Game Logic --- */

int init_snake()
{
    int i;
    
    snake_length = INITIAL_LENGTH;
    snake_direction = DIR_RIGHT;
    next_direction = DIR_RIGHT;
    
    /* Place snake in center, moving right */
    for (i = 0; i < snake_length; i++) {
        snake_row[i] = (MIN_ROW + MAX_ROW) / 2;
        snake_col[i] = (MIN_COL + MAX_COL) / 2 - i;
    }
    
    /* Draw initial snake */
    for (i = 0; i < snake_length; i++) {
        draw_snake_segment(snake_row[i], snake_col[i], (i == 0));
    }
    
    return 0;
}

int move_snake()
{
    int head_r, head_c;
    int i;
    int ate_food;
    
    /* Update direction */
    snake_direction = next_direction;
    
    /* Calculate new head position */
    head_r = snake_row[0];
    head_c = snake_col[0];
    
    if (snake_direction == DIR_UP) {
        head_r--;
    } else if (snake_direction == DIR_DOWN) {
        head_r++;
    } else if (snake_direction == DIR_LEFT) {
        head_c--;
    } else if (snake_direction == DIR_RIGHT) {
        head_c++;
    }
    
    /* Check wall collision */
    if (head_r < MIN_ROW || head_r > MAX_ROW ||
        head_c < MIN_COL || head_c > MAX_COL) {
        game_state = GAME_OVER;
        return 0;
    }
    
    /* Check self collision */
    for (i = 0; i < snake_length; i++) {
        if (snake_row[i] == head_r && snake_col[i] == head_c) {
            game_state = GAME_OVER;
            return 0;
        }
    }
    
    /* Check food collision */
    ate_food = 0;
    if (food_exists && head_r == food_row && head_c == food_col) {
        ate_food = 1;
        food_exists = 0;
        score += 10;
        
        /* Increase speed every 50 points */
        if (score % 50 == 0 && speed_level < 10) {
            speed_level++;
        }
    }
    
    /* Move snake body */
    if (!ate_food) {
        /* Erase tail */
        erase_position(snake_row[snake_length - 1], snake_col[snake_length - 1]);
        
        /* Shift body positions */
        for (i = snake_length - 1; i > 0; i--) {
            snake_row[i] = snake_row[i - 1];
            snake_col[i] = snake_col[i - 1];
        }
    } else {
        /* Snake grows - shift body and increase length */
        if (snake_length < MAX_SNAKE_LENGTH) {
            for (i = snake_length; i > 0; i--) {
                snake_row[i] = snake_row[i - 1];
                snake_col[i] = snake_col[i - 1];
            }
            snake_length++;
        }
    }
    
    /* Set new head position */
    snake_row[0] = head_r;
    snake_col[0] = head_c;
    
    /* Redraw snake head and body */
    draw_snake_segment(snake_row[0], snake_col[0], 1);  /* Head */
    if (snake_length > 1) {
        draw_snake_segment(snake_row[1], snake_col[1], 0);  /* Neck (was head) */
    }
    
    return 1;
}

int get_game_speed()
{
    int base_cycles;
    int speed_cycles;
    
    /* Base speed: 8 cycles = 200ms, gets faster with speed_level */
    base_cycles = 8;
    speed_cycles = base_cycles - speed_level;
    if (speed_cycles < 2) speed_cycles = 2;  /* Minimum: 50ms */
    return speed_cycles;
}

/* --- Timer Functions --- */

int start_game_loop_timer()
{
    outp(29, 25);  /* 25ms timer */
    return 0;
}

int is_game_loop_timer_expired()
{
    return (inp(29) == 0);
}

/* --- Game Over Display --- */

int show_game_over()
{
    cursor_move(15, 30);
    cputs("GAME OVER!");
    cursor_move(16, 25);
    cputs("Final Score: ");
    putnum(score);
    cursor_move(17, 25);
    cputs("Final Length: ");
    putnum(snake_length);
    cursor_move(18, 25);
    cputs("Press Q to quit");
    return 0;
}

/* --- Main Program --- */

int main()
{
    int key;
    
    /* Initialize game state */
    game_state = GAME_PLAYING;
    score = 0;
    speed_level = 1;
    food_exists = 0;
    
    /* Initialize timing counters */
    game_counter = 0;
    input_counter = 0;
    status_counter = 0;
    
    /* Set up display */
    clear_screen();
    hide_cursor();
    draw_instructions();
    draw_walls();
    
    /* Initialize snake */
    init_snake();
    
    /* Place first food */
    place_food();
    
    /* Initial status display */
    update_status();
    
    /* Main game loop */
    while (game_state == GAME_PLAYING) {
        /* Start timer for this loop iteration */
        start_game_loop_timer();
        
        /* Handle input every cycle */
        input_counter++;
        if (input_counter >= 1) {  /* Check input every cycle */
            handle_input();
            input_counter = 0;
        }
        
        /* Move snake based on speed */
        game_counter++;
        if (game_counter >= get_game_speed()) {
            if (game_state == GAME_PLAYING) {
                move_snake();
            }
            game_counter = 0;
        }
        
        /* Place new food if needed */
        if (!food_exists && game_state == GAME_PLAYING) {
            place_food();
        }
        
        /* Update status display */
        status_counter++;
        if (status_counter >= 20) {  /* Update every 500ms */
            update_status();
            status_counter = 0;
        }
        
        /* Wait for timer */
        while (!is_game_loop_timer_expired() && game_state == GAME_PLAYING) {
            /* Check for quit during wait */
            if (check_key_ready()) {
                handle_input();
            }
        }
    }
    
    /* Game over or quit */
    if (game_state == GAME_OVER) {
        show_game_over();
        
        /* Wait for quit key */
        while (1) {
            if (check_key_ready()) {
                key = read_input_key();
                if (key == 'q' || key == 'Q') break;
            }
        }
    }
    
    /* Cleanup */
    cursor_move(27, 1);
    show_cursor();
    cputs("Thanks for playing Snake!\r\n");
    
    return 0;
}
