#!/bin/bash

mosquitto -c /etc/mosquitto/mosquitto.conf &

cd /Altair8800/Terminal && python3 -m http.server 8081 &

cd /Altair8800/AltairHL_emulator

./build/Altair_emulator -s $ID_SCOPE -d $DEVICE_ID -k $DERIVED_KEY -n "eth0" -o $OPEN_WEATHER_MAP_API_KEY -a $AIR_VISUAL_API_KEY &

wait -n

exit $?