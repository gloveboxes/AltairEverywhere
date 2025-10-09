/*
 * Tetris Game for Altair 8800 - VT100/xterm.js compatible
 * BDS C 1.6 on CP/M
 *
 * Rewritten for BDS C constraints:
 *  - All symbols unique within first 7 characters
 *  - No goto labels named 'end'
 *  - Simple deterministic gravity; timer port 29: 1 while running, 0 on expiry
 *  - K&R style definitions only
 */

/* --- BDS C hooks --- */
int bdos();
int bios();
int inp();
int outp();
int x_tmrset();
int x_tmrexp();

/* --- Key codes --- */
#define KEY_ESC   27
#define KEY_UP     5
#define KEY_DOWN  24
#define KEY_LEFT  19
#define KEY_RIGHT  4
#define KEY_SPACE 32

/* Timer configuration */
#define TIMER_ID 2      /* Use timer 2 */
#define DELAY_MS 255    /* 255ms game loop delay */

/* --- Board & UI layout --- */
#define BD_W      10
#define BD_H      20
#define BD_SROW   3
#define BD_SCOL  30

#define PRV_ROW   6
#define PRV_COL  51

/* --- Game constants --- */
#define PIECE_NONE 0
#define PIECE_I    1
#define PIECE_O    2
#define PIECE_T    3
#define PIECE_S    4
#define PIECE_Z    5
#define PIECE_J    6
#define PIECE_L    7

#define GAME_PLAYING 0
#define GAME_OVER    1
#define GAME_QUIT    3

/* --- Globals (<=7 chars, unique) --- */
int board[BD_H][BD_W];

int act_pcs;    /* active piece id */
int act_rot;    /* active rotation */
int act_x, act_y;  /* board coords */

int nxt_pcs;    /* next piece id */
int game_st;    /* state */
int score;
int lines_cl;   /* lines cleared */
int level;
int fall_tm;    /* gravity tick counter */
int fall_sp;    /* ticks per row */
int soft_dr;    /* soft drop active this tick */

int pcs_shp[8][4]; /* 16-bit 4x4 masks, bit15=TL, bit0=BR */

int prv_ax;    /* previous active x */
int prv_ay;    /* previous active y */
int prv_rt;    /* previous active rotation */
int prv_pc;    /* previous active piece id */
int prv_on;    /* previous active drawn flag */

/* ========================= Low-level console ========================= */
int chput(c)
char c;
{ return bios(4, c); }

int cputs(s)
char *s;
{ while (*s) chput(*s++); return 0; }

int putnum(n)
int n;
{
    char buf[8]; int i;
    if (n==0) { chput('0'); return 0; }
    if (n<0) { chput('-'); n = -n; }
    i = 0; while (n>0 && i<7) { buf[i++] = (n%10)+'0'; n/=10; }
    while (i--) chput(buf[i]);
    return 0;
}

int num_buf(n,buf)
int n;
char *buf;
{
    char tmp[8]; int i,j;
    if (buf==0) return 0;
    i = 0;
    if (n==0) { buf[i++] = '0'; return i; }
    if (n<0) { buf[i++]='-'; n = -n; }
    j = 0;
    while (n>0 && j<8) { tmp[j++] = (n%10)+'0'; n/=10; }
    while (j>0) { buf[i++] = tmp[--j]; }
    return i;
}

int cur_mov(row,col)
int row; int col;
{
    char seq[16]; int idx;
    idx = 0;
    seq[idx++] = 27;
    seq[idx++] = '[';
    idx += num_buf(row, &seq[idx]);
    seq[idx++] = ';';
    idx += num_buf(col, &seq[idx]);
    seq[idx++] = 'H';
    seq[idx] = 0;
    cputs(seq);
    return 0;
}

int clr_scr()
{
    cputs("\033[2J");
    cputs("\033[0m");
    cur_mov(1,1);
    return 0;
}

int hid_cur() { cputs("\033[?25l"); return 0; }
int shw_cur() { cputs("\033[?25h"); return 0; }
int ers_cur() { cputs("\033[K"); return 0; } /* erase to end of line */

/* Color support for xterm.js */
int set_col(colcode)
int colcode;
{
    char seq[12]; int idx;
    idx = 0;
    seq[idx++] = 27;
    seq[idx++] = '[';
    idx += num_buf(colcode, &seq[idx]);
    seq[idx++] = 'm';
    seq[idx] = 0;
    cputs(seq);
    return 0;
}

int rst_col() { cputs("\033[0m"); return 0; }

/* ========================= Simple RNG ========================= */
int iabs(n) int n; { return (n<0)?-n:n; }

int str2int(s)
char *s; {
    int r; int sign; r=0; sign=1;
    while (*s==' ') s++;
    if (*s=='-') { sign=-1; s++; } else if (*s=='+') { s++; }
    while (*s>='0' && *s<='9') { r=r*10+(*s-'0'); s++; }
    return r*sign;
}

int rnd_get()
{
    char rs[16]; int i,ch,r;
    outp(44,1);
    i=0; while (i<15) { ch=inp(200); if (ch==0) break; rs[i++]=ch; }
    rs[i]=0; r=str2int(rs);
    return iabs(r);
}

int rnd_pcs()
{
    return (rnd_get() % 7) + 1; /* PIECE_I through PIECE_L */
}

/* ========================= Shapes ========================= */
int init_shp()
{
    /* MSB-first: bit15=TL, bit0=BR; row-major */
    /* I */
    pcs_shp[PIECE_I][0] = 0x0F00; pcs_shp[PIECE_I][1] = 0x2222;
    pcs_shp[PIECE_I][2] = 0x00F0; pcs_shp[PIECE_I][3] = 0x4444;
    /* O */
    pcs_shp[PIECE_O][0] = 0x0660; pcs_shp[PIECE_O][1] = 0x0660;
    pcs_shp[PIECE_O][2] = 0x0660; pcs_shp[PIECE_O][3] = 0x0660;
    /* T */
    pcs_shp[PIECE_T][0] = 0x0E40; pcs_shp[PIECE_T][1] = 0x4C40;
    pcs_shp[PIECE_T][2] = 0x4E00; pcs_shp[PIECE_T][3] = 0x4640;
    /* S */
    pcs_shp[PIECE_S][0] = 0x06C0; pcs_shp[PIECE_S][1] = 0x4620;
    pcs_shp[PIECE_S][2] = 0x06C0; pcs_shp[PIECE_S][3] = 0x4620;
    /* Z */
    pcs_shp[PIECE_Z][0] = 0x0C60; pcs_shp[PIECE_Z][1] = 0x2640;
    pcs_shp[PIECE_Z][2] = 0x0C60; pcs_shp[PIECE_Z][3] = 0x2640;
    /* J */
    pcs_shp[PIECE_J][0] = 0x08E0; pcs_shp[PIECE_J][1] = 0x64C0;
    pcs_shp[PIECE_J][2] = 0x0E20; pcs_shp[PIECE_J][3] = 0x6440;
    /* L */
    pcs_shp[PIECE_L][0] = 0x02E0; pcs_shp[PIECE_L][1] = 0x4460;
    pcs_shp[PIECE_L][2] = 0x0E80; pcs_shp[PIECE_L][3] = 0xC440;
    return 0;
}

int pcell(piece,rot,row,col)
int piece,rot,row,col;
{
    int idx,dt;
    if (piece<PIECE_I || piece>PIECE_L) return 0;
    if (row<0 || row>=4 || col<0 || col>=4) return 0;
    dt = pcs_shp[piece][rot & 3];
    idx = row*4 + col; /* 0..15 */
    return (dt >> (15-idx)) & 1;
}

int pcs_chr(piece)
int piece;
{
    if (piece==PIECE_I) return '#';
    if (piece==PIECE_O) return 'O';
    if (piece==PIECE_T) return 'T';
    if (piece==PIECE_S) return 'S';
    if (piece==PIECE_Z) return 'Z';
    if (piece==PIECE_J) return 'J';
    if (piece==PIECE_L) return 'L';
    return ' ';
}

/* Color palette for pieces (ANSI codes) */
int pcs_col(piece)
int piece;
{
    if (piece==PIECE_I) return 36; /* Cyan */
    if (piece==PIECE_O) return 33; /* Yellow */
    if (piece==PIECE_T) return 35; /* Magenta */
    if (piece==PIECE_S) return 32; /* Green */
    if (piece==PIECE_Z) return 31; /* Red */
    if (piece==PIECE_J) return 34; /* Blue */
    if (piece==PIECE_L) return 93; /* Bright Yellow */
    return 37; /* White */
}

/* ========================= UI ========================= */
int drw_ins()
{
    cur_mov(1,1);
    cputs("TETRIS for Altair 8800 V1.5 (Enable Character Mode: Ctrl+L)");
    cur_mov(2,1);
    cputs("LEFT/RIGHT: Move  UP: Rotate  DOWN: Soft Drop  SPACE: Hard Drop  ESC x2: Quit");
    cur_mov(3,1);
    cputs("====================================================================");
    return 0;
}

int drw_bor()
{
    int i;
    for (i=0;i<BD_H+2;i++) { cur_mov(BD_SROW+i, BD_SCOL-1); chput('|'); }
    for (i=0;i<BD_H+2;i++) { cur_mov(BD_SROW+i, BD_SCOL+BD_W*2); chput('|'); }
    cur_mov(BD_SROW+BD_H+1, BD_SCOL-1);
    for (i=0;i<BD_W*2+2;i++) chput('-');
    return 0;
}

int brd_cell(r,c)
int r;
int c;
{
    int piece;
    int rel_r;
    int rel_c;

    piece = board[r][c];

    /* Restore normal piece logic */

    if (game_st == GAME_PLAYING && piece == 0)
    {
        rel_r = r - act_y;
        rel_c = c - act_x;
        if (rel_r >= 0 && rel_r < 4 && rel_c >= 0 && rel_c < 4)
        {
            if (pcell(act_pcs, act_rot, rel_r, rel_c))
            {
                piece = act_pcs;
            }
        }
    }

    return piece;
}

int brd_syn()
{
    int r;
    int c;
    int scr_row;
    int scr_col;
    int color;
    int cur_col;

    rst_col();
    for (r = 0; r < BD_H; r++) {
        scr_row = BD_SROW + r + 1;
        scr_col = BD_SCOL;
        cur_col = 0;
        cur_mov(scr_row, scr_col);
        for (c = 0; c < BD_W; c++) {
            color = board[r][c];
            if (color != 0) {
                color = pcs_col(color);
                if (cur_col != color) {
                    set_col(color);
                    cur_col = color;
                }
                chput('#');
                chput('#');
            } else {
                if (cur_col != 0) {
                    rst_col();
                    cur_col = 0;
                }
                chput(' ');
                chput(' ');
            }
        }
        if (cur_col != 0) {
            rst_col();
        }
    }

    prv_on = 0;
    prv_pc = PIECE_NONE;
    cur_mov(5,1);
    return 0;
}

int clr_act(piece,rot,x,y)
int piece;
int rot;
int x;
int y;
{
    int pr;
    int pc;
    int run_len;
    int run_col;
    int br;
    int bc;
    int scr_row;
    int scr_col;
    int i;

    if (piece<PIECE_I || piece>PIECE_L) return 0;

    rst_col();
    for (pr = 0; pr < 4; pr++) {
        br = y + pr;
        if (br < 0 || br >= BD_H) continue;
        run_len = 0;
        for (pc = 0; pc <= 4; pc++) {
            if (pc < 4 && pcell(piece, rot, pr, pc)) {
                bc = x + pc;
                if (bc >= 0 && bc < BD_W) {
                    if (run_len == 0) run_col = bc;
                    run_len++;
                    continue;
                }
            }
            if (run_len > 0) {
                scr_row = BD_SROW + br + 1;
                scr_col = BD_SCOL + run_col * 2;
                cur_mov(scr_row, scr_col);
                for (i = 0; i < run_len; i++) { chput(' '); chput(' '); }
                run_len = 0;
            }
        }
    }

    return 0;
}

int drw_act(piece,rot,x,y)
int piece;
int rot;
int x;
int y;
{
    int pr;
    int pc;
    int run_len;
    int run_col;
    int br;
    int bc;
    int scr_row;
    int scr_col;
    int i;
    int color;
    int color_on;

    if (piece<PIECE_I || piece>PIECE_L) return 0;

    color = pcs_col(piece);
    color_on = 0;
    for (pr = 0; pr < 4; pr++) {
        br = y + pr;
        if (br < 0 || br >= BD_H) continue;
        run_len = 0;
        for (pc = 0; pc <= 4; pc++) {
            if (pc < 4 && pcell(piece, rot, pr, pc)) {
                bc = x + pc;
                if (bc >= 0 && bc < BD_W) {
                    if (!color_on) { set_col(color); color_on = 1; }
                    if (run_len == 0) run_col = bc;
                    run_len++;
                    continue;
                }
            }
            if (run_len > 0) {
                scr_row = BD_SROW + br + 1;
                scr_col = BD_SCOL + run_col * 2;
                cur_mov(scr_row, scr_col);
                for (i = 0; i < run_len; i++) { chput('#'); chput('#'); }
                run_len = 0;
            }
        }
    }
    if (color_on) {
        rst_col();
    }

    return 0;
}

int brd_drw()
{
    int rot;

    rot = act_rot & 3;

    if (prv_on && game_st == GAME_PLAYING && act_pcs >= PIECE_I && act_pcs <= PIECE_L) {
        if (prv_pc == act_pcs && prv_rt == rot && prv_ax == act_x && prv_ay == act_y) {
            return 0;
        }
    }

    if (prv_on) {
        clr_act(prv_pc, prv_rt, prv_ax, prv_ay);
        prv_on = 0;
    }

    if (game_st == GAME_PLAYING && act_pcs >= PIECE_I && act_pcs <= PIECE_L) {
        drw_act(act_pcs, rot, act_x, act_y);
        prv_ax = act_x;
        prv_ay = act_y;
        prv_rt = rot;
        prv_pc = act_pcs;
        prv_on = 1;
    }

    cur_mov(5,1);
    return 0;
}

int drw_nxt()
{
    int i,j,ch;
    rst_col();
    cur_mov(PRV_ROW, PRV_COL); cputs("Next:");
    cur_mov(PRV_ROW+1, PRV_COL); cputs("    ");
    cur_mov(PRV_ROW+2, PRV_COL); cputs("    ");
    cur_mov(PRV_ROW+3, PRV_COL); cputs("    ");
    cur_mov(PRV_ROW+4, PRV_COL); cputs("    ");
    if (nxt_pcs<PIECE_I || nxt_pcs>PIECE_L) return 0;
    ch = pcs_chr(nxt_pcs);
    set_col(pcs_col(nxt_pcs));
    for (i=0;i<4;i++) for (j=0;j<4;j++) if (pcell(nxt_pcs,0,i,j)) { cur_mov(PRV_ROW+1+i, PRV_COL+j); chput(ch); }
    rst_col();
    return 0;
}

int upd_stat()
{
    cur_mov(5,1); cputs("Score: "); putnum(score);
    cputs("                    ");
    cur_mov(6,1); cputs("Lines: "); putnum(lines_cl);
    cputs("                    ");
    cur_mov(7,1); cputs("Level: "); putnum(level);
    cputs("                    ");
    return 0;
}

/* ========================= Game logic ========================= */
int is_pos(piece,rot,x,y)
int piece,rot,x,y;
{
    int i,j,bx,by;
    if (piece<PIECE_I || piece>PIECE_L) return 0;
    for (i=0;i<4;i++) for (j=0;j<4;j++) if (pcell(piece,rot,i,j)) {
        bx = x+j; by = y+i;
        if (bx<0 || bx>=BD_W || by>=BD_H) return 0;
        if (by>=0 && board[by][bx]!=0) return 0; /* allow above top */
    }
    return 1;
}

int plc_pcs()
{
    int i,j,bx,by;
    for (i=0;i<4;i++) for (j=0;j<4;j++) if (pcell(act_pcs,act_rot,i,j)) {
        bx = act_x + j; by = act_y + i;
        if (by>=0 && by<BD_H && bx>=0 && bx<BD_W) board[by][bx] = act_pcs;
    }
    prv_on = 0;
    prv_pc = PIECE_NONE;
    act_pcs = PIECE_NONE;
    return 0;
}

int clr_lin()
{
    int r,c,s,full,found; found=0;
    for (r=BD_H-1;r>=0;r--) {
        full=1; for (c=0;c<BD_W;c++) if (board[r][c]==0) { full=0; break; }
        if (full) {
            found++;
            for (s=r;s>0;s--) for (c=0;c<BD_W;c++) board[s][c]=board[s-1][c];
            for (c=0;c<BD_W;c++) board[0][c]=0;
            r++;
        }
    }
    if (found>0) {
        lines_cl += found;
        if (found==1) score += 40*(level+1);
        else if (found==2) score += 100*(level+1);
        else if (found==3) score += 300*(level+1);
        else if (found==4) score += 1200*(level+1);
        level = lines_cl/10; if (level>20) level=20;
        fall_sp = 2; /* Keep constant moderate speed */
        brd_syn();
        upd_stat();
    }
    return found;
}

int spn_new()
{
    act_pcs = nxt_pcs; act_rot = 0; act_x = 3; act_y = 0; fall_tm = 0;
    if (!is_pos(act_pcs,act_rot,act_x,act_y)) { game_st = GAME_OVER; return 0; }
    prv_on = 0;
    brd_drw();
    nxt_pcs = rnd_pcs(); /* Random next piece */
    drw_nxt();
    upd_stat();
    return 1;
}

/* ========================= Input ========================= */
int key_rdy() { return (bdos(11) & 0xFF); }
int get_chr() { return (bdos(6,0xFF) & 0xFF); }

int rd_key()
{ if (!key_rdy()) return 0; return get_chr(); }

int hnd_inp()
{
    int k,nx,nr;
    int moved;
    int scch;
    int redraw;
    if (!key_rdy()) return 0;
    k = rd_key(); if (k==0) return 0;
    if (k==KEY_ESC) { game_st = GAME_QUIT; return 1; }
    if (game_st!=GAME_PLAYING) return 0;

    moved = 0;
    scch = 0;
    redraw = 0;

    if (k==KEY_LEFT)
    {
        nx = act_x - 1;
        if (is_pos(act_pcs,act_rot,nx,act_y))
        {
            act_x = nx;
            moved = 1;
            redraw = 1;
        }
    }
    else if (k==KEY_RIGHT)
    {
        nx = act_x + 1;
        if (is_pos(act_pcs,act_rot,nx,act_y))
        {
            act_x = nx;
            moved = 1;
            redraw = 1;
        }
    }
    else if (k==KEY_UP)
    {
        nr = (act_rot + 1) & 3;
        if (is_pos(act_pcs,nr,act_x,act_y))
        {
            act_rot = nr;
            moved = 1;
            redraw = 1;
        }
    }
    else if (k==KEY_DOWN)
    {
        soft_dr = 1;
    }
    else if (k==KEY_SPACE)
    {
        while (is_pos(act_pcs,act_rot,act_x,act_y+1))
        {
            act_y++;
            score += 2;
            scch = 1;
        }
        fall_tm = 0;
        moved = 1;
        redraw = 1;
    }

    if (redraw || scch || soft_dr)
    {
        brd_drw();
    }

    if (scch)
    {
        upd_stat();
    }

    return 0;
}

/* ========================= Game over ========================= */
int show_go()
{
    cur_mov(15,5); cputs("GAME OVER!");
    cur_mov(16,5); cputs("Final Score: "); putnum(score);
    cur_mov(17,5); cputs("Lines Cleared: "); putnum(lines_cl);
    cur_mov(18,5); cputs("Level Reached: "); putnum(level);
    cur_mov(19,5); cputs("Press ESC twice to quit");
    return 0;
}

/* ========================= Main ========================= */
int main()
{
    int r,c,key; int drop_sp;

    game_st = GAME_PLAYING; score=0; lines_cl=0; level=0; fall_tm=0; fall_sp=2; soft_dr=0;
    for (r=0;r<BD_H;r++) for (c=0;c<BD_W;c++) board[r][c]=0;
    init_shp();

    clr_scr(); hid_cur(); drw_ins(); drw_bor(); brd_syn(); brd_drw(); upd_stat();



    nxt_pcs = rnd_pcs(); if (!spn_new()) { /* immediate game over */ }

    x_tmrset(TIMER_ID, DELAY_MS);

    while (game_st==GAME_PLAYING) {
        if (key_rdy())
        {
            hnd_inp();
        }

        if (!x_tmrexp(TIMER_ID))
        {
            x_tmrset(TIMER_ID, DELAY_MS);

            drop_sp = soft_dr ? 2 : fall_sp;
            fall_tm++;

            if (fall_tm >= drop_sp)
            {
                if (is_pos(act_pcs,act_rot,act_x,act_y+1))
                {
                    act_y++;
                    brd_drw();
                    upd_stat();
                    if (soft_dr)
                    {
                        score += 1;
                    }
                }
                else
                {
                    plc_pcs();
                    brd_drw();
                    clr_lin();
                    if (!spn_new()) break;
                }

                fall_tm = 0;
                soft_dr = 0;
            }
        }
    }

    if (game_st==GAME_OVER) {
        brd_drw(); upd_stat();
        show_go();
        while (1) { if (key_rdy()) { key = rd_key(); if (key==KEY_ESC) break; } }
    }

    cur_mov(24,1); shw_cur(); cputs("Thanks for playing Tetris!\r\n");
    return 0;
}
