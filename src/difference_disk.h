#pragma once

#include "uthash.h"

// CacheEntry is 176 bytes
// 800 items is stable
#define MAX_CACHE_ITEMS 800
#define SECTOR_LENGTH 137

uint8_t *find_in_cache(int disk_number, int sector_number_key);
void add_to_cache(int disk_number, int sector_number_key, uint8_t *sector);
void delete_all();