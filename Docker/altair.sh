#!/bin/bash

mosquitto -c /etc/mosquitto/mosquitto.conf &

cd /Altair8800.Emulator.UN-X/

cd Terminal && python3 -m http.server 8081 &

cd /Altair8800.Emulator.UN-X/AltairHL_emulator

./build/Altair_emulator "-s" "0ne000F25AF" "-d" "docker" "-k" "zXE/bDrlOp0YzlITcD8oLSCI8NS0UXVeND6fTUWSS/g=" -o "41cfa0029da8bd93cafdf1ee18aede4a" &


wait -n

exit $?