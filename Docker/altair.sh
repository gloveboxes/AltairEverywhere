#!/bin/bash

cd /AltairEverywhere/Terminal 

python3 -m http.server 80 &

cd /AltairEverywhere/AltairHL_emulator

./build/Altair_emulator -s $ID_SCOPE -d $DEVICE_ID -k $DERIVED_KEY -o $OPEN_WEATHER_MAP_API_KEY -u $COPYX_URL -n eth0 &

wait -n

exit $?