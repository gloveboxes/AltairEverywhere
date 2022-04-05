#include "difference_disk.h"

struct CacheEntry
{
    int sector_number_key;
    uint8_t sector[SECTOR_LENGTH];
    UT_hash_handle hh;
};

static struct CacheEntry *cache = NULL;

uint8_t *find_in_cache(int disk_number, int sector_number_key)
{
    struct CacheEntry *entry;

    sector_number_key = sector_number_key + (disk_number * 2000);

    HASH_FIND_INT(cache, &sector_number_key, entry);
    return entry ?  entry->sector : NULL;
}

void add_to_cache(int disk_number, int sector_number_key, uint8_t *sector)
{
    struct CacheEntry *entry;

    // if (HASH_COUNT(cache) < MAX_CACHE_ITEMS)
    // {

        sector_number_key = sector_number_key + (disk_number * 2000);

        HASH_FIND_INT(cache, &sector_number_key, entry); /* id already in the hash? */
        if (entry == NULL)
        {
            entry = malloc(sizeof(struct CacheEntry));
            entry->sector_number_key = sector_number_key;

            HASH_ADD_INT(cache, sector_number_key, entry);
        }

        memcpy(entry->sector, sector, SECTOR_LENGTH);
    // }
}

void delete_all()
{
    struct CacheEntry *entry, *tmp_entry;

    HASH_ITER(hh, cache, entry, tmp_entry)
    {
        HASH_DEL(cache, entry); /* delete; users advances to next */
        free(entry);            /* optional- if you want to free  */
    }
}