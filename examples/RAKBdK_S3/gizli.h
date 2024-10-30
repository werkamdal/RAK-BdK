

/* PAYLOAD FORMATTER TTN TARAFI
function Decoder(bytes, port) { 

  // Konum verisi (15 byte)
 var konumBytes = bytes.slice(0, 15);
  //var konumBytes = bytes.slice(0, 46);
  var konum = "";
  for (var i = 0; i < konumBytes.length; i++) {
    if (konumBytes[i] === 0) break;
    konum += String.fromCharCode(konumBytes[i]);
  } 
  
  //var derece = ( bytes[15] << 8 | bytes[16]);
  var hum = bytes[17];
  var messageId = (bytes[19] << 8) | bytes[18];
  
//  var celciusInt = (bytes[16] & 0x80 ? 0xFFFF0000 : 0) | bytes[16]<<8 | bytes[15];
  var celciusInt =  (bytes[16]) << 8 | bytes[15];
  	return{
		Temp:  celciusInt/100,
		Hum: hum, 
		Konum: konum,
		id: messageId
	};
}
*/ 

#define my_node_device_eui2 "123456789ABCDFEG"	//16 Basmaklı kendi device eui nizi yazın bu formatta
#define my_node_app_eui2 "0000000000000000"		//16 Basmaklı kendi app eui nizi yazın bu formatta 
#define my_node_app_key2 "12345678901234567890123456789012" // 32 basamaklı kendi app key yazınız bu formatta 

#define OTAA_BAND RAK_REGION_EU868
#define OTAA_PERIOD 20000

#define RX 17  //  Lora nın 3. pini RX in ESP32 de hangi pine bağlı olduğu
#define TX 18  //  Lora nın 4. pini TX in ESP32 de hangi pine bağlı olduğu
#define LEDrxtx 42 

uint16_t _message_id = 0;

//sıcaklık değerini yollarken float yapıp 4 byte yollamak yerine bu şekil 2 byte ile işimizi çözdük
union {
  int16_t deger;
  uint8_t bytes[2];
} derece;

// Enum değerlerine karşılık gelen karakter dizileri
const char* bandStrings[] = {
  "0",  // RAK_REGION_EU433
  "1",  // RAK_REGION_CN470
  "2",  // RAK_REGION_RU864
  "3",  // RAK_REGION_IN865
  "4",  // RAK_REGION_EU868
  "5",  // RAK_REGION_US915
  "6",  // RAK_REGION_AU915
  "7",  // RAK_REGION_KR920
  "8",  // RAK_REGION_AS923
  "9",  // RAK_REGION_AS923_2
  "10", // RAK_REGION_AS923_3
  "11", // RAK_REGION_AS923_4
  "12"  // RAK_REGION_LA915
};

typedef enum {
  RAK_REGION_EU433 = 0,     ///< EU433
  RAK_REGION_CN470 = 1,     ///< CN470 ~ 510
  RAK_REGION_RU864 = 2,     ///< RU864 ~ 870
  RAK_REGION_IN865 = 3,     ///< IN865 ~ 867
  RAK_REGION_EU868 = 4,     ///< EU863 ~ 870
  RAK_REGION_US915 = 5,     ///< US902 ~ 928
  RAK_REGION_AU915 = 6,     ///< AU915 ~ 928
  RAK_REGION_KR920 = 7,     ///< KR920 ~ 923
  RAK_REGION_AS923 = 8,     ///< AS923-1
  RAK_REGION_AS923_2 = 9,   ///< AS923-2
  RAK_REGION_AS923_3 = 10,  ///< AS923-3
  RAK_REGION_AS923_4 = 11,  ///< AS923-4
  RAK_REGION_LA915 = 12,    ///< LA915
} RAK_LORA_BAND;

// Serial ve fixaj için mesajları tanımlıyoruz

const char* serialMessages[] = {
  " ,Ağ modunu ayarlar (ör. LoRaWAN).",
  " ,Cihaz EUI'sini ayarlar.",
  " ,Uygulama EUI'sini ayarlar.",
  " ,Uygulama anahtarını ayarlar.",
  " ,LoRaWAN frekans bandını ayarlar.",
  " ,LoRaWAN cihaz sınıfını (A, B, C) ayarlar.",
  " ,Ağ katılım modunu ayarlar (OTAA/ABP).",
  "",
  " ,Model: Donanım modelini sorgular.",
  " ,Versiyon: Firmware versiyonunu sorgular.",
  " ,daptif Veri Hızını etkinleştirir/devre dışı bırakır.",
  " ,Yeniden iletim sayısını ayarlar.",
  " ,Onaylı mesaj modunu ayarlar."
};
// fixajSerial için prefix ve mesaj array'leri
const char* fixajPrefixMessages[] = {
  "AT+NWM=1",    // AT+NWM için bir prefix yok
  "AT+DEVEUI=",  // DEVEUI için prefix
  "AT+APPEUI=",  // APPEUI için prefix
  "AT+APPKEY=",  // APPKEY için prefix
  "AT+BAND=",    // BAND için prefix
  "AT+CLASS=A",  // CLASS için prefix yok
  "AT+NJM=1",
  "AT+JOIN=1:0:8:10",  // NJM için prefix yok
  "AT+HWMODEL=",
  "AT+VER=",
  "AT+ADR=1",
  "AT+RETY=1",
  "AT+CFM=1"
};

const char* fixajMessages[] = {
  "",
  my_node_device_eui2,
  my_node_app_eui2,
  my_node_app_key2,
  bandStrings[OTAA_BAND],
  "",
  "",
  "",
  "?",
  "?",
  "",
  "",
  ""
};

#include <Wire.h>
#include "ClosedCube_HDC1080.h"

ClosedCube_HDC1080 hdc1080;

