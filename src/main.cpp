#include <Arduino.h>
//#include <DHT.h>
#include <SD.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>

#define RXD2 16
#define TXD2 17
// #define DHT11P 32
int step = 9;
int curread;
int data[9];
int check = 0;
uint8_t broadcastAddress[] = {0xec, 0xe3, 0x34, 0x88, 0xf4, 0x7c};
typedef struct wiadomosc {
  float PM25V;
  float PM10V;
  //float temp;
  //float humid;
  long long czas;
} wiadomosc;

wiadomosc dane;
esp_now_peer_info_t peerInfo;
File plik;
// DHT dht(DHT11P,DHT11);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  //dht.begin();
  Serial.begin(921600);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  SD.begin(5);
  pinMode(2, OUTPUT);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }

  delay(2000);
  Serial.println("Serial Txd is on pin: "+String(TX));
  Serial.println("Serial Rxd is on pin: "+String(RX));
  if(SD.exists("/dane.txt")){
    Serial.println("Istnieje");
  } else {
    plik = SD.open("/dane.txt", FILE_WRITE);
    plik.close();
  }
  plik = SD.open("/dane.txt", FILE_APPEND);
  plik.println("");
  plik.println("---------------------------------");
  plik.println("PM2.5 PM10 Czas");
  plik.close();
}

void loop() {
  while (Serial2.available()) {
    curread = Serial2.read();
    if(curread == 0xAA && step == 9){
      step = 0;
    } else if(curread == 0xC0 && step == 0){
      step = 1;
    } else if(curread == 0xAB && step == 8){
      step = 9;
      check = data[2]+data[3]+data[4]+data[5]+data[6]+data[7]-data[8];
      if(check%256 == 0){
        //znak
        digitalWrite(2,HIGH);
        delay(2);
        // obliczenia
        //dane.humid = dht.readHumidity();
        //dane.temp = dht.readTemperature();
        dane.PM25V = (data[3]*256+data[2]);
        dane.PM25V = dane.PM25V/10;
        dane.PM10V = (data[5]*256+data[4]);
        dane.PM10V = dane.PM10V/10;
        dane.czas = millis();
        // Otwieranie pliku
        plik = SD.open("/dane.txt", FILE_APPEND);
        // PM 2.5
        Serial.print("PM2.5: ");
        plik.print(dane.PM25V);
        plik.print(" ");
        Serial.println(dane.PM25V);
        // PM 10
        Serial.print("PM10: ");
        plik.print(dane.PM10V);
        plik.print(" ");
        Serial.println(dane.PM10V);
        /* Temperatura
        Serial.print("Temperatura: ");
        plik.print(dane.temp);
        plik.print(" ");
        Serial.print(dane.temp);
        Serial.println(" C");
        // Wilgotność
        Serial.print("Wilgotność: ");
        plik.print(dane.humid);
        plik.print(" ");
        Serial.print(dane.humid);
        Serial.println(" %");
        */
        // czas
        Serial.print("Czas: ");
        plik.print(dane.czas);
        plik.println("");
        Serial.println(dane.czas);
        // wysylanie
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &dane, sizeof(dane));
        if (result == ESP_OK) {
          Serial.println("Sent with success");
        }
        else {
          Serial.println("Error sending the data");
        }
        // Zamykanie pliku
        plik.close();
        digitalWrite(2,LOW);
      } else {
        Serial.println("Blad");
        Serial.print("Time: ");
        Serial.println(millis());
      }
    } else if(step == 8 || step == 0){
      step = 9;
    } else {
      step++;
      data[step] = curread;
    }
  }
}