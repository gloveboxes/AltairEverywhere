# Open Weather Map

Connecting the Open Weather Map cloud service is optional, free, and recommended so the Altair emulator can stream weather and pollution information to Azure IoT Central.

## Create an Open Weather Map API Key

1. From your web browser navigate to [https://openweathermap.org/api](https://openweathermap.org/api)
1. Sign in to Open Weather Map
1. Select **API** from the Open Weather Map main menu.
1. Subscribe to the Current Weather Data. Select the **Free** option, and select **Get API key**.
1. Repeat and subscribe to the Air Pollution API. Select the **Free** option, and select **Get API key**.
1. Select **API Keys** from the Open Weather Map site.
1. Copy the Open Weather Map API key somewhere safe as you will need this data when you start the Altair in cloud-connected mode.

## Open Weather Map air quality calculation

Open Weather Map air quality calculation is based on the Common Air Quality Index (CAQI). The Common Air Quality Index (CAQI) is an air quality index used in Europe since 2006.

![The image shows how Open Weather Map calculate air quality index](img/open_weather_map_air_quality_index.png)

For more information, refer to the [calculation of Air Quality index](https://en.wikipedia.org/wiki/Air_quality_index#CAQI).
