#include "led_panel.h"

static const char *FILEPATH_FB_0 = "/dev/fb0";
static const char *FILEPATH_FB_1 = "/dev/fb1";
static const int NUM_WORDS = 64;

static int fbfd;
static uint16_t *map;
static uint16_t *p;
static struct fb_fix_screeninfo fix_info;

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

static void init_8x8_panel(void)
{
    if (!open_8x8_panel(FILEPATH_FB_0)) {
        if (!open_8x8_panel(FILEPATH_FB_1)) {
            exit(EXIT_FAILURE);
        }
    }

    /* map the led frame buffer device into memory */
    map = mmap(NULL, PI_SENSE_8x8_BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (map == MAP_FAILED) {
        close(fbfd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }

    /* set a pointer to the start of the memory area */
    p = map;

    /* clear the led matrix */
    memset(map, 0, PI_SENSE_8x8_BUFFER_SIZE);
}

void pi_sense_hat_init(void)
{
    init_8x8_panel();
}

bool pi_sense_8x8_panel_update(uint16_t *panel_buffer, size_t buffer_len)
{
    if (buffer_len != buffer_len) {
        return false;
    }

    memcpy(map, panel_buffer, PI_SENSE_8x8_BUFFER_SIZE);

    return true;
}