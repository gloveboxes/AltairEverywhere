#pragma once

#include <fcntl.h>
#include <linux/fb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PI_SENSE_8x8_BUFFER_SIZE (64 * sizeof(uint16_t))

void pi_sense_hat_init(void);
bool pi_sense_8x8_panel_update(uint16_t *panel_buffer, size_t buffer_len);
