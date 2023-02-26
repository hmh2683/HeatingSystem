#ifndef INC_GLOBAL_H_
#define INC_GLOBAL_H_

#define DEFAULT_TEMP 30
#define CLICK_TIME 200
#define START_SW_CHECK_TIME 100
#define SELECTED_TEMP_RANGE 7

extern char button_up;
extern char button_down;
extern char button_set;

typedef enum{
	ON_t = 0,
	OFF_t = 1
} ON_OFF_t;

#endif
