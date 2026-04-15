#pragma once

#include <Arduino.h>
#include <VescUart.h>
#include <hardware.h>

extern VescUart UART;

void vescInit(HardwareSerial &serialPort, int rxPin, int txPin);
bool vescUpdate();
