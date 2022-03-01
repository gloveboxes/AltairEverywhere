#include "88dcdd.h"

typedef enum { TRACK_MODE, SECTOR_MODE } DISK_SELECT_MODE;
void writeSector(disk_t *pDisk, uint8_t drive_number);
disks disk_drive;

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

        if (disk_drive.current->sectorDirty) {
            writeSector(disk_drive.current, disk_drive.currentDisk);
        }

        lseek(disk_drive.current->fp, seek_offset, SEEK_SET);

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

        if (disk_drive.current->sectorDirty) {
            writeSector(disk_drive.current, disk_drive.currentDisk);
        }

        lseek(disk_drive.current->fp, seek_offset, SEEK_SET);

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

    if (disk_drive.current->sectorDirty) {
        writeSector(disk_drive.current, disk_drive.currentDisk);
    }

    seek_offset = disk_drive.current->track * TRACK + disk_drive.current->sector * (SECTOR_SIZE);
    disk_drive.current->sectorPointer = 0;

    lseek(disk_drive.current->fp, seek_offset, SEEK_SET);

    disk_drive.current->diskPointer = seek_offset;
    disk_drive.current->sectorPointer = 0; // needs to be set here for write operation (read fetches sector data and resets the pointer).
    disk_drive.current->haveSectorData = false;

    ret_val = (uint8_t)(disk_drive.current->sector << 1);

    disk_drive.current->sector++;
    return ret_val;
}

void disk_write(uint8_t b)
{
    disk_drive.current->sectorData[disk_drive.current->sectorPointer++] = b;
    disk_drive.current->sectorDirty = true;

    if (disk_drive.current->write_status == 137) {
        disk_drive.current->write_status = 0;
        clear_status(STATUS_ENWD);
    } else
        disk_drive.current->write_status++;
}

uint8_t disk_read()
{
    if (!disk_drive.current->haveSectorData) {
        disk_drive.current->sectorPointer = 0;
        memset(disk_drive.current->sectorData, 0x00, SECTOR_SIZE);
        ssize_t bytes = read(disk_drive.current->fp, disk_drive.current->sectorData, SECTOR_SIZE);

        if (bytes != SECTOR_SIZE){
            Log_Debug("Sector read failed. Read %d\n", bytes);
        }
        
        disk_drive.current->haveSectorData = SECTOR_SIZE == bytes;
    }

    return disk_drive.current->sectorData[disk_drive.current->sectorPointer++];
}

void writeSector(disk_t *pDisk, uint8_t drive_number)
{

    ssize_t bytes = write(pDisk->fp, pDisk->sectorData, SECTOR_SIZE);

    if (bytes != SECTOR_SIZE) {
        Log_Debug("Sector write failed. Wrote %d\n", bytes);
    }

    pDisk->sectorPointer = 0;
    pDisk->sectorDirty = false;
}
