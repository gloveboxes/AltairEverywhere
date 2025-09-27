/*
 * dx.c - BDS C 1.6 compatible data exchange helpers (hardware only)
 */

#include "stdio.h"

#define WG_IDX_RESET 109
#define WWG_SET_URL  112
#define WG_FILENAME  114
#define WG_EP_NAME   110

#define WG_STATUS    33
#define WG_EOF       0
#define WG_FAILED    3
#define WG_DATAREADY 2
#define WG_GET_BYTE  201
#define WG_WAITING   1

#define STG_FILENAME 68
#define STG_EOF      68
#define STG_GET_BYTE 202

#define GET_STRING  200
#define WG_GET_URL  111
#define WG_SELECTED 113

int inp();
int outp();
int fputc();

int dxwebfn(filename, len, endpoint)
char *filename;
int len;
int endpoint;
{
    int c;

    outp(WWG_SET_URL, endpoint);
    outp(WG_IDX_RESET, 0);

    for (c = 0; c < len; c++)
    {
        outp(WG_FILENAME, filename[c]);
    }
    outp(WG_FILENAME, 0);
    return 0;
}

int dxwebcpy(fp_output, bytes_written)
FILE *fp_output;
int *bytes_written;
{
    int status;
    int count;
    int next_byte;

    count = 0;

    while (1)
    {
        status = inp(WG_STATUS) & 255;
        if (status == WG_EOF)
        {
            break;
        }

        if (status == WG_FAILED)
        {
            return -1;
        }

        if (status == WG_DATAREADY)
        {
            next_byte = inp(WG_GET_BYTE) & 255;
            fputc(next_byte, fp_output);
            count++;
        }
        else if (status == WG_WAITING)
        {
            /* nothing to do */
        }
    }

    if (bytes_written != 0)
    {
        *bytes_written = count;
    }

    return 0;
}

int dxstgfn(filename)
char *filename;
{
    int c;
    int len;

    len = 0;
    while (filename[len] != 0)
    {
        len++;
    }

    for (c = 0; c < len; c++)
    {
        outp(STG_FILENAME, filename[c]);
    }
    outp(STG_FILENAME, 0);
    return 0;
}

int dxstgcpy(fp_output, bytes_written)
FILE *fp_output;
int *bytes_written;
{
    int ch;
    int count;

    count = 0;
    while ((ch = inp(STG_EOF) & 255) == 0)
    {
        fputc(inp(STG_GET_BYTE) & 255, fp_output);
        count++;
    }

    if (bytes_written != 0)
    {
        *bytes_written = count;
    }

    return 0;
}

int dxgeturl(endpoint, buffer, buflen)
int endpoint;
char *buffer;
int buflen;
{
    int ch;
    int count;

    if (buffer == 0)
    {
        return -1;
    }

    if (buflen < 1)
    {
        return -1;
    }

    outp(WG_GET_URL, 0);
    count = 0;
    ch = inp(GET_STRING) & 255;
    while (ch != 0 && count < buflen - 1)
    {
        buffer[count] = ch;
        count++;
        ch = inp(GET_STRING) & 255;
    }
    buffer[count] = 0;

    return count;
}

int dxseturl(endpoint, len)
char *endpoint;
int len;
{
    int c;

    outp(WG_IDX_RESET, 0);
    for (c = 0; c < len; c++)
    {
        outp(WG_EP_NAME, endpoint[c]);
    }
    outp(WG_EP_NAME, 0);
    return 0;
}

int dxgetsel(buffer, buflen)
char *buffer;
int buflen;
{
    int ch;
    int count;

    if (buffer == 0)
    {
        return -1;
    }

    if (buflen < 1)
    {
        return -1;
    }

    outp(WG_SELECTED, 0);
    count = 0;
    ch = inp(GET_STRING) & 255;
    while (ch != 0 && count < buflen - 1)
    {
        buffer[count] = ch;
        count++;
        ch = inp(GET_STRING) & 255;
    }
    buffer[count] = 0;

    return count;
}
