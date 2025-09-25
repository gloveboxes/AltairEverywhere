#!/bin/bash

cd /AltairEverywhere/Terminal 

python3 -m http.server 80 &

cd /AltairEverywhere/src

# Build the command line arguments
ARGS=""

# MQTT Configuration (optional)
if [ ! -z "$MQTT_HOST" ]; then
    ARGS="$ARGS -m $MQTT_HOST"
fi

if [ ! -z "$MQTT_PORT" ]; then
    ARGS="$ARGS -p $MQTT_PORT"
fi

if [ ! -z "$MQTT_CLIENT_ID" ]; then
    ARGS="$ARGS -c $MQTT_CLIENT_ID"
fi

if [ ! -z "$MQTT_USERNAME" ]; then
    ARGS="$ARGS -U $MQTT_USERNAME"
fi

if [ ! -z "$MQTT_PASSWORD" ]; then
    ARGS="$ARGS -P $MQTT_PASSWORD"
fi

# Network Configuration
if [ ! -z "$NETWORK_INTERFACE" ]; then
    ARGS="$ARGS -n $NETWORK_INTERFACE"
else
    # Default to eth0 if not specified
    ARGS="$ARGS -n eth0"
fi

# External Services
if [ ! -z "$OPEN_WEATHER_MAP_API_KEY" ]; then
    ARGS="$ARGS -o $OPEN_WEATHER_MAP_API_KEY"
fi

if [ ! -z "$COPYX_URL" ]; then
    ARGS="$ARGS -u $COPYX_URL"
fi

if [ ! -z "$OPENAI_API_KEY" ]; then
    ARGS="$ARGS -a $OPENAI_API_KEY"
fi

./build/Altair_emulator $ARGS &

wait -n

exit $?