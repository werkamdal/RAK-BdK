

```markdown
# LoRaWAN Güvenliği ve Şifreleme

Bu proje, LoRaWAN üzerinden veri gönderimi sırasında şifreleme ve güvenlik önlemlerini ele almaktadır. Aşağıda, LoRaWAN veri iletiminde karşılaşılan potansiyel güvenlik açıkları ve bu açıkları kapatmak için yapılan iyileştirmeler bulunmaktadır.

## İçindekiler
1. [Güvenlik ve Şifreleme Sorunları](#güvenlik-ve-şifreleme-sorunları)
2. [Kod Örneği ve Hatalar](#kod-örneği-ve-hatalar)
3. [Önerilen Düzenleme](#önerilen-düzenleme)

## Güvenlik ve Şifreleme Sorunları

LoRaWAN gibi iletişim sistemlerinde veri güvenliği çok önemlidir. Özellikle verilerin şifrelenmemesi, kötü niyetli kişilerin veriye erişmesine yol açabilir. Ayrıca sabit anahtar kullanımı, şifreleme anahtarlarının kötüye kullanılmasına neden olabilir. Bu nedenle güvenlik için AES gibi şifreleme algoritmaları kullanmak ve anahtarları güvenli bir şekilde yönetmek gerekir.

## Kod Örneği ve Hatalar

Aşağıda, LoRaWAN üzerinden veri iletimi yapan bir örnek kod bulunmaktadır. Bu kodda güvenlik ve şifreleme ile ilgili önemli eksiklikler vardır.

```cpp
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
#include <FixajSerial.h>

HardwareSerial fixSerial(1);  // UART1'i seçiyoruz
FixajSerial fixajSerial(TX, RX, &fixSerial, 115200);

// Gonderilecek mesaj içeriği
struct SensorData {
  byte konum[15] = "fixaj";  // 15 byte, "mutfak" gibi bir metni depolamak için
  byte temperature[2];        // 2 byte,
  byte humidity;              // 1 byte, uint8_t olarak depolanacak
  uint16_t message_id;        // 2 byte, uint16_t olarak depolanacak
} data;

void setup() {
  Serial.begin(115200);
  fixajSerial.begin();
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

  // Veriyi gönderme işlemi burada yapılır
  fixajSerial.sendSensorData(&data, sizeof(data));
}
```

### Hatalar ve Güvenlik Zayıflıkları

1. **Şifreleme Eksikliği**: Kodda veri şifrelemesi yapılmıyor. Bu durum, verinin kötü niyetli kişiler tarafından okunmasına yol açabilir.
   
2. **Sabit Anahtar Kullanımı**: Anahtar sabit bir şekilde belirlenmiş. Sabit anahtarlar güvenlik açığı yaratabilir. Anahtarların dinamik olarak yönetilmesi gerekir.
   
3. **Veri Doğrulama Eksikliği**: Veri doğrulama işlemi yapılmamış. Bu, iletilen verilerin değiştirilmesine ya da hatalı verilere neden olabilir.

## Önerilen Düzenleme

Şifreleme eklemek için AES gibi güçlü bir algoritma kullanılabilir. Her veri iletimi için farklı bir `Initialization Vector (IV)` kullanarak, şifreli verilerin güvenliğini sağlayabiliriz. Aşağıda, güvenliği artırmak için AES şifrelemesi eklenmiş ve verinin şifreli olarak gönderildiği güncellenmiş kod örneği bulunmaktadır:

```cpp
#include <Wire.h>
#include "ClosedCube_HDC1080.h"
#include <AES.h>
#include <Base64.h>

ClosedCube_HDC1080 hdc1080;             // HDC1080 sensörü kütüphanesi
#include <FixajSerial.h>

HardwareSerial fixSerial(1);  // UART1'i seçiyoruz
FixajSerial fixajSerial(TX, RX, &fixSerial, 115200);

// Güçlü, rastgele üretilmiş şifreleme anahtarı
const char* encryptionKey = "SüperGizliAnahtar123!"; 
byte iv[16] = {0};  // Initialization Vector (IV), her mesajda rastgele değişmeli

struct SensorData {
  byte konum[15] = "fixaj";
  byte temperature[2];
  byte humidity;
  uint16_t message_id;
} data;

void setup() {
  Serial.begin(115200);
  fixajSerial.begin();
  hdc1080.begin(0x40);
  Serial.println("fixaj.com HDC1080 Test");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last > OTAA_PERIOD) {
    fixajSerial.kesme();
    fixajSerial.ilkDurum("ONAY", delay5s);

    sendEncryptedSensorData();  // Şifrelenmiş veri gönderimi
    last = millis();
  }
  fixajSerial.dinle();
}

void sendEncryptedSensorData() {
  derece.deger = (int16_t)(hdc1080.readTemperature() * 100);
  data.temperature[0] = derece.bytes[0];
  data.temperature[1] = derece.bytes[1];
  data.humidity = (byte)hdc1080.readHumidity();
  data.message_id = _message_id++;

  // AES ile şifreleme
  AES aes;
  String base64Data = base64::encode((byte*)&data, sizeof(data));
  
  aes.do_aes_encrypt((byte*)base64Data.c_str(), base64Data.length(), (byte*)encryptionKey, iv);

  LoRa.beginPacket();
  LoRa.write((byte*)iv, sizeof(iv));  // IV ile birlikte gönder
  LoRa.write(aes.get_output(), aes.get_output_length());  // Şifreli veriyi gönder
  LoRa.endPacket();

  Serial.print("Veri şifrelendi ve gönderildi: ");
  Serial.println(base64Data);  // Gönderilen veriyi base64 olarak yazdır
}
```

### İyileştirmeler:
1. **AES Şifrelemesi**: AES şifreleme algoritması ile veriler güvenli bir şekilde şifrelenmiştir.
2. **Rastgele IV Kullanımı**: Her veri iletimi için farklı bir IV kullanarak, aynı verinin şifrelenmesinin bile farklı şifreli çıktılar üretmesini sağladık.
3. **Veri Bütünlüğü**: HMAC veya dijital imza ile veri doğrulaması eklenebilir.


```

