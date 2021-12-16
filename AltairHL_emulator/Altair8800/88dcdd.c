#include "88dcdd.h"

#define DISK_DEBUG
// #define DISK_DEBUG_VERBOSE
// #define VDISK_TRACE

static VDISK_SECTOR_T vdisk_sector;
static pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
// static pthread_cond_t intercore_disk_cond = PTHREAD_COND_INITIALIZER;
// static pthread_mutex_t intercore_disk_lock = PTHREAD_MUTEX_INITIALIZER;

static vdisk_mqtt_write_sector_t write_sector;
static bool read_from_cache = false;
static uint32_t sector_requested = UINT32_MAX;

typedef enum { TRACK_MODE, SECTOR_MODE } DISK_SELECT_MODE;
void writeSector(disk_t *pDisk, uint8_t drive_number);
disks disk_drive;

// static uint16_t cal_crc(uint8_t *sector)
//{
//    uint16_t crc = 0x0000;
//    for (int x = 0; x < 137; x++) {
//        crc += intercore_disk_block.sector[x];
//        crc &= 0xffff;
//    }
//
//    return crc;
//}

static void vdisk_cache_write_sector(uint8_t *sectorData, uint16_t sector_number)
{
    intercore_disk_block.disk_ic_msg_type = DISK_IC_WRITE;
    intercore_disk_block.sector_number = sector_number;
    memcpy(intercore_disk_block.sector, sectorData, SECTOR_SIZE);

    dx_intercorePublish(&intercore_disk_cache_ctx, &intercore_disk_block, sizeof(INTERCORE_DISK_DATA_BLOCK_T));
}

static void vdisk_cache_read_sector(disk_t *pDisk)
{
    read_from_cache = false;
    uint16_t requested_sector_number = (uint16_t)(pDisk->diskPointer / 137);

    // prepare and send intercore message requesting offset from cache
    intercore_disk_block.disk_ic_msg_type = DISK_IC_READ;
    intercore_disk_block.sector_number = requested_sector_number;
    intercore_disk_block.cached = false;

    //// NOTE, the cache server on M4 must be compiled in released mode, optimised for speed to service read requests in time
    //// measure using finish clock_gettime(CLOCK_MONOTONIC, &finish); - start clock_gettime(CLOCK_MONOTONIC, &start);
    //// measurements were timed using release build with debugger attached for log_debug time stats
    //// Measurements averaged over 1000 read requests from the cache
    //// Average response time measured before request sent
    //// Avg cache read response 208279 nanoseconds
    //// Avg cache read response 211613 nanoseconds
    //// Avg cache read response 210598 nanoseconds
    //// Avg cache read response 211290 nanoseconds

    //// Average response time measured after request sent
    //// Avg cache read response 45421 nanoseconds
    //// Avg cache read response 45469 nanoseconds
    //// Avg cache read response 45774 nanoseconds
    //// Avg cache read response 45781 nanoseconds
    //// Avg cache read response 45772 nanoseconds

    if (dx_intercorePublishThenRead(&intercore_disk_cache_ctx, &intercore_disk_block, 3) < 0) {
        return;
    }

    if (intercore_disk_block.cached && intercore_disk_block.sector_number == requested_sector_number) {
        memcpy(pDisk->sectorData, intercore_disk_block.sector, SECTOR_SIZE);
        pDisk->sectorPointer = 0;
        read_from_cache = true;
        (*(int *)dt_diskCacheHits.propertyValue)++;
    } else {
        (*(int *)dt_diskCacheMisses.propertyValue)++;
    }
}

void vdisk_mqtt_response_cb(uint8_t *sector)
{
    // first 4 bytes are the sector number in the response from the virtual disk server
    if (*(uint32_t *)(sector) == sector_requested) {
        sector_requested = UINT32_MAX;

        pthread_mutex_lock(&lock);

        // Skip the first 4 bytes as they are the sector data
        memcpy(vdisk_sector.data, sector + 4, 137);
        vdisk_sector.dirty = true;

        pthread_cond_signal(&cond1);
        pthread_mutex_unlock(&lock);
    }
}

int write_virtual_sector(disk_t *pDisk)
{
    // The sector size presented to this API is 138 bytes
    // This includes a terminating control byte
    // Altair 8inch floopy sector size is 137 so use SECTOR_SIZE = 137 bytes
    write_sector.offset = pDisk->diskPointer;
    memcpy(write_sector.data, pDisk->sectorData, SECTOR_SIZE);

    vdisk_cache_write_sector(pDisk->sectorData, (uint16_t)(pDisk->diskPointer / 137));
    vdisk_mqtt_write_sector(&write_sector);

    (*(int *)dt_diskTotalWrites.propertyValue)++;

    return 0;
}

bool read_virtual_sector(disk_t *pDisk)
{
    struct timespec now = {0, 0};
    vdisk_sector.dirty = false;
    bool result = false;

    vdisk_cache_read_sector(pDisk);

    if (read_from_cache) {
        result = true;
    } else {

        // save sector requested to be compared with inbound mqtt sector
        sector_requested = pDisk->diskPointer;

        // data not found in cache - try reading from vdisk storage
        vdisk_mqtt_read_sector(pDisk->diskPointer);

        // allow up to 8 seconds for the io to complete
        clock_gettime(CLOCK_REALTIME, &now);
        now.tv_sec += 8;

        pthread_mutex_lock(&lock);
        pthread_cond_timedwait(&cond1, &lock, &now);
        pthread_mutex_unlock(&lock);

        if (vdisk_sector.dirty) {
            memcpy(pDisk->sectorData, vdisk_sector.data, SECTOR_SIZE);
            pDisk->sectorPointer = 0;

            vdisk_cache_write_sector(pDisk->sectorData, (uint16_t)(pDisk->diskPointer / 137));

            result = true;

        } else {
            // Log_Debug("VDISK Read Fail\n");
            (*(int *)dt_diskTotalErrors.propertyValue)++;
        }
    }

    return result;
}

void set_status(uint8_t bit)
{
    disk_drive.current->status &= (uint8_t)~bit;
}

void clear_status(uint8_t bit)
{
    disk_drive.current->status |= bit;
}

void disk_select(uint8_t b)
{
    uint8_t select = b & 0xf;
    disk_drive.currentDisk = select;

    if (select == 0) {
        disk_drive.current = &disk_drive.disk1;
    } else if (select == 1) {
        disk_drive.current = &disk_drive.disk2;
    } else {
        disk_drive.current = &disk_drive.nodisk;
    }
}

uint8_t disk_status()
{
    return disk_drive.current->status;
}

void disk_function(uint8_t b)
{
    if (b & CONTROL_STEP_IN) {
        disk_drive.current->track++;
        disk_drive.current->sector = 0;

        if (disk_drive.current->track != 0) {
            clear_status(STATUS_TRACK_0);
        }

        uint32_t seek_offset = TRACK * disk_drive.current->track;

#ifndef SD_CARD_ENABLED
        if (disk_drive.currentDisk != 0 && disk_drive.current->sectorDirty) {
            writeSector(disk_drive.current, disk_drive.currentDisk);
        }

        if (disk_drive.currentDisk == 0) {
            lseek(disk_drive.current->fp, seek_offset, SEEK_SET);
        }
#else
        if (disk_drive.current->sectorDirty) {
            writeSector(disk_drive.current, disk_drive.currentDisk);
        }
#endif
        disk_drive.current->diskPointer = seek_offset;
        disk_drive.current->haveSectorData = false;
        disk_drive.current->sectorPointer = 0;
    }

    if (b & CONTROL_STEP_OUT) {
        if (disk_drive.current->track > 0) {
            disk_drive.current->track--;
        }

        if (disk_drive.current->track == 0) {
            set_status(STATUS_TRACK_0);
        }

        disk_drive.current->sector = 0;
        uint32_t seek_offset = TRACK * disk_drive.current->track;

#ifndef SD_CARD_ENABLED
        if (disk_drive.currentDisk != 0 && disk_drive.current->sectorDirty) {
            writeSector(disk_drive.current, disk_drive.currentDisk);
        }

        if (disk_drive.currentDisk == 0) {
            lseek(disk_drive.current->fp, seek_offset, SEEK_SET);
        }
#else
        if (disk_drive.current->sectorDirty) {
            writeSector(disk_drive.current, disk_drive.currentDisk);
        }
#endif
        disk_drive.current->diskPointer = seek_offset;
        disk_drive.current->haveSectorData = false;
        disk_drive.current->sectorPointer = 0;
    }

    if (b & CONTROL_HEAD_LOAD) {
        set_status(STATUS_HEAD);
        set_status(STATUS_NRDA);
    }

    if (b & CONTROL_HEAD_UNLOAD) {
        clear_status(STATUS_HEAD);
    }

    if (b & CONTROL_IE) {
    }

    if (b & CONTROL_ID) {
    }

    if (b & CONTROL_HCS) {
    }

    if (b & CONTROL_WE) {
        set_status(STATUS_ENWD);
        disk_drive.current->write_status = 0;
    }
}

uint8_t sector()
{
    uint32_t seek_offset;
    uint8_t ret_val;

    if (disk_drive.current->sector == 32) {
        disk_drive.current->sector = 0;
    }

#ifndef SD_CARD_ENABLED
    if (disk_drive.currentDisk != 0 && disk_drive.current->sectorDirty) {
        writeSector(disk_drive.current, disk_drive.currentDisk);
    }
#else
    if (disk_drive.current->sectorDirty) {
        writeSector(disk_drive.current, disk_drive.currentDisk);
    }
#endif

    seek_offset = disk_drive.current->track * TRACK + disk_drive.current->sector * (SECTOR_SIZE);
    disk_drive.current->sectorPointer = 0;
#ifndef SD_CARD_ENABLED
    if (disk_drive.currentDisk == 0) {
        lseek(disk_drive.current->fp, seek_offset, SEEK_SET);
    }
#endif
    disk_drive.current->diskPointer = seek_offset;
    disk_drive.current->sectorPointer = 0; // needs to be set here for write operation (read fetches sector data and resets the pointer).
    disk_drive.current->haveSectorData = false;

    ret_val = (uint8_t)(disk_drive.current->sector << 1);

    disk_drive.current->sector++;
    return ret_val;
}

void disk_write(uint8_t b)
{

#ifndef SD_CARD_ENABLED
    if (disk_drive.currentDisk != 0) {
        // calculate file offset from TRACK and offset.
        disk_drive.current->sectorData[disk_drive.current->sectorPointer++] = b;
        disk_drive.current->sectorDirty = true;
    }
#else
    disk_drive.current->sectorData[disk_drive.current->sectorPointer++] = b;
    disk_drive.current->sectorDirty = true;
#endif

    if (disk_drive.current->write_status == 137) {
        disk_drive.current->write_status = 0;
        clear_status(STATUS_ENWD);
    } else
        disk_drive.current->write_status++;
}

uint8_t disk_read()
{
#ifdef SD_CARD_ENABLED
    if (!disk_drive.current->haveSectorData) {
        disk_drive.current->sectorPointer = 0;

        memset(intercore_disk_block.sector, 0x00, sizeof(intercore_disk_block.sector));
        intercore_disk_block.cached = false;
        intercore_disk_block.success = false;
        intercore_disk_block.drive_number = disk_drive.currentDisk;
        intercore_disk_block.sector_number = (uint16_t)(disk_drive.current->diskPointer / 137);
        intercore_disk_block.disk_ic_msg_type = DISK_IC_READ;

        dx_intercorePublishThenRead(&intercore_sd_card_ctx, &intercore_disk_block, sizeof(intercore_disk_block));

        if (intercore_disk_block.success) {
            disk_drive.current->haveSectorData = true;
            memcpy(disk_drive.current->sectorData, intercore_disk_block.sector, 137);
        }
    }
#else
    if (disk_drive.currentDisk == 0) {
        if (!disk_drive.current->haveSectorData) {

            disk_drive.current->sectorPointer = 0;
            disk_drive.current->haveSectorData = true;
            read(disk_drive.current->fp, disk_drive.current->sectorData, SECTOR_SIZE);
        }
    } else {
        if (!disk_drive.current->haveSectorData) {
            disk_drive.current->haveSectorData = true;
            disk_drive.current->sectorPointer = 0;
            if (!read_virtual_sector(disk_drive.current)) {
                Log_Debug("Virtual disk sector read failed\n");
            }
        }
    }

#endif // SD_CARD_ENABLED

    return disk_drive.current->sectorData[disk_drive.current->sectorPointer++];
}

void writeSector(disk_t *pDisk, uint8_t drive_number)
{
#ifdef SD_CARD_ENABLED

    memcpy(intercore_disk_block.sector, pDisk->sectorData, 137);
    intercore_disk_block.cached = false;
    intercore_disk_block.success = false;
    intercore_disk_block.drive_number = drive_number;
    intercore_disk_block.sector_number = (uint16_t)(disk_drive.current->diskPointer / 137);
    intercore_disk_block.disk_ic_msg_type = DISK_IC_WRITE;

    dx_intercorePublishThenRead(&intercore_sd_card_ctx, &intercore_disk_block, sizeof(intercore_disk_block));

    // if (intercore_disk_block.success) {
    //    Log_Debug("block written: %d\n", (uint16_t)(disk_drive.current->diskPointer / 137));
    //} else {
    //    Log_Debug("block NOT written\n");
    //}

#else
    write_virtual_sector(pDisk);

#endif // SD_CARD_ENABLED

    pDisk->sectorPointer = 0;
    pDisk->sectorDirty = false;
}
