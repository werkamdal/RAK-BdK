#include "esp32-hal-gpio.h"
#include "FixajSerial.h"

FixajSerial::FixajSerial(int8_t txPin, int8_t rxPin, Stream* serialPort, unsigned long baudRate) {
  this->txPin = txPin;
  this->rxPin = rxPin;
  this->fixajSerial = serialPort;
  this->baudRate = baudRate;

  pinMode(M1, OUTPUT);
  pinMode(M0, OUTPUT);
}

void FixajSerial::begin() {
  ((HardwareSerial*)fixajSerial)->begin(baudRate, SERIAL_8N1, rxPin, txPin);
}

void FixajSerial::kesme() {
  interruptOlustur();
}

void FixajSerial::AyarlarGonderBaglan(const char* serialMessages[], const char* fixajPrefixMessages[], const char* fixajMessages[]) {

  digitalWrite(M0, LOW);  //  rak modüle RESET at
  Serial.println(F("Rak modulü resetlendi"));
  delay(delay1s);
  pinMode(M0, INPUT);
  ilkDurum("TAMAM", 1000);
  Serial.println("ayarlar yollanıyor ilkayarlar");

  Serial.println(F("UYARI- Hardware Model no ile Firmware Model Aynı olmalıdır."));
  Serial.println(F("AT+HWMODEL=rak3172 ise Versiyon RAK3172-E olmalıdır."));

  // 7 adet komutu array'den sırayla çekip göndermek
  for (int i = 8; i < 10; i++) {
    sendATCommand(serialMessages[i], fixajPrefixMessages[i], fixajMessages[i], "OK", 100);
  }

  Serial.println(F("Lütfen Yukarıdaki Uyarıyı kontrol edin."));
  delay(2000);

  while (fixajSerial->available() > 0) {
    fixajSerial->read();
  }

  //Lora ilk Ayarlar
  for (int i = 0; i < 7; i++) {
    sendATCommand(serialMessages[i], fixajPrefixMessages[i], fixajMessages[i], "OK", 100);
  }

  Serial.println(serialMessages[7]);
  fixajSerial->println(fixajPrefixMessages[7]);
  Serial.print(F("Lorawan Ağına Bağlanıyor...10sn bekleyin..."));
  delay(12000); 
  ilkDurum("+EVT:JOINED", delay5s);
  Serial.println(F("LoraWAN BAĞLANDI."));
  /***
    burada neden bazı ayarları ağa bağlandıktan sonra veriyoruz
    çünkü bu ayarlar lora modülü ile alakalı değil bağlanılan ağ ile
    ilgili ayarlar olduğu için bağlandıktan sonra vermek daha mantıklı.*/
  Serial.println("sonra ayarları yükleniyor...");
  for (int i = 10; i < 13; i++) {
    sendATCommand(serialMessages[i], fixajPrefixMessages[i], fixajMessages[i], "OK", 100);
  }

  Serial.println(F("Ayarlama Bitti, Node kısmı callback fonksiyonları kuruyor.."));
  kesme();
  ilkDurum("ONAY", delay100);
}

void FixajSerial::sendATCommand(const char* serialMsg, const char* fixajPrefixMsg, const char* fixajMsg, const char* response, int delayDuration) {
  Serial.print(F(fixajPrefixMsg));
  Serial.println(F(serialMsg));
  fixajSerial->print(fixajPrefixMsg);
  fixajSerial->println(fixajMsg);
  ilkDurum(response, delayDuration);
}

bool FixajSerial::ilkDurum(String msg, int delaySure) {
  const int maxAttempts = 20;  // Toplam sorgulama sayısı
  int attempts = 0;
  bool messageReceived = false;
  String message = "";

  while (attempts < maxAttempts) {
    if (fixajSerial->available() > 0) {
      message = fixajSerial->readStringUntil('\n');  // Serial'den gelen mesajı oku
      if (message.indexOf(msg) != -1) {              // "tamam" mesajı geldiyse
        messageReceived = true;
        Serial.println("ONAY.");
        delay(100);
        while (fixajSerial->available() > 0) {
          fixajSerial->read();
        }
        break;  // Döngüden çık
      }
    }
    if (!message.isEmpty()) {
      Serial.println(message);
    }
    attempts++;
    delay(delaySure);  // Belirtilen süre kadar bekle
  }

  if (!messageReceived) {
    ESP.restart();
    return false;
  } else {
    return true;
  }
}

void FixajSerial::sendSensorData(const void* dataStruct, size_t dataSize) {
  if (dataSize == 0) {
    Serial.println("Hata: Geçersiz veri boyutu.");
    return;
  }

  // Convert struct to hex string
  char hexString[dataSize * 2 + 1];
  structToHex(dataStruct, dataSize, hexString, sizeof(hexString));

  // Prepare and send AT command
  snprintf(command, 1024, "AT+SEND=%d:%s\r\n", 2, hexString);

  fixajSerial->print("\r\n");
  fixajSerial->print(command);
  fixajSerial->flush();
  delay(50);
  Serial.println(F("Mesaj Gönderiliyor..."));

  CevapYOKReset++;
}

bool FixajSerial::dinle(char* message, size_t maxLength) {
  static bool downlinkReady = false;  // "Data received!" mesajı alındığında bayrak ayarlanacak
  int index = 0;

  // Serial'den karakterleri oku ve null terminatörü ile bitir
  while (fixajSerial->available() > 0 && index < maxLength - 1) {
    char c = fixajSerial->read();
    if (c == '\n') {  // Satır sonuna kadar oku
      break;
    }
    message[index++] = c;
  }
  message[index] = '\0';  // Sonuna null terminatörü ekle

  // Son kontrol: message pointer'ının geçerli olup olmadığını kontrol et
  if (message == nullptr) {
    Serial.println(F("Geçersiz pointer, dinleme sırasında hata oluştu!"));
    return false;  // Pointer geçersiz hale geldiyse false döndür
  }

  // Gelen mesajın boş olup olmadığını kontrol et
  if (index == 0) {
    return false;  // Eğer mesaj boşsa false döndür
  }

  // Mesajların kontrolü ve ekrana yazdırılması
  if (strstr(message, "SEND_CONFIRMED_OK") != NULL) {
    CevapYOKReset = 0;
    Serial.print(F("Mesaj Servera ulaştı. "));
    Serial.println(message);  // gelenMesaj'ı ekrana yazdır
  } else if (strstr(message, "OK") != NULL) {
    Serial.print(F("Mesaj başarı ile gönderildi. "));
    Serial.println(message);  // gelenMesaj'ı ekrana yazdır
  } else if (strstr(message, "Data received!") != NULL) {
    Serial.println(F("Data received! Mesajı alındı, downlink bekleniyor..."));
    downlinkReady = true;
  } else if (downlinkReady) {
    // Eğer "Data received!" mesajından sonra bu mesaj gelmişse, bu downlink mesajıdır
    downlinkReady = false;  // Bayrağı sıfırla
    return true;            // Downlink mesajı geldiği için true döndür
  } else {
    Serial.print(F("Gelen mesaj: "));
    Serial.println(message);  // gelenMesaj'ı ekrana yazdır
  }

  if (CevapYOKReset >= 5) {
    Serial.println("HATA- Lora Modülü ile bağlantı koptu, Sistem Yeniden Başlatılıyor");
    delay(delay1s);
    ESP.restart();
  }

  return false;  // Diğer tüm durumlarda false döndür
}

bool FixajSerial::dinle() {
  char gelenMesaj[100];               // 100 karaktere kadar olan mesajları tutacak char dizisi
  static bool downlinkReady = false;  // "Data received!" mesajı alındığında bayrak ayarlanacak
  int index = 0;
  size_t maxLength = sizeof(gelenMesaj);

  // Serial'den karakterleri oku ve null terminatörü ile bitir
  while (fixajSerial->available() > 0 && index < maxLength - 1) {
    char c = fixajSerial->read();
    if (c == '\n') {  // Satır sonuna kadar oku
      break;
    }
    gelenMesaj[index++] = c;
  }
  gelenMesaj[index] = '\0';  // Sonuna null terminatörü ekle

  // Son kontrol: message pointer'ının geçerli olup olmadığını kontrol et
  if (gelenMesaj == nullptr) {
    Serial.println(F("Geçersiz pointer, dinleme sırasında hata oluştu!"));
    return false;  // Pointer geçersiz hale geldiyse false döndür
  }

  // Gelen mesajın boş olup olmadığını kontrol et
  if (index == 0) {
    return false;  // Eğer mesaj boşsa false döndür
  }

  // Mesajların kontrolü ve ekrana yazdırılması
  if (strstr(gelenMesaj, "SEND_CONFIRMED_OK") != NULL) {
    CevapYOKReset = 0;
    Serial.print(F("Mesaj Servera ulaştı. "));
    Serial.println(gelenMesaj);  // gelenMesaj'ı ekrana yazdır
  } else if (strstr(gelenMesaj, "OK") != NULL) {
    Serial.print(F("Mesaj başarı ile gönderildi. "));
    Serial.println(gelenMesaj);  // gelenMesaj'ı ekrana yazdır
  } else {
    Serial.print(F("Gelen mesaj: "));
    Serial.println(gelenMesaj);  // Gelen mesajı ekrana yazdır
  }

  if (CevapYOKReset >= 5) {
    Serial.println("HATA- Lora Modülü ile bağlantı koptu, Sistem Yeniden Başlatılıyor");
    delay(delay1s);
    ESP.restart();
  }

  return true;  // Diğer tüm durumlarda false döndür
}

void FixajSerial::structToHex(const void* dataStruct, size_t dataSize, char* hexString, size_t hexStringSize) {
  const byte* pData = (const byte*)dataStruct;
  size_t requiredSize = dataSize * 2 + 1;

  if (hexStringSize < requiredSize) {
    Serial.println("Hata: hexString buffer'ı yeterli büyüklükte değil.");
    return;
  }

  for (size_t i = 0; i < dataSize; ++i) {
    sprintf(hexString + i * 2, "%02X", pData[i]);
  }
  hexString[dataSize * 2] = '\0';
}
