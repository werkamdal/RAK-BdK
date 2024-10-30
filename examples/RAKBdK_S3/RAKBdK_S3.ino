#include "gizli.h"
#include <FixajSerial.h>

HardwareSerial fixSerial(1);  // UART1'i seçiyoruz
FixajSerial fixajSerial(TX, RX, &fixSerial, 115200);

//Gonderilecek mesaj içeriği
struct SensorData {
  byte konum[15] = "fixaj";  // 15 byte, "mutfak" gibi bir metni depolamak için
  byte temperature[2];        // 2 byte,
  byte humidity;              // 1 byte, uint8_t olarak depolanacak
  uint16_t message_id;        // 2 byte, uint16_t olarak depolanacak
} data;

void setup() {
  pinMode(LEDrxtx, OUTPUT);
  digitalWrite(LEDrxtx, HIGH);

  Serial.begin(115200);
  fixajSerial.begin();

  time_t serial_timeout = millis();
  while (!Serial) {
    if ((millis() - serial_timeout) < delay5s) {
      delay(63);
      digitalWrite(LEDrxtx, !digitalRead(LEDrxtx));
    } else {
      break;
    }
  }
  digitalWrite(LEDrxtx, LOW);

  fixajSerial.AyarlarGonderBaglan(serialMessages, fixajPrefixMessages, fixajMessages);

  hdc1080.begin(0x40);
  Serial.println("fixaj.com HDC1080 Test");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last > OTAA_PERIOD) {

    fixajSerial.kesme();
    fixajSerial.ilkDurum("ONAY", delay5s);

    sendSensorData();
    last = millis();
  }

fixajSerial.dinle();

/*
  char downlinkMesaj[100];  // 100 karaktere kadar olan mesajları tutacak char dizisi

  if (fixajSerial.dinle(downlinkMesaj, sizeof(downlinkMesaj))) {
    // Eğer dinle fonksiyonu true dönerse, bu durum downlink mesajı alındığı anlamına gelir
    Serial.print(F("Merkezden Mesaj Geldi: "));
    Serial.println(downlinkMesaj);  // Mesajı ekrana yazdır
  }
*/
}

void sendSensorData() {

  derece.deger = (int16_t)(hdc1080.readTemperature() * 100);
  data.temperature[0] = derece.bytes[0];
  data.temperature[1] = derece.bytes[1];
  data.humidity = (byte)hdc1080.readHumidity();
  data.message_id = _message_id++;

  Serial.print("T=");
  Serial.print(*((int16_t*)data.temperature));
  Serial.print("C, RH=");
  Serial.print(data.humidity);
  Serial.print("%, id: ");
  Serial.println(data.message_id);
  // Veri gönderme
  fixajSerial.sendSensorData(&data, sizeof(data));
}
