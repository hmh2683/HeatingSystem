# Heating System  
* 개발 기간 : 2022.03 ~ 2022.08 
* 개발 환경 : STM32CubeIDE 1.9.0
* 개발 언어 : C
* 개발 목표 
  * 센서에서 출력한 현재 온도를 FND에 표시합니다.
  * 버튼을 통해 설정온도를 선택하고, OLED에서 설정온도와 드라이기 동작상태를 표시합니다.
  * 설정온도와 현재온도를 비교하며 드라이기를 제어하고 설정온도를 유지 및 관리합니다.

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
* (GPIO) PB0:UP_BUTTON, PB1:SET_BUTTON, PB2:DOWN_BUTTON, PB12:START_SW
* (GPIO) PB5:RELAY, PB6:BUTTON_LED, PB7:START_LED 
* (GPIO) PB14:FND_RCLK, PA3:TEMP_DATA
* (SPI) PB13:FND_SCLK, PB15:FND_DIO
* (I2C) PB10:I2C2_SCL, PB11:I2C2_SDA
* (SYS) PA13:JTMS-SWDIO, PA14:SYS_JTCK-SWCLK
* (RCC) PC14:RCC_OSC32_IN, PC13:RCC_OSC32_OUT, PD0:RCC_OSC_IN, PD1:RCC_OSC_OUT

### Timers
* TIM2(Sensor)
  * Prescaler : 71
  * Counter Period : 65535
* TIM3(FND)
  * Prescaler : 71
  * Counter Period : 99

### Connectivity
* SPI2(FND)
  * Data Size : 8bit
  * First Bit : MSB
  * Prescaler : 16
  * Clock Polarity : High
  * Clock Phase : 1 Edge
* I2C2(OLED)
  * Speed Mode : Fast Mode
  * Clock Speed : 400000 Hz
  * Fast Mode Duty Cycle : Duty cycle Tlow/Thigh = 2
* USART1(FTDI)
  * Baud Rate : 115200 Bits/s
  * Word Length : 8 Bit
* ONEWIRE(Sensor)

<br/> <br/>

## Clock Configuration
* STM32 MCU의 수정 발진기를 사용하여 72 MHz의 클럭 주파수를 생성합니다.
<a href="#"><img src="https://github.com/hmh2683/HeatingSystem/blob/main/image/clock.png" width="1000px" height="400px"></a> 

<br/> <br/>

## Code Review
### Main
* SelectButton 함수에서 버튼 입력에 대한 OLED, LED, USART 를 제어합니다. 
* 온도 변환 상태를 확인하고 현재온도를 반환합니다. 
* 스위치가 ON 이라면, Relay는 GetTemp 함수의 반환 값과 SelectButton 함수 내 전역 변수값을 비교하며 작동합니다. 
* 스위치가 OFF 라면, Relay는 값에 상관없이 작동하지 않습니다.
```C
float temperature = 0.0;
while (1)
{
	SelectButton();

	if (!GetConvertState())
		StartConverting();
	CheckConverting();

	if (!GetConvertState()) // 온도변환 완료 시
	{
		temperature = GetTemp();
		CheckSwitch((int)temperature);
	}
}
```

<br/>

### Interrupt
* 다수의 입력을 방지하기 위해 HAL_GetTick 함수를 사용하고 CLICK_TIME을 200으로 설정하며 0.2s 마다 실행합니다. (1 Tick = 1ms)
```C
void EXTI0_IRQHandler(void) 
{
  HAL_GPIO_EXTI_IRQHandler(PB0_TEMP_UP_BUTTON_Pin);

	if (HAL_GetTick() - before_time > CLICK_TIME) 
		button_up = 1;
	
	before_time = HAL_GetTick();
}
```

<br/>

* TIM3 prescaler: 72, period: 100으로 설정하여 100us 마다 Timer를 실행합니다. ((72 / 72M) * 100 = 100u)
* 센서 초기화 상태와 OneWire 실행 상태를 확인하고 참이라면 현재온도를 FND에 표시합니다.
```C
void TIM3_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim3);
  
  if (GetSensorInitState() && !GetBusy())
	DisplayTemp((int)GetCurrentTemp() * 10);
}
```

<br/>

### Communication 
#### 1. SPI  
* GPIO를 통해 2개의 클럭을 생성합니다. (DIO, SCLK)  
* MSB 방식으로 X 값을 1 bit씩 8회 출력합니다. (1bite = 8bit)
```C
void send(uint8_t X) 
{
	for (int i = 8; i >= 1; i--) 
	{
		if (X & 0x80) 
			HAL_GPIO_WritePin(FND_DIO_GPIO_Port, FND_DIO_Pin, HIGH); 
		else 
			HAL_GPIO_WritePin(FND_DIO_GPIO_Port, FND_DIO_Pin, LOW);
			
		X <<= 1;
		HAL_GPIO_WritePin(FND_SCLK_GPIO_Port, FND_SCLK_Pin, LOW);
		HAL_GPIO_WritePin(FND_SCLK_GPIO_Port, FND_SCLK_Pin, HIGH);
	}
}
```

<br/>

* STM32에서 제공하는 SPI 기능을 사용합니다. (SPI -> DIO, SCLK), (GPIO -> RCLK)
```C
static SPI_HandleTypeDef *fhspi;

void send(uint8_t X) 
{
 	HAL_SPI_Transmit(fhspi, &X, 1, 100);
}
```

<br/>

* 16bit 정보를 전송하고 RCLK을 LOW로 출력하고 HIGH로 출력합니다.
```C
void send_port(uint8_t X, uint8_t port) 
{
	send(X);
	send(port);	
	HAL_GPIO_WritePin(FND_RCLK_GPIO_Port, FND_RCLK_Pin, LOW);
	HAL_GPIO_WritePin(FND_RCLK_GPIO_Port, FND_RCLK_Pin, HIGH);
}
```

<br/>

#### 2. I2C 
* 명령 테이블에 등록된 16 bit 정보를 슬레이브 주소로 전송합니다. (8 + 8 = 16)
```C
void ssd1306_I2C_Write(uint8_t address, uint8_t reg, uint8_t data) 
{
	uint8_t dt[2];
	dt[0] = reg;
	dt[1] = data;
	HAL_I2C_Master_Transmit(&hi2c2, address, dt, 2, 10);
}
```

<br/>

* 슬레이브 주소에 1,024 bit 정보를 전송합니다. (128 * 8 = 1,024)
* SSD1306_UpdateScreen 함수에서 해당 함수를 8회 호출하여 8,192 bit 정보를 전송합니다. (128 * 64 = 8,192)
```C
void ssd1306_I2C_WriteMulti(uint8_t address, uint8_t reg, uint8_t *data, uint16_t count) 
{
	uint8_t dt[256];
	dt[0] = reg;
	uint8_t i;
	for (i = 0; i < count; i++)
		dt[i + 1] = data[i];
	HAL_I2C_Master_Transmit(&hi2c2, address, dt, count + 1, 10);
}
```

<br/>

#### 3. UART
* HAL 드라이버 함수를 사용하여 UART 통신을 수행합니다.
* printf 함수를 구현하여 사용합니다.
```C
extern UART_HandleTypeDef *huart1;

int _write(int file, char *p, int len) 
{
	HAL_UART_Transmit(&huart1, (uint8_t*) p, len, 10);
	return len;
}
```

<br/>

#### 4. ONEWIRE
* 초기화를 진행합니다. (0 = OK, 1 = ERROR)
```C
uint8_t OneWire_Reset(OneWire_t *OneWireStruct) 
{
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

<br/>

* ROM 주소를 선택합니다. 
```C
void OneWire_SelectWithPointer(OneWire_t *OneWireStruct, uint8_t *ROM) {
	uint8_t i;
	
	OneWire_WriteByte(OneWireStruct, ONEWIRE_CMD_MATCHROM);
	for (i = 0; i < 8; i++) 
		OneWire_WriteByte(OneWireStruct, *(ROM + i));
	
}
```

<br/>

* LSB 방식으로 8 bit 정보를 전송합니다. (8 bit 정보는 명령 테이블 통해 확인할 수 있습니다.)
* SetResolution 함수와 StartAll 함수에서 슬레이브에 명령과 ROM 주소를 전송할 때 사용합니다.  
```C
void OneWire_WriteByte(OneWire_t *OneWireStruct, uint8_t byte) 
{
	uint8_t i = 8;
	while (i--) 
	{
		OneWire_WriteBit(OneWireStruct, byte & 0x01);
		byte >>= 1;
	}
}
```

<br/>

* 센서의 전환 상태를 확인하고 bit 정보가 완료(1) 실행(0)을 반환합니다.
```C
uint8_t OneWire_ReadBit(OneWire_t *OneWireStruct)
{
	uint8_t bit = 0;

	ONEWIRE_LOW(OneWireStruct);
	ONEWIRE_OUTPUT(OneWireStruct);
	ONEWIRE_DELAY(2);
	ONEWIRE_INPUT(OneWireStruct);
	ONEWIRE_DELAY(10);	

	if (HAL_GPIO_ReadPin(OneWireStruct->GPIOx, OneWireStruct->GPIO_Pin)) 
		bit = 1;
	ONEWIRE_DELAY(50);
	
	return bit;
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
* STM32CubeIDE에서 GUI를 통해 생성된 초기화 코드를 해석하고 HAL 드라이버의 입출력 함수를 제어하며 ST3M32 MCU 기본을 학습하였습니다. 
* Timer 및 Interrept가 발생하도록 Pinout 및 Clock 을 설정하고, Interrept 영역에서 외부 파일을 포함하며 각각의 모듈 기능을 확인하였습니다.  
* SPI, I2C, UART, ONEWIRE 등 통신의 기초를 학습하고, 오픈소스 수정부터 프로젝트 적용까지의 전 과정을 진행하며 실무 역량을 강화하였습니다.
