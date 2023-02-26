#include "heating_control.h"
#include "oled_control.h"
#include "main.h"
#include "global.h"
#include <stdio.h>

char button_up = 0;
char button_down = 0;
char button_set = 0;

static int select_temp = DEFAULT_TEMP;
static int set_temp = 0;
static ON_OFF_t relay_state = 1;

extern UART_HandleTypeDef *huart1;

int _write(int file, char *p, int len)
{
	HAL_UART_Transmit(huart1, (uint8_t*) p, len, 10);
	return len;
}

/*
 * 버튼 값에 따라 온도를 선택하며 설정온도를 지정한다. (OLED, LED, USART)
 * 설정버튼을 한번 더 클릭하여 온도를 재설정한다.
 */
void SelectButton()
{
	static char button_flag = 0;

	if (button_up == 1 && button_flag == 0) {
		button_up = 0;

		select_temp++;
		if (select_temp > 99)
			select_temp = 0;

		TempScreen(select_temp);

		HAL_GPIO_TogglePin(BUTTON_LED_GPIO_Port, BUTTON_LED_Pin);
		HAL_Delay(20);
		HAL_GPIO_TogglePin(BUTTON_LED_GPIO_Port, BUTTON_LED_Pin);
		printf("Heating room temperature : %d.0\r\n", select_temp);
	}
	if (button_down == 1 && button_flag == 0) {
		button_down = 0;

		select_temp--;
		if (select_temp < 0)
			select_temp = 99;

		TempScreen(select_temp);

		HAL_GPIO_TogglePin(BUTTON_LED_GPIO_Port, BUTTON_LED_Pin);
		HAL_Delay(20);
		HAL_GPIO_TogglePin(BUTTON_LED_GPIO_Port, BUTTON_LED_Pin);
		printf("Heating room temperature : %d.0\r\n", select_temp);
	}
	if (button_set == 1 && button_flag == 0) {
		button_set = 0;
		button_flag = 1;
		set_temp = select_temp;

		SetScreen(set_temp, relay_state);

		HAL_GPIO_TogglePin(BUTTON_LED_GPIO_Port, BUTTON_LED_Pin);
		printf("Confirmation of heating room temperature\r\n");
	}
	if (button_set == 1 && button_flag == 1) {
		button_set = 0;
		button_flag = 0;

		HAL_GPIO_TogglePin(BUTTON_LED_GPIO_Port, BUTTON_LED_Pin);
		printf("Cancel heating room temperature\r\n");
	}
}

/*
 * Switch 상태에 따라 LED ON/OFF 하고, Relay 상태에 따라 OLED ON/OFF 한다.
 * Switch ON 상태에서 현재온도(매개변수)와 설정온도(전역변수)를 비교하고 현재온도가 설정온도 보다 낮다면 Relay ON, 그렇지 않다면 OFF 한다.
 * Switch OFF 상태에서 관계없이 Relay OFF 한다.
 */
void CheckSwitch(int current_temp)
{
	static int start_sw_time = 0;

	if (start_sw_time > START_SW_CHECK_TIME)
	{
		if (!HAL_GPIO_ReadPin(START_SW_GPIO_Port, START_SW_Pin))
		{
			HAL_GPIO_WritePin(START_LED_GPIO_Port, START_LED_Pin, ON_t);

			if ((int)current_temp < set_temp)
			{
				HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, ON_t);
				WorkScreen(ON_t);
				relay_state = 0;
			}
			else if ((int)current_temp >= (set_temp - SELECTED_TEMP_RANGE))
			{
				HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, OFF_t);
				WorkScreen(OFF_t);
				relay_state = 1;
			}
		}
		else
		{
			HAL_GPIO_WritePin(START_LED_GPIO_Port, START_LED_Pin, OFF_t);

			HAL_GPIO_WritePin(RELAY_CTRL_GPIO_Port, RELAY_CTRL_Pin, OFF_t);
			WorkScreen(OFF_t);
			relay_state = 1;
		}
		start_sw_time = 0;
	}
	start_sw_time++;
}
