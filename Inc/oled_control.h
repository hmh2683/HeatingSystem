#ifndef INC_OLED_CONTROL_H_
#define INC_OLED_CONTROL_H_

#include "global.h"

void StartScreen(void);
void DefaultScreen(void);
void TempScreen(int temp);
void WorkScreen(ON_OFF_t state);
void SetScreen(int temp, ON_OFF_t state);



#endif
