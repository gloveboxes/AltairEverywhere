#include <stdbool.h>
#include <time.h>

#include "as1115.h"
#include "graphics.h"

#include <applibs/log.h>

#include "hw/azure_sphere_learning_path.h"

as1115_t retro_click = { .interfaceId = ISU2, .handle = -1, .bitmap64 = 0, .keymap = 0, .debouncePeriodMilliseconds = 500 };
static char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";


void init_altair_hardware(void) {
	as1115_init(&retro_click, 1);
}

int main(void) {
	unsigned int letter = 0;
	uint8_t button_position = 0;

	init_altair_hardware();

	while (true) {

		letter++;
		letter = letter % (sizeof(letters) - 1);

		gfx_load_character(letters[letter], retro_click.bitmap);

		gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);
		gfx_reverse_panel(retro_click.bitmap);
		gfx_rotate_counterclockwise(retro_click.bitmap, 1, 1, retro_click.bitmap);

		as1115_panel_write(&retro_click);

		if ((button_position = as1115_get_btn_position(&retro_click)) != 0) {
			Log_Debug("Button %d pressed\n", button_position);
		}

		nanosleep(&(struct timespec) { 0, 150000000 }, NULL);
	}
}
