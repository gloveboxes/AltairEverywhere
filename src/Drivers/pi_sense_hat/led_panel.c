#include "led_panel.h"

static const char *FILEPATH_FB_0 = "/dev/fb0";
static const char *FILEPATH_FB_1 = "/dev/fb1";
static const int NUM_WORDS = 64;

static int fbfd = -1;
static uint16_t *map;
static uint16_t *p;
static struct fb_fix_screeninfo fix_info;
static bool panel_initialized;

static bool open_8x8_panel(const char *filepath)
{
    bool result = false;

    /* open the led frame buffer device */
    fbfd = open(filepath, O_RDWR);
    if (fbfd == -1) {
        perror("Error (call to 'open')");
        return false;
    }

    /* read fixed screen info for the open device */
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fix_info) == -1) {
        perror("Error (call to 'ioctl')");
        close(fbfd);
        return false;
    }

    /* now check the correct device has been found */
    if (strcmp(fix_info.id, "RPi-Sense FB") != 0) {
        printf("%s\n", "Error: RPi-Sense FB not found");
        close(fbfd);
        return false;
    }

    return true;
}

static bool init_8x8_panel(void)
{
    if (!open_8x8_panel(FILEPATH_FB_0)) {
        if (!open_8x8_panel(FILEPATH_FB_1)) {
            return false;
        }
    }

    /* map the led frame buffer device into memory */
    map = mmap(NULL, PI_SENSE_8x8_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (map == MAP_FAILED) {
        close(fbfd);
        fbfd = -1;
        perror("Error mmapping the file");
        return false;
    }

    /* set a pointer to the start of the memory area */
    p = map;

    /* clear the led matrix */
    memset(map, 0, PI_SENSE_8x8_BUFFER_SIZE);

    return true;
}

bool pi_sense_hat_init(void)
{
    if (panel_initialized) {
        return true;
    }

    if (!init_8x8_panel()) {
        return false;
    }

    panel_initialized = true;
    return true;
}

bool pi_sense_8x8_panel_update(uint16_t *panel_buffer, size_t buffer_len)
{
    if (!panel_initialized) {
        return false;
    }

    if (buffer_len != PI_SENSE_8x8_BUFFER_SIZE) {
        return false;
    }

    memcpy(map, panel_buffer, PI_SENSE_8x8_BUFFER_SIZE);

    return true;
}

void pi_sense_hat_close(void)
{
    if (!panel_initialized) {
        return;
    }

    if (map && map != MAP_FAILED) {
        munmap(map, PI_SENSE_8x8_BUFFER_SIZE);
    }

    if (fbfd >= 0) {
        close(fbfd);
    }

    map              = NULL;
    p                = NULL;
    fbfd             = -1;
    panel_initialized = false;
}
