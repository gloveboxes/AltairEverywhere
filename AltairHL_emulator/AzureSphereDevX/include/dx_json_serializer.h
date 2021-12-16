#pragma once

#include "parson.h"
#include "stdarg.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"

typedef enum {
	DX_JSON_BOOL,
	DX_JSON_STRING,
    DX_JSON_INT,
    DX_JSON_FLOAT,
    DX_JSON_DOUBLE
} DX_JSON_TYPE;

/// <summary>
/// JSON Serializer. Pass in a variable number of JSON Key Value Pairs
/// </summary>
/// <param name="buffer">Buffer for JSON string result</param>
/// <param name="buffer_size">Size of JSON string</param>
/// <param name="key_value_pair_count">The number of Key Value Pairs to serialize as JSON</param>
/// <param name="">
/// Data to be serialised must be passed in groups of three (JSON type, key name, key value). The value passed must match the type. 
/// Examples: DX_JSON_DOUBLE, "Temperature", temperature, DX_JSON_INT, "Humidity", humidity, DX_JSON_STRING, "Status", "cooling"
/// </param>
/// <returns></returns>
bool dx_jsonSerialize(char* buffer, size_t buffer_size, int key_value_pair_count, ...);