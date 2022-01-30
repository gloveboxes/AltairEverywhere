#!/bin/bash

mosquitto -c /etc/mosquitto/mosquitto.conf &

cd /Altair8800Linux/AltairHL_emulator

./build/Altair_emulator "-s" "0ne000F25AF" "-d" "desktopdevx" "-k" "Mnk4T7wevKznmYcrjEGCHnlWSFDvv+/uUf6uIttPEmY=" 


wait -n

exit $?