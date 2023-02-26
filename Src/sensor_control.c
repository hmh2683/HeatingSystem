#include "sensor_control.h"
#include "ds18b20.h"

Ds18b20Sensor_t Sensor;
extern OneWire_t OneWire;
extern uint8_t OneWireDevices;

static char init_state = 0;
static char convert_state = 0;

/*
 * Init 함수에서 OneWire 구조체 변수를 초기화시키며 GPIO 핀설정을 완료하고, 온도센서의 ROM 주소를 OneWire 주소 리스트에 대입한다.
 * GetFullROM 함수에서 OneWire 주소리스트를 온도센서 주소에 대입한다.
 * SetResolution 함수에서 12bit 만큼 온도 정밀도를 표현한다.
 * DS18B20_DisableAlarmTemperature 함수에서 알람기능도 지원한다.
 */
void Sensor_Init(void)
{
	init_state = 0;

	OneWire_Init(&OneWire,_DS18B20_GPIO ,_DS18B20_PIN);
	OneWire.ROM_NO[0] = 0x28;
	OneWire.ROM_NO[1] = 0x78;
	OneWire.ROM_NO[2] = 0x89;
	OneWire.ROM_NO[3] = 0x95;
	OneWire.ROM_NO[4] = 0xf0;
	OneWire.ROM_NO[5] = 0x01;
	OneWire.ROM_NO[6] = 0x3c;
	OneWire.ROM_NO[7] = 0x2b;
	OneWire_GetFullROM(&OneWire, Sensor.Address);

	Ds18b20Delay(50);
	DS18B20_SetResolution(&OneWire, Sensor.Address, DS18B20_Resolution_12bits);
	Ds18b20Delay(50);
	DS18B20_DisableAlarmTemperature(&OneWire, Sensor.Address);
	init_state = 1;
}

/*
 * StartAll 함수에서 모든 Slave 장치에게 동시에 명령어를 보낼 수 있도록 하고 온도변환 시작을 알린다.
 * AllDone 함수에서 진행은 (0)을 완료는 (1)을 반환한다.
 */
void StartConverting()
{
	DS18B20_StartAll(&OneWire);
	convert_state = !DS18B20_AllDone(&OneWire);
	convert_state = 1;
}

/*
 * AllDone 함수에서 완료(1)가 될시, 온도정보를 받는다.
 */
void CheckConverting()
{
	convert_state = !DS18B20_AllDone(&OneWire);
}

/*
 * DS18B20_Read 함수에서 ROM 주소를 선택하고 스크랫치 패드에서 명령을 읽는다.
 * CRC 함수에서 오류를 검출하며 성공 여부를 반환하고, 해당 센서의 온도를 반환한다.
 */
float GetTemp()
{
	Ds18b20Delay(100);
	Sensor.DataIsValid = DS18B20_Read(&OneWire, Sensor.Address, &Sensor.Temperature);

	return Sensor.Temperature;
}

float GetCurrentTemp()
{
	return Sensor.Temperature;
}

char GetSensorInitState()
{
	return init_state;
}

char GetConvertState()
{
	return convert_state;
}
