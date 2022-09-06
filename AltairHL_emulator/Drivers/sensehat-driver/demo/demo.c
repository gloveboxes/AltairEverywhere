#include <sense_hat.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	if (init(1) == 0)
	{
		printf("Unable to initialize sense_hat");
		return -1;
	}
	for(int i=0; i<10; i++)
	{
		printf("HTS221 Temperature: %.2f\n", get_temperature());
		printf("HTS221 Humidity: %.2f\n", get_humidity());
		printf("LPS25H Pressure: %d\n", get_pressure());
		printf("LPS25H Temperature: %.2f\n", get_temperature_from_lps25h());
		printf("-------------------------\n");
		sleep(1);
	}
	stop();
	return 0;
}