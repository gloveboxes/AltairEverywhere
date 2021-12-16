//GROVE_NAME        "Grove - Blue LED Button"
//SKU               111020046
//WIKI_URL          http://wiki.seeedstudio.com/Grove-LED_Button/

#pragma once

void *GroveLEDButton_Init(GPIO_Id btn, GPIO_Id led);
GPIO_Value_Type GroveLEDButton_GetBtnState(void *inst);
void GroveLEDButton_LedOn(void *inst);
void GroveLEDButton_LedOff(void *inst);