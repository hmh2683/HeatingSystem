#include "fnd_control.h"

uint8_t _LED_0F[29];

static SPI_HandleTypeDef *fhspi;

void FND_Init(SPI_HandleTypeDef *hspi)
{
	fhspi = hspi;

	_LED_0F[0] = 0xC0; //0
	_LED_0F[1] = 0xF9; //1
	_LED_0F[2] = 0xA4; //2
	_LED_0F[3] = 0xB0; //3
	_LED_0F[4] = 0x99; //4
	_LED_0F[5] = 0x92; //5
	_LED_0F[6] = 0x82; //6
	_LED_0F[7] = 0xF8; //7
	_LED_0F[8] = 0x80; //8
	_LED_0F[9] = 0x90; //9
	_LED_0F[10] = 0x88; //A
	_LED_0F[11] = 0x83; //B
	_LED_0F[12] = 0xC6; //C
	_LED_0F[13] = 0xA1; //D
	_LED_0F[14] = 0x86; //E
	_LED_0F[15] = 0x8E; //F
	_LED_0F[16] = 0xC2; //G
	_LED_0F[17] = 0x89; //H
	_LED_0F[18] = 0xF9; //I
	_LED_0F[19] = 0xF1; //J
	_LED_0F[20] = 0xC3; //L
	_LED_0F[21] = 0xA9; //N
	_LED_0F[22] = 0xC0; //O
	_LED_0F[23] = 0x8C; //P
	_LED_0F[24] = 0x98; //q
	_LED_0F[25] = 0x92; //S
	_LED_0F[26] = 0xC1; //U
	_LED_0F[27] = 0x91; //Y
	_LED_0F[28] = 0xFE; //-
}

// STM32 에서 제공되는 SPI 기능을 활용하여 CLK 을 만들어낸다.
void send(uint8_t X)
{
	HAL_SPI_Transmit(fhspi, &X, 1, 100);
}

/*
 * FND 에 출력되는 LED 8개, DIG 4개의 BIT 정보를 send 함수에 보낸다.
 * RCLK 을 LOW 로 출력하여 12개의 BIT 정보를 FND에 표시하고 HIGH를 출력한다.
 */
void send_port(uint8_t X, uint8_t port)
{
	send(X);
	send(port);

	HAL_GPIO_WritePin(FND_RCLK_GPIO_Port, FND_RCLK_Pin, LOW);
	HAL_GPIO_WritePin(FND_RCLK_GPIO_Port, FND_RCLK_Pin, HIGH);
}

/*
 * 온도정보를 정수형 변수에 받아와 각 자릿수의 정수를 표시한다.
 * 온도의 소수점은 항상 출력될 수 있도록 DP2 LED를 0x7F로 AND 연산한다.
 * 받아온 자릿수까지만 출력하기 위해 COUNT를 지정하여 지정 자릿수까지 출력한다.
 */
void DisplayTemp(int temp)
{
	static int digit_count = 0;

	int n1 = (int) temp % 10;
	int n2 = (int) (temp % 100) / 10;
	int n3 = (int) (temp % 1000) / 100;
	int n4 = (int) (temp % 10000) / 1000;

	switch (digit_count++)
	{
	case 0:
		send_port(_LED_0F[n1], 0b0001);
		break;
	case 1:
		send_port(_LED_0F[n2] & 0x7F, 0b0010);
		break;
	case 2:
		if (temp > 99)
			send_port(_LED_0F[n3], 0b0100);
		break;
	case 3:
		if (temp > 999)
			send_port(_LED_0F[n4], 0b1000);
		break;
	default:
		break;
	}

	if (temp < 100 && digit_count == 2)
		digit_count = 0;

	if (temp < 1000 && digit_count == 3)
		digit_count = 0;

	if (digit_count == 4)
		digit_count = 0;

}
