#ifndef INC_FND_CONTROL_H_
#define INC_FND_CONTROL_H_

#include "main.h"

#define HIGH 1
#define LOW 0

void FND_Init(SPI_HandleTypeDef * hspi);
void send(uint8_t X);
void send_port(uint8_t X, uint8_t port);
void DisplayTemp(int temp);

#endif
