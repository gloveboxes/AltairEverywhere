#ifndef _88DCDD_H_
#define _88DCDD_H_

#include "comms_manager_wolf.h"
#include "dx_device_twins.h"
#include "dx_intercore.h"
#include "intercore_contract.h"
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

extern DX_DEVICE_TWIN_BINDING dt_diskCacheHits;
extern DX_DEVICE_TWIN_BINDING dt_diskCacheMisses;
extern DX_DEVICE_TWIN_BINDING dt_diskTotalWrites;
extern DX_DEVICE_TWIN_BINDING dt_diskTotalErrors;

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
	disk_t nodisk;
	disk_t *current;
	uint8_t currentDisk;
} disks;

extern disks disk_drive;
extern DX_INTERCORE_BINDING intercore_sd_card_ctx;
extern DX_INTERCORE_BINDING intercore_disk_cache_ctx;
extern INTERCORE_DISK_DATA_BLOCK_T intercore_disk_block;

void disk_select(uint8_t b);
uint8_t disk_status(void);
void disk_function(uint8_t b);
uint8_t sector(void);
void disk_write(uint8_t b);
uint8_t disk_read(void);

void vdisk_mqtt_response_cb(uint8_t* sector);
void vdisk_cache_response_cb(INTERCORE_DISK_DATA_BLOCK_T* intercore_disk_block);


typedef struct {
	int sector_number;
	bool dirty;
	uint8_t data[137];
} VDISK_SECTOR_T;



#endif
