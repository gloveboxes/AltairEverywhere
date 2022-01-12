#include "pi_sense_hat.h"

static int i;
static int fbfd;
static uint16_t *map;
static uint16_t *p;
static struct fb_fix_screeninfo fix_info;
static uint16_t panel_buffer[NUM_WORDS];

void init_altair_hardware(void)
{
    /* open the led frame buffer device */
    fbfd = open(FILEPATH, O_RDWR);
    if (fbfd == -1) {
        perror("Error (call to 'open')");
        exit(EXIT_FAILURE);
    }

    /* read fixed screen info for the open device */
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &fix_info) == -1) {
        perror("Error (call to 'ioctl')");
        close(fbfd);
        exit(EXIT_FAILURE);
    }

    /* now check the correct device has been found */
    if (strcmp(fix_info.id, "RPi-Sense FB") != 0) {
        printf("%s\n", "Error: RPi-Sense FB not found");
        close(fbfd);
        exit(EXIT_FAILURE);
    }

    /* map the led frame buffer device into memory */
    map = mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (map == MAP_FAILED) {
        close(fbfd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }

    /* set a pointer to the start of the memory area */
    p = map;

    /* clear the led matrix */
    memset(map, 0, FILESIZE);
}

static uint16_t *uint8_to_uint16_t(uint8_t bitmap, uint16_t *buffer)
{
    uint16_t mask = 1;
    uint8_t bit_number = 8;
    uint8_t pixel_number = 0;

    while (pixel_number < 8) {
        buffer[pixel_number++] = bitmap & mask ? RGB565_RED : 0x0000;
        mask = (uint16_t)(mask << 1);
    }
}

void update_panel_status_leds(uint8_t status, uint8_t data, uint16_t bus)
{
    uint8_to_uint16_t(status, panel_buffer);
    uint8_to_uint16_t(data, panel_buffer + (3 * 8));

    uint8_to_uint16_t((uint8_t)(bus >> 8), panel_buffer + (6 * 8));
    uint8_to_uint16_t((uint8_t)(bus), panel_buffer + (7 * 8));

    memcpy(map, panel_buffer, FILESIZE);
}