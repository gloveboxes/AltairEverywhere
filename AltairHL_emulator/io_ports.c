#include "io_ports.h"

static FILE *copyx_stream;
static bool copyx_enabled = false;
static char copyx_filename[15];
static char data_buffer[256];
static int copyx_index = 0;
static volatile bool delay_enabled = false;
static volatile bool publish_weather_pending = false;
static volatile bool publish_json_pending = false;
static int jitter = 0;

// clang-format off
DX_MESSAGE_PROPERTY *json_msg_properties[] = {
    &(DX_MESSAGE_PROPERTY){.key = "appid", .value = "altair"}, 
    &(DX_MESSAGE_PROPERTY){.key = "type", .value = "json"},
    &(DX_MESSAGE_PROPERTY){.key = "schema", .value = "1"}};

DX_MESSAGE_CONTENT_PROPERTIES json_content_properties = {
    .contentEncoding = "utf-8", 
    .contentType = "application/json"};
// clang-format off

DX_TIMER_HANDLER(port_out_weather_handler)
{
    if (environment.valid && azure_connected) {
        environment.latest.weather.temperature += jitter;
        publish_telemetry(&environment);
        environment.latest.weather.temperature -= jitter;        
    }
    publish_weather_pending = false;
}
DX_TIMER_HANDLER_END

DX_TIMER_HANDLER(port_out_json_handler)
{
    if (azure_connected) {
        dx_azurePublish(data_buffer, strlen(data_buffer), json_msg_properties, NELEMS(json_msg_properties), &json_content_properties);        
    }
    publish_json_pending = false;
}
DX_TIMER_HANDLER_END

/// <summary>
/// Intel 8080 OUT Port handler
/// </summary>
/// <param name="port"></param>
/// <param name="data"></param>
void io_port_out(uint8_t port, uint8_t data)
{    
    static char *data_ptr = NULL;
    static int data_index = 0;

    switch (port) {
    case 30:
        if (data > 0) {
            dx_timerOneShotSet(&tmr_mbasic_delay_expired, &(struct timespec){data, 0});
            delay_enabled = true;
        }
        break;
    case 31:
        if (!publish_json_pending) {
            if (data_index == 0) {
                memset((void *)data_buffer, 0x00, sizeof(data_buffer));
            }

            if (data != 0 && data_index < sizeof(data_buffer)) {
                data_buffer[data_index] = data;
                data_index++;
            }

            if (data == 0) {
                publish_json_pending = true;
                data_index = 0;
                dx_timerOneShotSet(&tmr_deferred_port_out_json, &(struct timespec){0, 1});                
            }
        }
        break;
    case 32:      
        if (!publish_weather_pending) {  
            publish_weather_pending = true;
            jitter = (int)data;
            dx_timerOneShotSet(&tmr_deferred_port_out_weather, &(struct timespec){0, 1});
        }
        break;
    case 33:
        if (copyx_index == 0){
            memset(copyx_filename, 0x00, sizeof(copyx_filename));
            copyx_enabled = false;
            if (copyx_stream != NULL){
                fclose(copyx_stream);
            }
        }

        if (data != 0 && copyx_index < sizeof(copyx_filename)) {
            copyx_filename[copyx_index] = data;
            copyx_index++;
        }

        if (data == 0) {
            copyx_enabled = true;
            copyx_index = 0;
        }
    default:
        break;
    }
}

DX_TIMER_HANDLER(mbasic_delay_expired_handler)
{
    delay_enabled = false;
}
DX_TIMER_HANDLER_END

/// <summary>
/// Intel 8080 IN Port handler
/// </summary>
/// <param name="port"></param>
/// <returns></returns>
uint8_t io_port_in(uint8_t port)
{
    static bool reading_data = false;
    static char data[40];
    static int readPtr = 0;
    uint8_t retVal = 0;
    int ch;
    uint64_t now;
    char filePathAndName[30];

    switch (port) {
    case 30: // Has delay expired
        retVal = (uint8_t)delay_enabled;
        break;
    case 33:
        if (!reading_data && copyx_enabled) {
            readPtr = 0;            
            snprintf(filePathAndName, sizeof(filePathAndName), "%s/%s", COPYX_FOLDER_NAME, copyx_filename);
            if ((copyx_stream = fopen(filePathAndName, "r")) == NULL){
                retVal = 0x00;
                reading_data = false;
            } else {
                reading_data = true;
            }
        }

        if (reading_data && copyx_enabled){
            if ((ch = fgetc(copyx_stream)) == EOF) {
                retVal = 0x00;
                fclose(copyx_stream);
                copyx_stream = NULL;
            } else {
                retVal = (uint8_t)ch;
            }
        }

        break;
    case 42: // Return current UTC
        if (!reading_data) {
            readPtr = 0;
            dx_getCurrentUtc(data, sizeof(data));
            reading_data = true;
        }

        retVal = data[readPtr++];
        break;
    case 43: // Return local time
        if (!reading_data) {
            readPtr = 0;
            dx_getLocalTime(data, sizeof(data));
            reading_data = true;
        }

        retVal = data[readPtr++];
        break;
    case 44: // Generate random number to seed mbasic randomize command
        LOAD_PORT_DATA(((rand() % 64000) - 32000), %d);
        break;
    case 45:
        if (environment.latest.weather.updated) {
            LOAD_PORT_DATA(environment.latest.weather.temperature, %d);
        }
        break;
    case 46:
        if (environment.latest.weather.updated) {
            LOAD_PORT_DATA(environment.latest.weather.pressure, %d);
        }
        break;
    case 47:
        if (environment.latest.weather.updated) {
            LOAD_PORT_DATA(environment.latest.weather.humidity, %d);
        }
        break;
    case 48:
        if (environment.latest.weather.updated) {
            LOAD_PORT_DATA_FROM_STRING(environment.latest.weather.description);
        }
        break;
    case 49:
        if (environment.locationInfo.updated) {
            LOAD_PORT_DATA(environment.locationInfo.lat, %.6f);
        }
        break;
    case 50:
        if (environment.locationInfo.updated) {
            LOAD_PORT_DATA(environment.locationInfo.lng, %.6f);
        }
        break;
    case 53:
        if (environment.latest.weather.updated) {
            LOAD_PORT_DATA(environment.latest.weather.wind_speed, %.2f);
        }
        break;
    case 54:
        if (environment.latest.weather.updated) {
            LOAD_PORT_DATA(environment.latest.weather.wind_direction, %d);
        }
        break;
    case 55:
        if (environment.locationInfo.updated) {
            LOAD_PORT_DATA_FROM_STRING(environment.locationInfo.country);
        }
        break;
    case 57:
        if (environment.locationInfo.updated) {
            LOAD_PORT_DATA_FROM_STRING(environment.locationInfo.city);
        }
        break;
    case 60:
        if (environment.latest.pollution.updated) {
            LOAD_PORT_DATA(environment.latest.pollution.air_quality_index, %.2f);
        }
        break;
    case 61:
        if (environment.latest.pollution.updated) {
            LOAD_PORT_DATA(environment.latest.pollution.carbon_monoxide, %.2f);
        }
        break;
    case 62:
        if (environment.latest.pollution.updated) {
            LOAD_PORT_DATA(environment.latest.pollution.nitrogen_monoxide, %.2f);
        }
        break;
    case 63:
        if (environment.latest.pollution.updated) {
            LOAD_PORT_DATA(environment.latest.pollution.nitrogen_dioxide, %.2f);
        }
        break;
    case 64:
        if (environment.latest.pollution.updated) {
            LOAD_PORT_DATA(environment.latest.pollution.ozone, %.2f);
        }
        break;
    case 65:
        if (environment.latest.pollution.updated) {
            LOAD_PORT_DATA(environment.latest.pollution.sulphur_dioxide, %.2f);
        }
        break;
    case 66:
        if (environment.latest.pollution.updated) {
            LOAD_PORT_DATA(environment.latest.pollution.ammonia, %.2f);
        }
        break;
    case 67:
        if (environment.latest.pollution.updated) {
            LOAD_PORT_DATA(environment.latest.pollution.pm2_5, %.2f);
        }
        break;
    case 68:
        if (environment.latest.pollution.updated) {
            LOAD_PORT_DATA(environment.latest.pollution.pm10, %.2f);
        }
        break;
    default:
        retVal = 0x00;
    }

    if (reading_data && retVal == 0x00) {
        reading_data = false;
    }

    return retVal;
}