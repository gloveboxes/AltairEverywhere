/*
 * DISKRW.C - Disk Read/Write Performance Test
 * 
 * BDS C port of diskrw.bas - tests file I/O performance
 * Creates and reads back 500 test records, repeated 1 million times
 * 
 * Usage: B>diskrw
 */

#include <stdio.h>

#define RECORDS 3000
#define MAX_LOOPS 1000
#define FILENAME "B:LIST.TXT"
#define PROGRESS_INTERVAL 100

/* Global variables for large data structures */
char record_buffer[80];
int loop_count;
int record_count;

/* Forward declarations */
int write_test_file();
int read_test_file();

int main()
{
    printf("Disk Read/Write Performance Test\n");
    printf("Creating %d records, %d iterations\n\n", RECORDS, MAX_LOOPS);
    
    for (loop_count = 1; loop_count <= MAX_LOOPS; loop_count++) {
        printf("Loop %d of %d\n", loop_count, MAX_LOOPS);
        
        /* Write test records */
        if (write_test_file() != 0) {
            printf("Error writing test file\n");
            return 1;
        }
        
        /* Read back test records */
        if (read_test_file() != 0) {
            printf("Error reading test file\n");
            return 1;
        }
    }
    
    printf("\nPerformance test completed successfully\n");
    return 0;
}

int write_test_file()
{
    FILE *fp;
    int i;
    char temp_str[10];
    
    /* Open file for writing */
    fp = fopen(FILENAME, "w");
    if (fp == NULL) {
        printf("Cannot create file: %s\n", FILENAME);
        return 1;
    }
    
    /* Write test records */
    for (i = 1; i <= RECORDS; i++) {
        fputs("THIS IS TEST RECORD #", fp);
        sprintf(temp_str, "%d", i);
        fputs(temp_str, fp);
        fputc('\n', fp);
        
        /* Show progress every 100 records */
        if (i % PROGRESS_INTERVAL == 0) {
            printf("WRITE #%d\n", i);
        }
    }
    
    fclose(fp);
    return 0;
}

int read_test_file()
{
    FILE *fp;
    int ch, pos, records_read;
    char line_buffer[80];
    
    /* Open file for reading */
    fp = fopen(FILENAME, "r");
    if (fp == NULL) {
        printf("Cannot open file: %s\n", FILENAME);
        return 1;
    }
    
    records_read = 0;
    pos = 0;
    
    /* Read file efficiently using fgetc but with buffering logic */
    while (records_read < RECORDS && (ch = fgetc(fp)) != EOF) {
        
        /* Check for CP/M EOF marker */
        if (ch == 26) {
            printf("CP/M EOF marker found at record %d\n", records_read + 1);
            break;
        }
        
        /* Build line until newline */
        if (ch == '\n') {
            line_buffer[pos] = '\0';
            printf("%s\n", line_buffer);
            records_read++;
            pos = 0;
        } else if (pos < 79) {
            line_buffer[pos++] = ch;
        }
    }
    
    if (records_read < RECORDS) {
        printf("Read %d records (expected %d)\n", records_read, RECORDS);
    }
    
    fclose(fp);
    return 0;
}