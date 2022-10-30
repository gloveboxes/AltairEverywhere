/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include "led_matrix_io.h"

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

#include "front_panel_retro_click.h"

extern DX_ASYNC_BINDING async_start_panel_io;
extern DX_ASYNC_BINDING async_stop_panel_io;
extern DX_TIMER_BINDING tmr_read_panel;
extern DX_TIMER_BINDING tmr_refresh_panel;

static int led_brightness = 0;

#endif

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
#include "front_panel_pi_sense_hat.h"
#include "graphics.h"
#include "sense_hat.h"
#endif

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
// Font buffers
static uint8_t bitmap[8];
uint16_t panel_8x8_buffer[64];

#endif // ALTAIR_FRONT_PANEL_PI_SENSE

#if defined(ALTAIR_FRONT_PANEL_RETRO_CLICK) || defined(ALTAIR_FRONT_PANEL_PI_SENSE)

union {
    uint32_t mask[2];
    uint64_t mask64;
} pixel_mask;

typedef union {
    uint8_t bitmap[8];
    uint64_t bitmap64;
} PIXEL_MAP;

PIXEL_MAP pixel_map;

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK

DX_ASYNC_HANDLER(async_stop_panel_io_handler, handle)
{
#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

    dx_timerStop(&tmr_read_panel);
    dx_timerStop(&tmr_refresh_panel);
    as1115_clear(&retro_click);

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK
}
DX_ASYNC_HANDLER_END

DX_ASYNC_HANDLER(async_start_panel_io_handler, handle)
{
#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

    int brightness = *((int *)handle->data);
    dx_timerStart(&tmr_read_panel);
    dx_timerStart(&tmr_refresh_panel);

    brightness = (brightness < 0) ? 0 : brightness;
    as1115_set_brightness(&retro_click, (unsigned char)brightness);

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK
}
DX_ASYNC_HANDLER_END

size_t led_matrix_output(int port_number, uint8_t data, char *buffer, size_t buffer_length)
{

    switch (port_number)
    {

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

        case 65: // Enable/Disable 8x8 LED Panel and 4x4 keypad
            if (data == 0)
            {
                dx_asyncSend(&async_stop_panel_io, NULL);
            }
            else
            {
                led_brightness = data - 1;
                dx_asyncSend(&async_start_panel_io, &led_brightness);
            }
            break;
#endif

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
        case 65: // Set brightness of the 8x8 LED Panel
            set_led_panel_color(data);
            break;
#endif

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK

        case 80: // panel_mode 0 = bus data, 1 = font, 2 = bitmap
            if (data < 3)
            {
                panel_mode = data;
            }
            break;
        case 85: // display character
            gfx_load_character(data, retro_click.bitmap);
            gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
            gfx_reverse_panel(retro_click.bitmap);
            gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
            as1115_panel_write(&retro_click);
            break;

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
        case 80: // panel_mode 0 = bus data, 1 = font, 2 = bitmap
            if (data < 3)
            {
                panel_mode = data;
            }
            break;
        case 81: // set font color
            gfx_set_color(data);
            break;
        case 82: // set pixel red color
            break;
        case 83: // set pixel green color
            break;
        case 84: // set pixel blue color
            break;
        case 85: // display character
            memset(panel_8x8_buffer, 0x00, sizeof(panel_8x8_buffer));
            gfx_load_character(data, bitmap);
            gfx_rotate_counterclockwise(bitmap, 1, 1, bitmap);
            gfx_reverse_panel(bitmap);
            gfx_rotate_counterclockwise(bitmap, 1, 1, bitmap);
            gfx_bitmap_to_rgb(bitmap, panel_8x8_buffer, sizeof(panel_8x8_buffer));
            pi_sense_8x8_panel_update(panel_8x8_buffer, sizeof(panel_8x8_buffer));
            break;
#endif // PI SENSE HAT

#if defined(ALTAIR_FRONT_PANEL_RETRO_CLICK) || defined(ALTAIR_FRONT_PANEL_PI_SENSE)

        case 90: // Bitmap row 0
            pixel_map.bitmap[0] = data;
            break;
        case 91: // Bitmap row 1
            pixel_map.bitmap[1] = data;
            break;
        case 92: // Bitmap row 2
            pixel_map.bitmap[2] = data;
            break;
        case 93: // Bitmap row 3
            pixel_map.bitmap[3] = data;
            break;
        case 94: // Bitmap row 4
            pixel_map.bitmap[4] = data;
            break;
        case 95: // Bitmap row 5
            pixel_map.bitmap[5] = data;
            break;
        case 96: // Bitmap row 6
            pixel_map.bitmap[6] = data;
            break;
        case 97: // Bitmap row 7
            pixel_map.bitmap[7] = data;
            break;
        case 98: // Pixel on
            if (data < 64)
            {
                pixel_mask.mask64                 = 0;
                pixel_mask.mask[(int)(data / 32)] = data < 32 ? 1u << data : 1u << (data - 32);
                pixel_map.bitmap64                = pixel_map.bitmap64 | pixel_mask.mask64;
            }
            break;
        case 99: // Pixel off
            if (data < 64)
            {
                pixel_mask.mask64                 = 0;
                pixel_mask.mask[(int)(data / 32)] = data < 32 ? 1u << data : 1u << (data - 32);
                pixel_mask.mask64 ^= 0xFFFFFFFFFFFFFFFF;
                pixel_map.bitmap64 = pixel_map.bitmap64 & pixel_mask.mask64;
            }
            break;
        case 100: // Pixel flip
            if (data < 64)
            {
                pixel_mask.mask64                 = 0;
                pixel_mask.mask[(int)(data / 32)] = data < 32 ? 1u << data : 1u << (data - 32);
                pixel_map.bitmap64                = pixel_map.bitmap64 ^ pixel_mask.mask64;
            }
            break;
        case 101: // clear all pixels
            pixel_map.bitmap64 = 0;
            break;

#endif // defined(ALTAIR_FRONT_PANEL_RETRO_CLICK) || defined(ALTAIR_FRONT_PANEL_PI_SENSE)

#ifdef ALTAIR_FRONT_PANEL_RETRO_CLICK
        case 102: // Bitmap draw
            gfx_rotate_counterclockwise(pixel_map.bitmap, 1, 1, retro_click.bitmap);
            gfx_reverse_panel(retro_click.bitmap);
            gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
            // retro_click.bitmap64 = pixel_map.bitmap64;
            as1115_panel_write(&retro_click);
            break;

#endif // ALTAIR_FRONT_PANEL_RETRO_CLICK

#ifdef ALTAIR_FRONT_PANEL_PI_SENSE
        case 102: // Bitmap draw
            gfx_rotate_counterclockwise(pixel_map.bitmap, 1, 1, bitmap);
            gfx_reverse_panel(bitmap);
            gfx_rotate_counterclockwise(bitmap, 1, 1, bitmap);
            gfx_bitmap_to_rgb(bitmap, panel_8x8_buffer, sizeof(panel_8x8_buffer));
            pi_sense_8x8_panel_update(panel_8x8_buffer, sizeof(panel_8x8_buffer));
            break;
#endif // ALTAIR_FRONT_PANEL_PI_SENSE

    } // switch (port_number)

    return 0;
}
