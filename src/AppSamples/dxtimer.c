/* ============================================================
 * LLM RULES FOR GENERATING BDS C CODE (Altair 8800 / CP/M)
 * ============================================================
 *
 * 1. Syntax:
 *    - Use K&R (BDS C) style: return_type name(args) on next line
 *    - No ANSI prototypes, no "void", no modern keywords
 *    - All function definitions and calls must follow BDS C rules
 *
 * 2. Symbols (VERY IMPORTANT):
 *    - All symbol names (functions, variables, labels, statics, globals)
 *      must be unique in their first 7 characters
 *    - Prefer short, descriptive names, e.g. "x_delay", "x_tmrset"
 *    - Avoid underscores beyond the leading "x_" unless necessary
 *    - Do not exceed 7 characters for clarity and linker safety
 *
 * 3. Types:
 *    - Use int or unsigned (16-bit) for parameters and locals
 *    - Use long.c for longs
 *    - Explicitly declare return type (no implicit int)
 *
 *
 * 6. Style:
 *    - Add a short comment block before each function
 *    - Keep indentation simple (max 4 spaces)
 *    - No C99/C89 features (stick to 1980-era BDS C)
 *
 *
 * ============================================================
 */

/* Altair 8800 timer functions for BDS C
 *
 * Ports:
 *  28 - Set milliseconds timer high byte (bits 15-8)
 *  29 - Set milliseconds timer low byte (bits 7-0) and start timer
 */

#define TMR_MSH   28
#define TMR_MSL   29
#define CHUNK_MAX 255

/* BDS C I/O entry points */
int inp(); /* int inp(port) */
outp();    /* void outp(port,val) */

/* ------------------------------------------------------- */
/* x_delay(ms) - Blocking delay in milliseconds (0..65535).*/
x_delay(ms) unsigned ms;
{
    char hi_byte, lo_byte;

    /* Set high byte (bits 15-8) */
    hi_byte = ms >> 8;
    outp(TMR_MSH, hi_byte);

    /* Set low byte (bits 7-0) and start timer */
    lo_byte = ms & 0xFF;
    outp(TMR_MSL, lo_byte);

    /* Poll until timer expires (inp returns 0 when expired) */
    while (inp(TMR_MSL))
        ;
}

/* ------------------------------------------------------- */
/* x_tmrset(ms) - Start non-blocking timer for given milliseconds.
 */
x_tmrset(ms) unsigned ms;
{
    char hi_byte, lo_byte;

    /* Set high byte (bits 15-8) */
    hi_byte = ms >> 8;
    outp(TMR_MSH, hi_byte);

    /* Set low byte (bits 7-0) and start timer */
    lo_byte = ms & 0xFF;
    outp(TMR_MSL, lo_byte);
}

/* ------------------------------------------------------- */
/* x_tmrexp() - Check if non-blocking timer has expired.
 * Returns 0 if expired, nonzero if still running.
 */
x_tmrexp()
{
    return inp(29);
}
