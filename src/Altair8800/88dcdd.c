#include "88dcdd.h"

// MITS 88-DCDD Disk Controller Emulation
// Implements active-low status bit logic for Altair 8800 floppy disk controller

void writeSector(disk_t *pDisk, uint8_t drive_number);
disks disk_drive;

// Helper function to handle common track positioning logic
static void seek_to_track(void)
{
    uint32_t seek_offset = TRACK * disk_drive.current->track;

    if (disk_drive.current->sectorDirty)
    {
        writeSector(disk_drive.current, disk_drive.currentDisk);
    }

    lseek(disk_drive.current->fp, seek_offset, SEEK_SET);

    disk_drive.current->diskPointer    = seek_offset;
    disk_drive.current->haveSectorData = false;
    disk_drive.current->sectorPointer  = 0;
    disk_drive.current->sector         = 0;
}

// Set status condition to TRUE (clears bit for active-low hardware)
void set_status(uint8_t bit)
{
    disk_drive.current->status &= (uint8_t)~bit;
}

// Set status condition to FALSE (sets bit for active-low hardware)
void clear_status(uint8_t bit)
{
    disk_drive.current->status |= bit;
}

void disk_select(uint8_t b)
{
    uint8_t select         = b & DRIVE_SELECT_MASK;
    disk_drive.currentDisk = select;

    // Array of disk pointers for cleaner selection
    disk_t *drives[] = {&disk_drive.disk1, &disk_drive.disk2, &disk_drive.disk3, &disk_drive.disk4};

    if (select < MAX_DRIVES)
    {
        disk_drive.current = drives[select];
    }
    else
    {
        disk_drive.current     = &disk_drive.disk1;
        disk_drive.currentDisk = DRIVE_A;
    }
}

uint8_t disk_status()
{
    return disk_drive.current->status;
}

void disk_function(uint8_t b)
{
    if (b & CONTROL_STEP_IN)
    {
        if (disk_drive.current->track < MAX_TRACKS - 1)
        {
            disk_drive.current->track++;
        }
        if (disk_drive.current->track != 0)
        {
            clear_status(STATUS_TRACK_0);
        }
        seek_to_track();
    }

    if (b & CONTROL_STEP_OUT)
    {
        if (disk_drive.current->track > 0)
        {
            disk_drive.current->track--;
        }
        if (disk_drive.current->track == 0)
        {
            set_status(STATUS_TRACK_0);
        }
        seek_to_track();
    }

    if (b & CONTROL_HEAD_LOAD)
    {
        set_status(STATUS_HEAD);
        set_status(STATUS_NRDA);
    }

    if (b & CONTROL_HEAD_UNLOAD)
    {
        clear_status(STATUS_HEAD);
    }

    if (b & CONTROL_WE)
    {
        set_status(STATUS_ENWD);
        disk_drive.current->write_status = 0;
    }
}

uint8_t sector()
{
    if (disk_drive.current->sector == SECTORS_PER_TRACK)
    {
        disk_drive.current->sector = 0;
    }

    if (disk_drive.current->sectorDirty)
    {
        writeSector(disk_drive.current, disk_drive.currentDisk);
    }

    uint32_t seek_offset = disk_drive.current->track * TRACK + disk_drive.current->sector * SECTOR_SIZE;

    lseek(disk_drive.current->fp, seek_offset, SEEK_SET);

    disk_drive.current->diskPointer    = seek_offset;
    disk_drive.current->sectorPointer  = 0; // Set once for both read and write operations
    disk_drive.current->haveSectorData = false;

    // Format sector number according to 88-DCDD specification:
    // D7-D6: Always 1 (unused bits)
    // D5-D1: Sector number (0-31)
    // D0: Sector True bit (0 when at beginning of sector, 1 otherwise)
    uint8_t ret_val = 0xC0;                                                // Set D7-D6 to 1
    ret_val |= (uint8_t)(disk_drive.current->sector << SECTOR_SHIFT_BITS); // D5-D1: sector number
    ret_val |= (disk_drive.current->sectorPointer == 0) ? 0 : 1;           // D0: Sector True bit

    disk_drive.current->sector++;
    return ret_val;
}

void disk_write(uint8_t b)
{
    disk_drive.current->sectorData[disk_drive.current->sectorPointer++] = b;
    disk_drive.current->sectorDirty                                     = true;

    if (disk_drive.current->write_status == SECTOR_SIZE)
    {
        writeSector(disk_drive.current, disk_drive.currentDisk);
        disk_drive.current->write_status = 0;
        clear_status(STATUS_ENWD);
    }
    else
    {
        disk_drive.current->write_status++;
    }
}

uint8_t disk_read()
{
    uint16_t requested_sector_number = (uint16_t)(disk_drive.current->diskPointer / SECTOR_SIZE);

    if (!disk_drive.current->haveSectorData)
    {

#ifdef ALTAIR_CLOUD
        disk_drive.current->sectorPointer = 0;

        uint8_t *sector = find_in_cache(disk_drive.current == &disk_drive.disk1 ? DRIVE_A : DRIVE_B, requested_sector_number);
        if (sector)
        {
            disk_drive.current->haveSectorData = true;
            memset(disk_drive.current->sectorData, 0x00, SECTOR_SIZE);
            memcpy(disk_drive.current->sectorData, sector, SECTOR_SIZE);
        }
#endif // ALTAIR_CLOUD

        if (!disk_drive.current->haveSectorData)
        {
            disk_drive.current->sectorPointer = 0;
            memset(disk_drive.current->sectorData, 0x00, SECTOR_SIZE);
            ssize_t bytes = read(disk_drive.current->fp, disk_drive.current->sectorData, SECTOR_SIZE);

            if (bytes != SECTOR_SIZE)
            {
                Log_Debug("Sector read failed. Read %d\n", bytes);
            }
            disk_drive.current->haveSectorData = SECTOR_SIZE == bytes;
        }
    }

    return disk_drive.current->sectorData[disk_drive.current->sectorPointer++];
}

void writeSector(disk_t *pDisk, uint8_t drive_number)
{
    uint16_t requested_sector_number = (uint16_t)(pDisk->diskPointer / SECTOR_SIZE);

#ifdef ALTAIR_CLOUD

    add_to_cache(disk_drive.current == &disk_drive.disk1 ? DRIVE_A : DRIVE_B, requested_sector_number, pDisk->sectorData);

#else

    ssize_t bytes = write(pDisk->fp, pDisk->sectorData, SECTOR_SIZE);

    if (bytes != SECTOR_SIZE)
    {
        Log_Debug("Sector write failed. Wrote %d\n", bytes);
    }

#endif // ALTAIR_CLOUD

    pDisk->sectorPointer = 0;
    pDisk->sectorDirty   = false;
}

void clear_difference_disk(void)
{
    delete_all();
}
