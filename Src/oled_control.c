#include "oled_control.h"
#include "ssd1306.h"
#include "bitmap.h"
#include <stdio.h>

/*
 * OLED 에서 LOGO를 출력하고 시스템의 준비 및 시작을 알린다.
 * 기본 스크린, 온도 스크린, 상태 스크린을 화면에 출력한다.
 */
void StartScreen()
{
	SSD1306_DrawBitmap(0, 0, logo, 128, 64, 1);
	SSD1306_UpdateScreen();
	HAL_Delay(2000);

	SSD1306_InvertDisplay(1);
	HAL_Delay(1000);
	SSD1306_InvertDisplay(0);
	HAL_Delay(1000);
	SSD1306_InvertDisplay(1);
	HAL_Delay(1000);
	SSD1306_InvertDisplay(0);
	HAL_Delay(1000);

	SSD1306_Clear();

	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("HEATING", &Font_11x18, 1);

	SSD1306_GotoXY(30, 20);
	SSD1306_Puts("ROOM", &Font_11x18, 1);

	SSD1306_GotoXY(45, 40);
	SSD1306_Puts("READY..", &Font_11x18, 1);
	SSD1306_UpdateScreen();

	SSD1306_ScrollRight(0x00, 0x0f);
	HAL_Delay(2500);
	SSD1306_ScrollLeft(0x00, 0x0f);
	HAL_Delay(2500);

	SSD1306_Stopscroll();
	SSD1306_Clear();

	SSD1306_GotoXY(0, 0);
	SSD1306_Puts("HEATING", &Font_11x18, 1);
	SSD1306_GotoXY(30, 20);
	SSD1306_Puts("ROOM", &Font_11x18, 1);
	SSD1306_GotoXY(45, 40);
	SSD1306_Puts("START!!", &Font_11x18, 1);
	SSD1306_UpdateScreen();
	HAL_Delay(2000);

	SSD1306_Clear();

	DefaultScreen();
	TempScreen(DEFAULT_TEMP);
	WorkScreen(OFF_t);
}

// 온도정보와 상태정보를 보여주는 기본 스크린을 출력한다.
void DefaultScreen()
{
	SSD1306_GotoXY(0, 0);
	SSD1306_Puts(" TEMP WORK ", &Font_11x18, 1);
	SSD1306_GotoXY(0, 15);
	SSD1306_Puts("___________", &Font_11x18, 1);
}

// 선택온도를 문자열로 변환하고 출력한다.
void TempScreen(int temp)
{
	char str[50] = "";

	sprintf(str, "%2d.0", temp);

	SSD1306_GotoXY(10, 40);
	SSD1306_Puts(str, &Font_11x18, 1);
	SSD1306_UpdateScreen();
}

// Relay 작동상태를 ON/OFF로 출력한다
void WorkScreen(ON_OFF_t state)
{
	SSD1306_GotoXY(80, 40);
	if (state == ON_t)
		SSD1306_Puts("ON ", &Font_11x18, 1);
	else
		SSD1306_Puts("OFF", &Font_11x18, 1);

	SSD1306_UpdateScreen();
}

// 기본, 온도, 상태 스크린을 2회 점멸한다. (온도설정효과)
void SetScreen(int temp, ON_OFF_t state)
{
	for (int i = 0; i < 4; i++)
	{
		if (i % 2 == 0)
		{
			SSD1306_Clear();
			SSD1306_UpdateScreen();
		}
		else
		{
			DefaultScreen();
			TempScreen(temp);
			WorkScreen(state);
		}
		HAL_Delay(250);
	}
}
