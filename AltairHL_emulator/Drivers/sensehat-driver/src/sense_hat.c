#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define HTS221_ADDRESS 	0x5F
#define LPS25H_ADDRESS  0x5C

void init_HTS221();
void init_LPS25H();
static int i2c_read(int file, unsigned char addr, unsigned char *buf, int len);
static int i2c_write(int file, unsigned char addr, unsigned char *buf, int len);

// Struct that contains the file descriptors for i2c access
struct i2c_files
{
    int HTS221;
    int LPS25H;
};

static struct i2c_files fds = {-1, -1};

unsigned char calibrate[32];

// humidity/temp calibration values
static int H0_rH_x2, H1_rH_x2, T0_degC_x8;
static int T1_degC_x8, H0_T0_OUT;
static int H1_T0_OUT, T0_OUT, T1_OUT;

int pi_sense_hat_sensors_init(int i2c_num)
{
    char filename[32];
    sprintf(filename, "/dev/i2c-%d", i2c_num);

    fds.HTS221 = open(filename, O_RDWR);
    if(ioctl(fds.HTS221, I2C_SLAVE, HTS221_ADDRESS) < 0)
    {
        fprintf(stderr, "Failed to acquire bus for HTS221 temperature and humidty sensor\n");
        return 0;
    }
    else
    {
        init_HTS221();
    }

    fds.LPS25H = open(filename, O_RDWR);
    if(ioctl(fds.LPS25H, I2C_SLAVE, LPS25H_ADDRESS) < 0)
    {
        fprintf(stderr, "Failed to acquire bus for LP25H pressure sensor\n");
        return 0;
    }
    else
    {
        init_LPS25H();
    }
    return 1;
}

float get_temperature()
{
    unsigned char buf[4];
    int rc;
    int H_T_out, T_out, T0_degC, T1_degC;
    int tmp;

    rc = i2c_read(fds.HTS221, 0x28+0x80, buf, 4);
    if (rc == 4)
    {
        H_T_out = buf[0] + (buf[1] << 8);
        T_out = buf[2] + (buf[3] << 8);
        if (H_T_out > 32767) H_T_out -=65536;
        if (T_out > 32767) T_out -= 65536;
        T0_degC = T0_degC_x8 / 8;
        T1_degC = T1_degC_x8 / 8;
        tmp = (T_out - T0_OUT) * (T1_degC - T0_degC)*10;
        return (tmp / (T1_OUT - T0_OUT) + T0_degC*10)/10.0f;
    }
    return INT16_MIN;
}

float get_humidity()
{
    unsigned char buf[4];
    int rc;
    int H_T_out, T_out;
    int H0_rh, H1_rh;
    int tmp;

    rc = i2c_read(fds.HTS221, 0x28+0x80, buf, 4);
    if (rc == 4)
    {
        H_T_out = buf[0] + (buf[1] << 8);
        T_out = buf[2] + (buf[3] << 8);
        if (H_T_out > 32767) H_T_out -=65536;
        if (T_out > 32767) T_out -= 65536;
        H0_rh = H0_rH_x2 / 2;
        H1_rh = H1_rH_x2 / 2;
        tmp = (H_T_out - H0_T0_OUT) * (H1_rh - H0_rh)*10;
        return (tmp / (H1_T0_OUT - H0_T0_OUT) + H0_rh*10)/10.0f;
    }
    return INT16_MIN;
}

int get_pressure()
{
    unsigned char buf[8];
    int rc, P;

    if(fds.LPS25H != -1)
    {
        rc = i2c_read(fds.LPS25H, 0x28+0x80, buf, 5);
        if (rc == 5)
        {
            P = buf[0] + (buf[1]<<8) + (buf[2]<<16); 
            return P / 4096;
        }
        return 0;	
    }
    return -1;
}

float get_temperature_from_lps25h()
{
    unsigned char buf[8];
    int rc, T;

    if (fds.LPS25H != -1)
    {
        rc = i2c_read(fds.LPS25H, 0x28+0x80, buf, 5);
        if (rc == 5)
        {
            T = buf[3] + (buf[4] << 8);
            if (T > 32767) T -= 65536;
            T = 425 + (T / 48); // 42.5 + T value/480
            return (T)/10.0f;
        }
        return 1;	
    }
    return 0;
}

void init_HTS221()
{   
    // code to initialize and calibrate HTS221
    // Initialization
    i2c_read(fds.HTS221, 0x10, calibrate, 1);
    calibrate[0] &= 0xc0;
    calibrate[0] |= 0x1b; // average temperature = 16, humidity = 32
    i2c_write(fds.HTS221, 0x10, calibrate, 1);

    i2c_read(fds.HTS221, 0x20+0x80, calibrate, 3); // read CTRL_REG 1 to 3
    calibrate[0] &= 0x78; // keep reserved bits
    calibrate[0] |= 0x81; // bit to turn on and set the sample rate to 1hz
    calibrate[1] &= 0x7c; // bit to  turn off heater, boot, one shot
    i2c_write(fds.HTS221, 0x20+0x80, calibrate, 3);

    // calibrate humidity and temperature
    i2c_read(fds.HTS221, 0x30+0x80, calibrate, 16);
    H0_rH_x2 = calibrate[0];
    H1_rH_x2 = calibrate[1];
    T0_degC_x8 = calibrate[2];
    T1_degC_x8 = calibrate[3];
    T0_degC_x8 |= ((calibrate[5] & 0x3) << 8);
    T1_degC_x8 |= ((calibrate[5] & 0xc) << 6);
    H0_T0_OUT = calibrate[6] | (calibrate[7] << 8);
    H1_T0_OUT = calibrate[10] | (calibrate[11] << 8);
    T0_OUT = calibrate[12] | (calibrate[13] << 8);
    T1_OUT = calibrate[14] | (calibrate[15] << 8);
    if (H0_T0_OUT > 32767) H0_T0_OUT -= 65536;
    if (H1_T0_OUT > 32767) H1_T0_OUT -= 65536;
    if (T0_OUT > 32767) T0_OUT -= 65536;
    if (T1_OUT > 32767) T1_OUT -= 65536;
}

void init_LPS25H()
{   
    // code to intialize LP15H goes here
    calibrate[0] = 0x90; // bit to turn on and set the sample rate to 1hz
    i2c_write(fds.LPS25H, 0x20, calibrate, 1);
}

static int i2c_read(int file, unsigned char addr, unsigned char *buf, int len)
{
    int rc;
    rc = write(file, &addr, 1);
    if (rc == 1)
    {
        rc = read(file, buf, len);
    }
    return rc;
}

static int i2c_write(int file, unsigned char addr, unsigned char *buf, int len)
{
    unsigned char tmp[512];
    int rc;
    if (len > 511 || len < 1 || buf == NULL)
        return -1;

    tmp[0] = addr;
    memcpy(&tmp[1], buf, len);
    rc = write(file, tmp, len+1);
    return rc-1;
}

void pi_sense_hat_sensors_close(void)
{
    // Close all I2C file handles
    if (fds.HTS221 != -1) close(fds.HTS221);
    if (fds.LPS25H != -1) close(fds.LPS25H);
}