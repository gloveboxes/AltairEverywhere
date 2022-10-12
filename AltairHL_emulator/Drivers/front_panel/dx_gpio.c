#include "dx_gpio.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

int GPIOWrite(int pin, int value);
int GPIODirection(int pin, int dir);
int GPIOUnexport(int pin);
int GPIOExport(int pin);



bool dx_gpioOpenOutput(int pin, bool state)
{
  if (!GPIOExport(pin))
  {
    nanosleep(&(struct timespec){0, 100 * 1000000}, NULL);
    if (!GPIODirection(pin, OUT))
    {
      if (dx_gpioStateSet(pin, state))
      {
        return true;
      }
    }
  }
  printf("GPIO Export failed for pin: %d\n", pin);
  return false;
}

bool dx_gpioClose(int pin)
{
  if (GPIOUnexport(pin))
  {
    printf("GPIO Unexport failed for pin: %d\n", pin);
    return false;
  }
  return true;
}

bool dx_gpioStateSet(int pin, bool state)
{
  return !GPIOWrite(pin, state ? 1 : 0);
}

int GPIOExport(int pin)
{
#define BUFFER_MAX 3
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open export for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

int GPIOUnexport(int pin)
{
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

int GPIODirection(int pin, int dir)
{
  static const char s_directions_str[] = "in\0out";

#define DIRECTION_MAX 35
  char path[DIRECTION_MAX];
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open gpio direction for writing!\n");
    return (-1);
  }

  if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3))
  {
    fprintf(stderr, "Failed to set direction!\n");
    return (-1);
  }

  close(fd);
  return (0);
}

static int
GPIORead(int pin)
{
#define VALUE_MAX 30
  char path[VALUE_MAX];
  char value_str[3];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_RDONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open gpio value for reading!\n");
    return (-1);
  }

  if (-1 == read(fd, value_str, 3))
  {
    fprintf(stderr, "Failed to read value!\n");
    return (-1);
  }

  close(fd);

  return (atoi(value_str));
}

int GPIOWrite(int pin, int value)
{
  static const char s_values_str[] = "01";

  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    return (-1);
  }

  if (1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1))
  {
    fprintf(stderr, "Failed to write value!\n");
    return (-1);
  }

  close(fd);
  return (0);
}