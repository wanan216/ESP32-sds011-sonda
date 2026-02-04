#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <esp_now.h>
#include <WiFi.h>

#define RXD2 16
#define TXD2 17
#define LED_PIN 2
int step = 9;
int curread;
int data[9];
int check = 0;

uint8_t broadcastAddress[] = {0xec, 0xe3, 0x34, 0x88, 0xf4, 0x7c};
typedef struct wiadomosc {
  float PM25V;
  float PM10V;
  long long czas;
} wiadomosc;
wiadomosc dane;
esp_now_peer_info_t peerInfo;

File plik;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  //przygotowywanie komunikacji
  Serial.begin(921600);
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Błąd inicjacji ESP-NOW");
    return;
  }
  esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Nie udało się dodać urządzenia");
    return;
  }
  //przygotowanie sensora sds011
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  //przygotowanie karty pamięci
  SD.begin(5);
  pinMode(LED_PIN, OUTPUT);
  if(SD.exists("/dane.txt")){
    Serial.println("Plik istnieje");
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
    // czytanie aktualnej wartości
    curread = Serial2.read();
    // odkodowanie danych
    if(curread == 0xAA && step == 9){
      step = 0;
    } else if(curread == 0xC0 && step == 0){
      step = 1;
    } else if(curread == 0xAB && step == 8){
      step = 9;
      check = data[2]+data[3]+data[4]+data[5]+data[6]+data[7]-data[8];
      if(check%256 == 0){
        //znak
        digitalWrite(LED_PIN,HIGH);
        // obliczenia
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
        // czas
        Serial.print("Czas: ");
        plik.print(dane.czas);
        plik.println("");
        Serial.println(dane.czas);
        // wysylanie
        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &dane, sizeof(dane));
        if (!result == ESP_OK) {
          Serial.println("Błąd przesyłania danych");
        }
        // Zamykanie pliku
        plik.close();
        digitalWrite(LED_PIN,LOW);
      } else {
        // Wypisywanie błędu
        Serial.println("Blad");
        Serial.print("Time: ");
        Serial.println(millis());
      }
    } else if(step == 8 || step == 0){
      // sprawdzanie pewności przesyłu
      step = 9;
    } else {
      step++;
      data[step] = curread;
    }
  }
}