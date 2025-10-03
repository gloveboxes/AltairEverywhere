#include "front_panel_manager.h"

#include "front_panel_none.h"
#include "dx_utilities.h"

// Linux-specific front panels
#if defined(__linux__)
#include "front_panel_kit.h"
#include "front_panel_pi_sense_hat.h"
#endif

typedef struct
{
    FRONT_PANEL_TYPE type;
    const char *name;
    bool (*init)(void);
    void (*shutdown)(void);
    void (*io)(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void));
    void (*set_led_color)(int color);
    bool (*handle_led_matrix_output)(int port_number, uint8_t data, char *buffer, size_t buffer_length, size_t *handled_length);
} FRONT_PANEL_DRIVER;

static const FRONT_PANEL_DRIVER none_driver = {
    .type                       = FRONT_PANEL_TYPE_NONE,
    .name                       = "None",
    .init                       = none_front_panel_init,
    .shutdown                   = none_front_panel_shutdown,
    .io                         = none_front_panel_io,
    .set_led_color              = none_front_panel_set_led_color,
    .handle_led_matrix_output   = none_front_panel_handle_led_matrix_output,
};

#if defined(__linux__)
static const FRONT_PANEL_DRIVER sense_hat_driver = {
    .type                       = FRONT_PANEL_TYPE_SENSE_HAT,
    .name                       = "Raspberry Pi Sense HAT",
    .init                       = sense_hat_front_panel_init,
    .shutdown                   = sense_hat_front_panel_shutdown,
    .io                         = sense_hat_front_panel_io,
    .set_led_color              = sense_hat_set_led_panel_color,
    .handle_led_matrix_output   = sense_hat_handle_led_matrix_output,
};

static const FRONT_PANEL_DRIVER kit_driver = {
    .type                       = FRONT_PANEL_TYPE_KIT,
    .name                       = "Front Panel Kit",
    .init                       = kit_front_panel_init,
    .shutdown                   = kit_front_panel_shutdown,
    .io                         = kit_front_panel_io,
    .set_led_color              = kit_front_panel_set_brightness,
    .handle_led_matrix_output   = kit_front_panel_handle_led_matrix_output,
};
#endif

static const FRONT_PANEL_DRIVER *active_driver = &none_driver;

static void log_panel_initialization(const FRONT_PANEL_DRIVER *driver, bool success)
{
    dx_Log_Debug("Front panel %s: %s\n", driver->name, success ? "initialized" : "not available");
}

static const FRONT_PANEL_DRIVER *get_driver_by_type(FRONT_PANEL_TYPE type)
{
    switch (type)
    {
#if defined(__linux__)
        case FRONT_PANEL_TYPE_SENSE_HAT:
            return &sense_hat_driver;
        case FRONT_PANEL_TYPE_KIT:
            return &kit_driver;
#endif
        case FRONT_PANEL_TYPE_NONE:
        default:
            return &none_driver;
    }
}

bool front_panel_manager_init(FRONT_PANEL_SELECTION selection)
{
    const FRONT_PANEL_DRIVER *candidates[3];
    size_t candidate_count = 0;

    switch (selection)
    {
        case FRONT_PANEL_SELECTION_NONE:
            candidates[candidate_count++] = &none_driver;
            break;
#if defined(__linux__)
        case FRONT_PANEL_SELECTION_SENSE_HAT:
            candidates[candidate_count++] = &sense_hat_driver;
            candidates[candidate_count++] = &none_driver;
            break;
        case FRONT_PANEL_SELECTION_KIT:
            candidates[candidate_count++] = &kit_driver;
            candidates[candidate_count++] = &none_driver;
            break;
        default:
            candidates[candidate_count++] = &none_driver;
            break;
#else
        // On non-Linux platforms, only 'none' driver is available
        case FRONT_PANEL_SELECTION_SENSE_HAT:
        case FRONT_PANEL_SELECTION_KIT:
            dx_Log_Debug("Warning: Hardware front panels only supported on Linux. Using 'None' driver.\n");
            candidates[candidate_count++] = &none_driver;
            break;
        default:
            candidates[candidate_count++] = &none_driver;
            break;
#endif
    }

    for (size_t i = 0; i < candidate_count; ++i)
    {
        const FRONT_PANEL_DRIVER *driver = candidates[i];
        if (!driver->init)
        {
            continue;
        }

        if (driver->init())
        {
            active_driver = driver;
            log_panel_initialization(driver, true);
            return true;
        }

        log_panel_initialization(driver, false);
    }

    active_driver = &none_driver;
    active_driver->init();
    log_panel_initialization(active_driver, true);
    return true;
}

void front_panel_manager_shutdown(void)
{
    if (active_driver && active_driver->shutdown)
    {
        active_driver->shutdown();
    }

    active_driver = &none_driver;
    active_driver->init();
}

void front_panel_manager_io(uint8_t status, uint8_t data, uint16_t bus, void (*process_control_panel_commands)(void))
{
    if (active_driver && active_driver->io)
    {
        active_driver->io(status, data, bus, process_control_panel_commands);
    }
}

void front_panel_manager_set_led_color(int color)
{
    if (active_driver && active_driver->set_led_color)
    {
        active_driver->set_led_color(color);
    }
}

bool front_panel_manager_handle_led_matrix_output(int port_number, uint8_t data, char *buffer, size_t buffer_length, size_t *handled_length)
{
    if (active_driver && active_driver->handle_led_matrix_output)
    {
        return active_driver->handle_led_matrix_output(port_number, data, buffer, buffer_length, handled_length);
    }

    (void)port_number;
    (void)data;
    (void)buffer;
    (void)buffer_length;
    if (handled_length)
    {
        *handled_length = 0;
    }
    return false;
}

FRONT_PANEL_TYPE front_panel_manager_get_active_type(void)
{
    return active_driver ? active_driver->type : FRONT_PANEL_TYPE_NONE;
}

const char *front_panel_manager_get_active_name(void)
{
    return active_driver ? active_driver->name : none_driver.name;
}

bool front_panel_manager_is_active(FRONT_PANEL_TYPE type)
{
    return active_driver && active_driver->type == type;
}
