#include "env_air_visual.h"

static const char *air_visual_url_template = "http://api.airvisual.com/v2/nearest_city?lat=%.6f&lon=%.6f&key=%s";
static char air_visual_url[200];

static bool air_visual_initialized = false;

void update_air_visual(ENVIRONMENT_TELEMETRY *environment)
{
    JSON_Value *rootProperties = NULL;
    JSON_Object *current_properties, *weather_properties, *data_properties;
    POLLUTION_T *pollution = &environment->latest.pollution;

    if (!air_visual_initialized) {
        return;
    }

    char *data = dx_getHttpData(air_visual_url);

    if (data != NULL) {

        /*

        {
          "status": "success",
          "data": {
            "city": "Milsons Point",
            "state": "New South Wales",
            "country": "Australia",
            "location": {
              "type": "Point",
              "coordinates": [
                151.210579,
                -33.847175
              ]
            },
            "current": {
              "weather": {
                "ts": "2022-02-17T00:00:00.000Z",
                "tp": 27,
                "pr": 1008,
                "hu": 62,
                "ws": 1.34,
                "wd": 90,
                "ic": "01d"
              },
              "pollution": {
                "ts": "2022-02-16T13:00:00.000Z",
                "aqius": 15,
                "mainus": "p2",
                "aqicn": 5,
                "maincn": "p2"
              }
            }
          }
        }

        */
        if ((rootProperties = json_parse_string(data)) == NULL) {
            goto cleanup;
        }

        JSON_Object *rootObject = json_value_get_object(rootProperties);

        if (!json_object_has_value_of_type(rootObject, "status", JSONString)) {
            goto cleanup;
        }

        if (strcmp(json_object_get_string(rootObject, "status"), "success") != 0) {
            goto cleanup;
        }

        if ((data_properties = json_object_dotget_object(rootObject, "data")) == NULL) {
            goto cleanup;
        }

        if ((current_properties = json_object_dotget_object(data_properties, "current")) == NULL) {
            goto cleanup;
        }

        // Get weather data
        // if ((weather_properties = json_object_dotget_object(current_properties, "weather")) != NULL) {

        //     if (json_object_has_value_of_type(weather_properties, "tp", JSONNumber)) {
        //         weather->temperature = json_object_get_number(weather_properties, "tp");
        //     }

        //     if (json_object_has_value_of_type(weather_properties, "pr", JSONNumber)) {
        //         weather->pressure = json_object_get_number(weather_properties, "pr");
        //     }

        //     if (json_object_has_value_of_type(weather_properties, "hu", JSONNumber)) {
        //         weather->humidity = json_object_get_number(weather_properties, "hu");
        //     }

        //     if (json_object_has_value_of_type(weather_properties, "ws", JSONNumber)) {
        //         weather->wind_speed = (float)json_object_get_number(weather_properties, "ws");
        //     }

        //     if (json_object_has_value_of_type(weather_properties, "wd", JSONNumber)) {
        //         weather->wind_direction = json_object_get_number(weather_properties, "wd");
        //     }

        //     weather->updated = true;
        // }

        // Get pollution levels
        if ((data_properties = json_object_dotget_object(current_properties, "pollution")) != NULL) {

            if (json_object_has_value_of_type(data_properties, "aqius", JSONNumber)) {
                pollution->air_quality_index = json_object_get_number(data_properties, "aqius");
            }

            pollution->updated = true;
        }
    }

cleanup:

    if (rootProperties != NULL) {
        json_value_free(rootProperties);
        rootProperties = NULL;
    }

    if (data != NULL) {
        free(data);
        data = NULL;
    }

    return;
}

void init_air_visual_service(ALTAIR_CONFIG_T *altair_config, ENVIRONMENT_TELEMETRY *environment)
{
    if (!dx_isStringNullOrEmpty(altair_config->air_visual_api_key) && !air_visual_initialized && environment->locationInfo.updated) {

        snprintf(air_visual_url, sizeof(air_visual_url), air_visual_url_template, environment->locationInfo.lat, environment->locationInfo.lng, altair_config->air_visual_api_key);

        air_visual_initialized = true;
    }
}