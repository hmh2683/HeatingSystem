#ifndef INC_SENSOR_CONTROL_H_
#define INC_SENSOR_CONTROL_H_

void Sensor_Init(void);
void StartConverting(void);
void CheckConverting(void);

float GetTemp(void);
float GetCurrentTemp(void);

char GetSensorInitState(void);
char GetConvertState(void);



#endif
