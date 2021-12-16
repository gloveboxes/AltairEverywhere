/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

typedef struct
{
	uint8_t start_character;
	uint8_t end_character;
	uint8_t scroll_cursor;
	bool scroll_completed;
} GFX_SCROLL_LEFT_CONTEXT;

uint8_t gfx_reverse_byte(uint8_t data);
void gfx_load_character(uint8_t character, uint8_t bitmap[8]);
void gfx_reverse_panel(unsigned char A[8]);
void gfx_rotate_counterclockwise(unsigned char A[8], uint32_t m, uint32_t n, unsigned char B[8]);