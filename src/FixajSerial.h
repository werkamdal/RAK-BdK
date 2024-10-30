#ifndef FIXAJSERIAL_H
#define FIXAJSERIAL_H

#include <Arduino.h>
#include <HardwareSerial.h>

#define delay100 100
#define delay1s 1000
#define delay5s 5000
#define delay10s 10000

#define M0 4
#define M1 6

#define interruptOlustur() \
  { \
    digitalWrite(M1, HIGH); \
    delay(1000); \
    digitalWrite(M1, LOW); \
    delay(1000); \
  }

class FixajSerial {

public: 
  FixajSerial(int8_t txPin, int8_t rxPin, Stream* serialPort, unsigned long baudRate);
  void begin();
  void AyarlarGonderBaglan(const char* serialMessages[], const char* fixajPrefixMessages[], const char* fixajMessages[]);
  void sendSensorData(const void* dataStruct, size_t dataSize); 
  bool ilkDurum(String msg, int delaySure);     //node gel mesajları bekler
  void kesme();
  bool dinle();                                 //basic mesaj gönderildi durum kontrolü için
  bool dinle(char* message, size_t maxLength);  //downlink mesajı almak için

private:
  char command[1024] = { 0 };                   //AT komutları gönderirken kullanılır
  byte CevapYOKReset = 0;                       //mesaj gönderildi mesajı gelmezse kendini resetler esp
  Stream* fixajSerial;                          //Serial i kütüphanede hallediyoruz uart2
  unsigned long baudRate;
  int8_t txPin;
  int8_t rxPin;
  void structToHex(const void* dataStruct, size_t dataSize, char* hexString, size_t hexStringSize);  //struct verisini lorawana göndermek için HEX e çeviren fonksiyon
  void sendATCommand(const char* serialMsg, const char* fixajPrefixMsg, const char* fixajMsg, const char* response, int delayDuration);
};

#endif  // FIXAJSERIAL_H
