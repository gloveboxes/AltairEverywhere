//GROVE_NAME        "Grove - Relay"
//SKU               103020005
//WIKI_URL          http://wiki.seeedstudio.com/Grove-Relay/

#pragma once

#include "../applibs_versions.h"
#include <applibs/gpio.h>

void* GroveRelay_Open(GPIO_Id pinId);
void GroveRelay_On(void* inst);
void GroveRelay_Off(void* inst);
