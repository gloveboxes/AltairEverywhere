#include "front_panel_pi_sense_hat.h"

#include "graphics.h"
#include "led_panel.h"
#include "led_matrix_io.h"
#include "sense_hat.h"
#include <string.h>

static uint16_t panel_buffer[NUM_OF_LEDS];
static int rgb_color = 19 << 1;
static bool sense_hat_initialized;

typedef union
{
    uint8_t bitmap[8];
    uint64_t bitmap64;
} PIXEL_MAP;

static PIXEL_MAP pixel_map;

static uint16_t panel_8x8_buffer[NUM_OF_LEDS];
static uint8_t bitmap_rows[8];

static void uint8_to_uint16_t(uint8_t bitmap, uint16_t *buffer)
{
    uint16_t mask = 1;
    uint8_t pixel_number = 0;

    while (pixel_number < 8)
    {
        buffer[pixel_number++] = (bitmap & mask) ? rgb_color : 0x0000;
        mask                   = (uint16_t)(mask << 1);
    }
}

bool sense_hat_front_panel_init(void)
{
    if (sense_hat_initialized)
    {
        return true;
    }

    if (!pi_sense_hat_sensors_init(1))
    {
        return false;
    }

    if (!pi_sense_hat_init())
    {
        pi_sense_hat_sensors_close();
        return false;
    }

    sense_hat_initialized = true;
    return true;
}

void sense_hat_front_panel_shutdown(void)
{
    if (!sense_hat_initialized)
    {
        return;
    }

    pi_sense_hat_sensors_close();
    pi_sense_hat_close();
    sense_hat_initialized = false;
    memset(panel_buffer, 0, sizeof(panel_buffer));
    memset(panel_8x8_buffer, 0, sizeof(panel_8x8_buffer));
    pixel_map.bitmap64 = 0;
}

void sense_hat_set_led_panel_color(int color)
{
    if (!sense_hat_initialized)
    {
        return;
    }

    if (color != 0 && color < 3)
    {
        color = 3;
    }
    if (color > 15)
    {
        color = 15;
    }

    rgb_color = (color + 16) << 1;
}

void sense_hat_front_panel_io(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void))
{
    if (!sense_hat_initialized)
    {
        return;
    }

    uint8_to_uint16_t(status, panel_buffer);
    uint8_to_uint16_t(data, panel_buffer + (3 * 8));

    uint8_to_uint16_t((uint8_t)(bus >> 8), panel_buffer + (6 * 8));
    uint8_to_uint16_t((uint8_t)(bus), panel_buffer + (7 * 8));

    pi_sense_8x8_panel_update(panel_buffer, PI_SENSE_8x8_BUFFER_SIZE);
}

static void panel_draw_bitmap(void)
{
    memset(panel_8x8_buffer, 0x00, sizeof(panel_8x8_buffer));
    gfx_rotate_counterclockwise(pixel_map.bitmap, 1, 1, bitmap_rows);
    gfx_reverse_panel(bitmap_rows);
    gfx_rotate_counterclockwise(bitmap_rows, 1, 1, bitmap_rows);
    gfx_bitmap_to_rgb(bitmap_rows, panel_8x8_buffer, sizeof(panel_8x8_buffer));
    pi_sense_8x8_panel_update(panel_8x8_buffer, sizeof(panel_8x8_buffer));
}

bool sense_hat_handle_led_matrix_output(int port_number, uint8_t data, char *buffer, size_t buffer_length, size_t *handled_length)
{
    if (!sense_hat_initialized)
    {
        if (handled_length)
        {
            *handled_length = 0;
        }
        return false;
    }

    (void)buffer;
    (void)buffer_length;

    bool handled = false;

    switch (port_number)
    {
        case 65: // Set brightness of the 8x8 LED Panel
            sense_hat_set_led_panel_color(data);
            handled = true;
            break;
        case 80: // panel_mode 0 = bus data, 1 = font, 2 = bitmap
            if (data < 3)
            {
                panel_mode = data;
            }
            handled = true;
            break;
        case 81: // set font color
            gfx_set_color(data);
            handled = true;
            break;
        case 85: // display character
            memset(panel_8x8_buffer, 0x00, sizeof(panel_8x8_buffer));
            gfx_load_character(data, bitmap_rows);
            gfx_rotate_counterclockwise(bitmap_rows, 1, 1, bitmap_rows);
            gfx_reverse_panel(bitmap_rows);
            gfx_rotate_counterclockwise(bitmap_rows, 1, 1, bitmap_rows);
            gfx_bitmap_to_rgb(bitmap_rows, panel_8x8_buffer, sizeof(panel_8x8_buffer));
            pi_sense_8x8_panel_update(panel_8x8_buffer, sizeof(panel_8x8_buffer));
            handled = true;
            break;
        case 90: // Bitmap row 0
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 97:
            pixel_map.bitmap[port_number - 90] = data;
            handled = true;
            break;
        case 98: // Pixel on
            if (data < 64)
            {
                pixel_map.bitmap64 |= (uint64_t)1 << data;
            }
            handled = true;
            break;
        case 99: // Pixel off
            if (data < 64)
            {
                pixel_map.bitmap64 &= ~((uint64_t)1 << data);
            }
            handled = true;
            break;
        case 100: // Pixel flip
            if (data < 64)
            {
                pixel_map.bitmap64 ^= (uint64_t)1 << data;
            }
            handled = true;
            break;
        case 101: // clear all pixels
            pixel_map.bitmap64 = 0;
            handled            = true;
            break;
        case 102: // Bitmap draw
            panel_draw_bitmap();
            handled = true;
            break;
        default:
            break;
    }

    if (handled_length)
    {
        *handled_length = 0;
    }

    return handled;
}
