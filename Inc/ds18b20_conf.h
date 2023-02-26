#ifndef	DS18B20_CONF_H
#define	DS18B20_CONF_H

#include "stm32f1xx_hal.h"

extern TIM_HandleTypeDef htim2;

//	Init timer on cube    1us per tick				example 72 MHz cpu >>> Prescaler=(72-1)      counter period=Max
//###################################################################################
#define	_DS18B20_USE_FREERTOS		    				0
#define _DS18B20_MAX_SENSORS		    				1
#define	_DS18B20_GPIO									TEMP_DATA_GPIO_Port
#define	_DS18B20_PIN									TEMP_DATA_Pin

#define	_DS18B20_CONVERT_TIMEOUT_MS					5000
#if (_DS18B20_USE_FREERTOS==1)
#define	_DS18B20_UPDATE_INTERVAL_MS					10000					//  (((	if==0  >> Ds18b20_ManualConvert()  )))    ((( if>0  >>>> Auto refresh )))
#endif

#define	_DS18B20_TIMER											htim2
//###################################################################################

#endif
