# Heating System  
Development period : 2022.03 ~ 2022.08 
Development environment : STM32CubeIDE 1.9.0
Development language : C
Development Goals 
  * STM32에서 센서의 온도 정보를 호출하여 FND 모듈에 현재 온도를 표시합니다.
  * 3개의 버튼과 OLED 모듈을 통해 설정 온도를 선택하고, OLED 모듈을 통해 릴레이의 상태 정보를 표시한다.
  * 설정온도와 현재온도를 비교하여 릴레이를 제어하고 설정온도를 유지 및 관리합니다.

<br/> <br/>

## Microcontroller
<a href="#"><img src="https://github.com/hmh2683/HeatingSystem/blob/main/image/mcu.png" width="350px" height="350px"></a> 
* Part : STM32F103C8T6
* Manufacturer : ST-Microelectronics
* Core : ARM Cortex-M3
* Clock Speed(MAX) : 72MHz
* Package : LQFP 48 pin

<br/> <br/>

## Pinout Configuration
<a href="#"><img src="https://github.com/hmh2683/HeatingSystem/blob/main/image/pinout.png" width="400px" height="400px"></a>

### System Core
* GPIO -> INPUT -> PB0_UP_BUTTON, PB1_SET_BUTTON, PB2_DOWN_BUTTON, PB12_START_SW
* GPIO -> OUTPUT -> PA3_TEMP_DATA, PB5_RELAY, PB6_BUTTON_LED, PB7_START_LED, PB14_FND_RCLK
* SPI -> PB13_FND_SCLK, PB15_FND_DIO
* I2C -> PB10_I2C2_SCL, PB11_I2C2_SDA
* SYS -> PA13_JTMS-SWDIO, PA14_SYS_JTCK-SWCLK
* RCC -> PC14_RCC_OSC32_IN, PC15_RCC_OSC32_OUT, PD0_RCC_OSC_IN, PD1_RCC_OSC_OUT

### Timers
* TIM2 -> TEMPSENSOR
  * Prescaler : 71
  * Counter Period : 65535
* TIM3 -> FND
  * Prescaler : 71
  * Counter Period : 99

### Connectivity
* SPI2 -> FND
  * Data Size : 8bit
  * First Bit : MSB
  * Prescaler : 16
  * Clock Polarity : High
  * Clock Phase : 1 Edge
* I2C2 -> OLED
  * Speed Mode : Fast Mode
  * Clock Speed : 400000 Hz
  * Fast Mode Duty Cycle : Duty cycle Tlow/Thigh = 2
* USART1 -> FTDI
  * Baud Rate : 115200 Bits/s
  * Word Length : 8 Bit
* ONEWIRE -> SENSOR

<br/> <br/>

## Clock Configuration
* 72 MHz의 클럭 주파수는 STM32 MCU에 내장된 수정 공진기를 사용하여 생성됩니다.
<a href="#"><img src="https://github.com/hmh2683/HeatingSystem/blob/main/image/clock.png" width="1000px" height="400px"></a> 

<br/> <br/>

## Code Review
### Main
* if 문은 중단된 버튼 변수에 따라 실행됩니다.
* LED는 시동 스위치의 상태에 따라 제어됩니다.
* 온도 변환을 시작하고 온도 값을 가져옵니다.
* 스위치를 켜면 온도값에 따라 릴레이가 동작하고, 스위치를 끄면 온도값에 상관없이 릴레이가 동작하지 않는다.
```C
while (1) {
	checkButton();
	checkStartSw();

	if (!isConverted()) 
		startConvert();
	
	checkConvert();
	if (!isConverted()) {
		temperature = getTemp();
		if (getStartSw() == ON_t) {
			relayControl(temperature);
		} else {
			if (getRelayState() == ON_t) 
				relayOnOff(OFF_t);
		}
	}
}
```

### Interrupt
* 인터럽트가 발생하면 HAL_GetTick 함수의 반환값과 CLICK_TIME 값을 비교하여 if 문을 실행합니다.
```C
void EXTI0_IRQHandler(void) {
  HAL_GPIO_EXTI_IRQHandler(PB0_TEMP_UP_BUTTON_Pin);

	if (HAL_GetTick() - before_time > CLICK_TIME) {
		button_up = 1;
	}
	before_time = HAL_GetTick();
}
```
* RCC 모드를 활성화하고 TIM3 prescaler: 72, period: 100을 설정하여 100us마다 인터럽트를 생성합니다.
* 초기화 상태 및 사용 상태를 확인하고 true이면 현재 온도 값을 가져와 FND로 표시합니다.
```C
void TIM3_IRQHandler(void) {
  HAL_TIM_IRQHandler(&htim3);
  
	if (isTempSensorInit() && !isBusy()) {
		digitTemp((int)(getCurrentTemp() * 10));
	}
}
```

### Communication 
#### 1. SPI  
* 소프트웨어에서 직접 SPI 기능을 생성합니다.
* MSB에서 시계를 1비트씩 제어합니다.
```C
void send(uint8_t X) {
	for (int i = 8; i >= 1; i--) {
		if (X & 0x80) {
			HAL_GPIO_WritePin(FND_DIO_GPIO_Port, FND_DIO_Pin, HIGH); 
		} else {
			HAL_GPIO_WritePin(FND_DIO_GPIO_Port, FND_DIO_Pin, LOW);
		}
		X <<= 1;
		HAL_GPIO_WritePin(FND_SCLK_GPIO_Port, FND_SCLK_Pin, LOW);
		HAL_GPIO_WritePin(FND_SCLK_GPIO_Port, FND_SCLK_Pin, HIGH);
	}
}
```
* STM32에서 제공하는 SPI 기능을 사용합니다.
```C
static SPI_HandleTypeDef *fhspi;

void send(uint8_t X) {
 	HAL_SPI_Transmit(fhspi, &X, 1, 100);
}
```
* RCLK를 LOW로 낮췄다가 다시 HIGH로 올려 16비트 정보를 전송합니다.
```C
void sendPort(uint8_t X, uint8_t port) {
	send(X);
	send(port);
	HAL_GPIO_WritePin(PB14_FND_RCLK_GPIO_Port, PB14_FND_RCLK_Pin, LOW);
	HAL_GPIO_WritePin(PB14_FND_RCLK_GPIO_Port, PB14_FND_RCLK_Pin, HIGH);
}
```

#### 2. I2C 
* 명령 테이블에 등록된 명령은 슬레이브 주소로 전송됩니다.
* OLED를 제어하기 위해 7비트 단위로 전송합니다.
```C
void ssd1306_I2C_Write(uint8_t address, uint8_t reg, uint8_t data) {
	uint8_t dt[2];
	dt[0] = reg;
	dt[1] = data;
	HAL_I2C_Master_Transmit(&hi2c2, address, dt, 2, 10);
}
```
* 슬레이브 주소에 1024비트 값을 등록합니다.
```C
void ssd1306_I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t *data, uint16_t count) {
	uint8_t dt[256];
	dt[0] = reg;
	uint8_t i;
	for (i = 0; i < count; i++)
		dt[i + 1] = data[i];
	HAL_I2C_Master_Transmit(&hi2c2, address, dt, count + 1, 10);
}
```

#### 3. UART
* STM32에서 제공하는 UART 핸들러와 전송 기능이 사용됩니다.
* printf 함수를 구현하는 데 사용됩니다.
```C
extern UART_HandleTypeDef *huart1;

int _write(int file, char *p, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t*) p, len, 10);
	return len;
}
```
#### 4. ONEWIRE
* 읽기 비트 값이 낮으면 초기화에 성공합니다.
```C
inline uint8_t OneWire_Reset(OneWire_t *OneWireStruct) {
	uint8_t i;

	ONEWIRE_LOW(OneWireStruct);
	ONEWIRE_OUTPUT(OneWireStruct);
	ONEWIRE_DELAY(480);
	ONEWIRE_DELAY(20);
	ONEWIRE_INPUT(OneWireStruct);
	ONEWIRE_DELAY(70);
	i = HAL_GPIO_ReadPin(OneWireStruct->GPIOx, OneWireStruct->GPIO_Pin);
	ONEWIRE_DELAY(410);
	
	return i;
}
```
* 슬레이브 장치로 명령 정보를 보냅니다.
```C
inline void OneWire_WriteBit(OneWire_t *OneWireStruct, uint8_t bit) {
	if (bit) {
		ONEWIRE_LOW(OneWireStruct);
		ONEWIRE_OUTPUT(OneWireStruct);
		ONEWIRE_DELAY(10);
		ONEWIRE_INPUT(OneWireStruct);	
		ONEWIRE_DELAY(55);
		ONEWIRE_INPUT(OneWireStruct);
	} else {
		ONEWIRE_LOW(OneWireStruct);
		ONEWIRE_OUTPUT(OneWireStruct);
		ONEWIRE_DELAY(65);
		ONEWIRE_INPUT(OneWireStruct);
		ONEWIRE_DELAY(5);
		ONEWIRE_INPUT(OneWireStruct);
	}
}
```
* 읽기 비트가 낮으면 장치가 아직 계산 온도로 완료되지 않은 것입니다.
```C
inline uint8_t OneWire_ReadBit(OneWire_t *OneWireStruct) {
	uint8_t bit = 0;

	ONEWIRE_LOW(OneWireStruct);
	ONEWIRE_OUTPUT(OneWireStruct);
	ONEWIRE_DELAY(2);
	ONEWIRE_INPUT(OneWireStruct);
	ONEWIRE_DELAY(10);	
	if (HAL_GPIO_ReadPin(OneWireStruct->GPIOx, OneWireStruct->GPIO_Pin)) {
		bit = 1;
	}
	ONEWIRE_DELAY(50);

	return bit;
}
```
* ROM 번호를 선택합니다.
```C
void OneWire_SelectWithPointer(OneWire_t *OneWireStruct, uint8_t *ROM) {
	uint8_t i;
	
	OneWire_WriteByte(OneWireStruct, ONEWIRE_CMD_MATCHROM);
	for (i = 0; i < 8; i++) {
		OneWire_WriteByte(OneWireStruct, *(ROM + i));
	}
}
```

<br/> <br/>

## Result
### Photo
<a href="#"><img src="https://github.com/hmh2683/HeatingSystem/blob/main/image/result.png" width="500px" height="400px"></a>
<a href="#"><img src="https://github.com/hmh2683/HeatingSystem/blob/main/image/uart.png" width="500px" height="400px"></a>

### Video
[![Heating System](http://img.youtube.com/vi/CozpDULafgo/0.jpg)](https://youtu.be/CozpDULafgo?t=0s) 

<br/> <br/>

## Realization
* STM32Cube의 자동 생성IDE는 핀이 설정된 것을 확인했고, 나는 인터럽트와 타이머 사용법을 마스터할 수 있었다.
* 학부 때 C언어 문법을 배웠지만 프로젝트에서 처음으로 문법과 기술을 사용하는 방법을 배우고 상호작용하는 방법과 도서관에 포함시키는 방법에 대해 조금 더 배울 수 있었다.
* SPI, I2C, UART, ONEWIRE 등 칩 간 통신에 대해 배울 수 있었고, 오픈소스를 이용해 소프트웨어 구현을 직접 수정하면서 통신 방식을 배우고 체험할 수 있었다.
