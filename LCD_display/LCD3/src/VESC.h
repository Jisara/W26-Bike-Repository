#ifndef VESC_H
#define VESC_H

#include <Arduino.h>
#include <VescUart.h>

extern VescUart UART;

void vescInit(HardwareSerial &serialPort = Serial1, int rxPin = 17, int txPin = 18);
bool vescUpdate();

#endif