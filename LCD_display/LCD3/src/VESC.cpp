#include "VESC.h"

VescUart UART;

void vescInit(HardwareSerial &serialPort, int rxPin, int txPin) {
  Serial.begin(9600);
  Serial.println("Serial port initialized");
  serialPort.begin(115200, SERIAL_8N1, rxPin, txPin);
  UART.setSerialPort(&serialPort);
}

bool vescUpdate() {
  return UART.getVescValues();
}