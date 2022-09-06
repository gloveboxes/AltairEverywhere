#include <gtest/gtest.h>
#include <sense_hat.h>

TEST(test_sense, test_init)
{
    if (init(1) == 0)
	{
		FAIL();
	}
    stop();
}

TEST(test_sense, test_temperature)
{
    if (init(1) == 0)
	{
		FAIL();
	}
    float temperature = get_temperature();
    ASSERT_GT(temperature, -70.0);
    ASSERT_LT(temperature, 100.0);
    stop();
}

TEST(test_sense, test_humidity)
{
    if (init(1) == 0)
	{
		FAIL();
	}
    float humidity = get_humidity();
    ASSERT_GT(humidity, 0.0);
    ASSERT_LT(humidity, 100.0);
    stop();
}

TEST(test_sense, test_pressure)
{
    if (init(1) == 0)
	{
		FAIL();
	}
    float pressure = get_pressure();
    ASSERT_GT(pressure, 900);
    ASSERT_LT(pressure, 1200);
    stop();
}

TEST(test_sense, test_temperature_lps25)
{
    if (init(1) == 0)
	{
		FAIL();
	}
    float temperature = get_temperature_from_lps25h();
    ASSERT_GT(temperature, -70.0);
    ASSERT_LT(temperature, 70.0);
    stop();
}