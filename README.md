# Heating System
* Period : 2022.03-2022.08 (6 months)
* Personnel : 1 person
* Development Environment : STM32CubeIDE 1.9.0
* Development Language : C
* Development Goals  
  * The current temperature is displayed on the FND module by calling the temperature information of the sensor from the STM32.
  * Select the set temperature through 3 buttons and OLED module, and display status information of relay through OLED module.
  * Control relay by comparing set temperature with current temperature and maintain and manage set temperature.

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
<a href="#"><img src="https://github.com/hmh2683/heatingroom.project/blob/main/images/pinout.png" width="400px" height="400px"></a>

### System Core
* GPIO -> INPUT -> PB0_TEMP_UP, PB1_TEMP_SET, PB2_TEMP_DOWN, PB12_START_SW
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
* A clock frequency of 72 MHz is generated using the crystal resonator built into the STM32 MCU.
<a href="#"><img src="https://github.com/hmh2683/heatingroom.project/blob/main/images/clock.png" width="1000px" height="400px"></a> 

<br/> <br/>

## Code Review
### Main
* The if statement is executed according to the button variable that is interrupted.
* LED is controlled according to the state of the start switch.
* Start temperature conversion and get temperature value.
* When the switch is on, the relay operates according to the temperature value, and when the switch is off, the relay does not operate regardless of the temperature value.
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
* When an interrupt occurs, the if statement is executed by comparing the return value of the HAL_GetTick function with the CLICK_TIME value.
```C
void EXTI0_IRQHandler(void) {
  HAL_GPIO_EXTI_IRQHandler(PB0_TEMP_UP_BUTTON_Pin);

	if (HAL_GetTick() - before_time > CLICK_TIME) {
		button_up = 1;
	}
	before_time = HAL_GetTick();
}
```
* Activate RCC mode and set TIM3 prescaler: 72, period: 100 to generate an interrupt every 100us.
* Check the initialization status and usage status, and if true, get the current temperature value and display it in FND.
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
* Create the SPI function directly in software.
* Controls the clock by 1 bit from MSB.
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
* It uses the SPI function provided by STM32.
```C
static SPI_HandleTypeDef *fhspi;

void send(uint8_t X) {
 	HAL_SPI_Transmit(fhspi, &X, 1, 100);
}
```
* Lower RCLK to LOW and raise it to HIGH again to send 16-bit information.
```C
void sendPort(uint8_t X, uint8_t port) {
	send(X);
	send(port);
	HAL_GPIO_WritePin(PB14_FND_RCLK_GPIO_Port, PB14_FND_RCLK_Pin, LOW);
	HAL_GPIO_WritePin(PB14_FND_RCLK_GPIO_Port, PB14_FND_RCLK_Pin, HIGH);
}
```

#### 2. I2C 
* The command registered in the command table is sent to the slave address.
* Send in 7-bit units to control the OLED.
```C
void ssd1306_I2C_Write(uint8_t address, uint8_t reg, uint8_t data) {
	uint8_t dt[2];
	dt[0] = reg;
	dt[1] = data;
	HAL_I2C_Master_Transmit(&hi2c2, address, dt, 2, 10);
}
```
* Register 1024 bit value in slave address.
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
* UART handler and transfer function provided by STM32 are used.
* Used to implement the printf function.
```C
extern UART_HandleTypeDef *huart1;

int _write(int file, char *p, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t*) p, len, 10);
	return len;
}
```
#### 4. ONEWIRE
* If read bit value is low, initialization is successful.
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
* Send command information to slave device.
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
* If read bit is low, then device is not finished yet with calculation temperature.
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
* Select ROM number.
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
<a href="#"><img src="https://github.com/hmh2683/heatingroom.project/blob/main/images/result.png" width="500px" height="400px"></a>
<a href="#"><img src="https://github.com/hmh2683/heatingroom.project/blob/main/images/uart.png" width="500px" height="400px"></a>

### Video
[![Heating room temperature control system](http://img.youtube.com/vi/CozpDULafgo/0.jpg)](https://youtu.be/CozpDULafgo?t=0s) 

<br/> <br/>

## Realization
* The automatic generation of STM32CubeIDE confirmed that the pin was set up, and I was able to master how to use the interrupt and timer.
* I learned C language grammar when I was an undergraduate, but I was able to learn how to use grammar and skills for the first time in the project and learn a little more about how to interact and include them in the library.
* I was able to learn about the communication between chips such as SPI, I2C, UART, and ONEWIRE, and I was able to learn and experience the communication method by directly modifying the software implementation using open source.
