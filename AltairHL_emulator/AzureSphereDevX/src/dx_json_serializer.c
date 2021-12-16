#include "dx_json_serializer.h"

bool dx_jsonSerialize(char *buffer, size_t buffer_size, int key_value_pair_count, ...)
{
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    char *json_string = NULL;
    char *key = NULL;
    bool result = false;

    va_list valist;
    va_start(valist, key_value_pair_count);

    while (key_value_pair_count--) {
        DX_JSON_TYPE type = va_arg(valist, int);
        key = va_arg(valist, char *);

        switch (type) {
        case DX_JSON_INT:
            json_object_set_number(root_object, key, va_arg(valist, int));
            break;

            // floats are cast to doubles for valists
        case DX_JSON_FLOAT:
        case DX_JSON_DOUBLE:
            json_object_set_number(root_object, key, va_arg(valist, double));
            break;

        case DX_JSON_STRING:
            json_object_set_string(root_object, key, va_arg(valist, char *));
            break;

        case DX_JSON_BOOL:
            json_object_set_boolean(root_object, key, va_arg(valist, int));
            break;

        default:
            break;
        }
    }
    va_end(valist);

    json_string = json_serialize_to_string(root_value);

    if (strlen(json_string) < buffer_size) {
        strncpy(buffer, json_string, buffer_size);
        result = true;
    }

    json_free_serialized_string(json_string);
    json_value_free(root_value);

    return result;
}
