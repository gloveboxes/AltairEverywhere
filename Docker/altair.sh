#!/bin/bash

# cd /Altair8800/Terminal && python3 -m http.server 8081 &

cd /Altair8800/AltairHL_emulator

# ./build/Altair_emulator -s $ID_SCOPE -d $DEVICE_ID -k $DERIVED_KEY -n "eth0" -o $OPEN_WEATHER_MAP_API_KEY &

./build/Altair_emulator -s $ID_SCOPE -d $DEVICE_ID -k $DERIVED_KEY -o $OPEN_WEATHER_MAP_API_KEY -u $COPYX_URL -n eth0 &

wait -n

exit $?