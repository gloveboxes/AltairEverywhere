#ifndef _88DCDD_H_
#define _88DCDD_H_

#include "difference_disk.h"
#include "types.h"
#include "utils.h"
#include <applibs/log.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define STATUS_ENWD			1
#define STATUS_MOVE_HEAD	2
#define STATUS_HEAD			4
#define STATUS_IE			32
#define STATUS_TRACK_0		64
#define STATUS_NRDA			128

#define CONTROL_STEP_IN		1
#define CONTROL_STEP_OUT	2
#define CONTROL_HEAD_LOAD	4
#define CONTROL_HEAD_UNLOAD 8
#define CONTROL_IE			16
#define CONTROL_ID			32
#define CONTROL_HCS			64
#define CONTROL_WE			128

#define SECTOR_SIZE 137UL
#define TRACK (32UL*SECTOR_SIZE)

// Disk drive constants
#define MAX_DRIVES          4
#define MAX_TRACKS          77
#define SECTORS_PER_TRACK   32
#define DRIVE_SELECT_MASK   0x0F
#define SECTOR_SHIFT_BITS   1

// Drive numbers
#define DRIVE_A             0
#define DRIVE_B             1
#define DRIVE_C             2
#define DRIVE_D             3

typedef struct
{
	int		fp;
	uint8_t track;
	uint8_t sector;
	uint8_t status;
	uint8_t write_status;
	uint32_t diskPointer;
	uint8_t sectorPointer;
	uint8_t sectorData[SECTOR_SIZE + 2];
	bool sectorDirty;
	bool haveSectorData;
} disk_t;

typedef struct
{
	disk_t disk1;
	disk_t disk2;
	disk_t disk3;
	disk_t disk4;
	disk_t nodisk;
	disk_t *current;
	uint8_t currentDisk;
} disks;

extern disks disk_drive;

void disk_select(uint8_t b);
uint8_t disk_status(void);
void disk_function(uint8_t b);
uint8_t sector(void);
void disk_write(uint8_t b);
uint8_t disk_read(void);
void clear_difference_disk(void);


#endif
