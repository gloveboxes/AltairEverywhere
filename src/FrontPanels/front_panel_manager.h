#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    FRONT_PANEL_TYPE_NONE = 0,
    FRONT_PANEL_TYPE_SENSE_HAT,
    FRONT_PANEL_TYPE_KIT
} FRONT_PANEL_TYPE;

typedef enum
{
    FRONT_PANEL_SELECTION_NONE = 0,
    FRONT_PANEL_SELECTION_SENSE_HAT,
    FRONT_PANEL_SELECTION_KIT
} FRONT_PANEL_SELECTION;

bool front_panel_manager_init(FRONT_PANEL_SELECTION selection);
void front_panel_manager_shutdown(void);

void front_panel_manager_io(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void));
void front_panel_manager_set_led_color(int color);
bool front_panel_manager_handle_led_matrix_output(int port_number, uint8_t data, char *buffer, size_t buffer_length, size_t *handled_length);

FRONT_PANEL_TYPE front_panel_manager_get_active_type(void);
const char *front_panel_manager_get_active_name(void);
bool front_panel_manager_is_active(FRONT_PANEL_TYPE type);
