#include <uFire_SHT20.h> // Library sensor SHT20
#include <Wire.h> // Library I2c
#include <CTBot.h> // Library bot telegram
#include <ESP8266WiFi.h> // Library ESP8266
#include <LiquidCrystal_I2C.h> // Library LCD
uFire_SHT20 sht20; // Library sensor SHT20

#define pinkipas D5 // Deklarasi variabel tetap pin kipas
#define pinheater D3 // Deklarasi variabel tetap pin heater
#define pinuv D4 // Deklarasi variabel tetap pin lampu UV

// Jenis Sepatu
#define CANVAS 1 // Deklarasi variabel tetap sepatu canvas
#define RUNNING 2 // Deklarasi variabel tetap sepatu running
#define LEATHER 3 // Deklarasi variabel tetap sepatu leather

CTBot myBot; // Library CTBot Telegram
CTBotInlineKeyboard myKbd; 
const char *ssid = "Polinema Hotspot"; // username Wifi
const char *password = ""; // Password Wifi
String newHostname = "pengering-sepatu"; // Pemanggilan nama pengguna pada telegram
String token = "5515432350:AAHd7PAjGlItQqMfW5_9qOcZmrDBpeO2ATk"; // Token chatbot dari BotFather

float suhu, kelembaban; // Pembuatan variabel
LiquidCrystal_I2C lcd(0x27, 16, 2); // Library LCD 16x2
char buff[33];
int jenissepatu = CANVAS;
int minSuhu, midSuhu, maxSuhu, minKelembaban, maxKelembaban; // Pembuatan nilai variabel
int pwm; // Pembuatan variabel PWM kipas
bool pengering = false; // Pemilihan kondisi

float sangatlembab, lembab, kuranglembab, tidaklembab; // Pembuatan variabel kondisi
float krg, krglmb, lmb, sgtlmb; // Pembuatan variabel kondisi
float rule1, rule2, rule3, rule4; // Pembuatan nilai rule pada fuzzy logic


byte temperatureChar[] = {0x0E, 0x0A, 0x0A, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E}; // Menampilkan output berupa icon temperature pada LCD
byte humidityChar[] = {0x00, 0x04, 0x0A, 0x1F, 0x1F, 0x1F, 0x0E, 0x00}; // Menampilkan output berupa icon humidity pada LCD
byte fanChar[] = {0x00, 0x0A, 0x15, 0x0E, 0x15, 0x0A, 0x00, 0x00}; // Menampilkan output berupa icon kipas pada LCD
byte heaterChar[] = {0x00, 0x1B, 0x1B, 0x04, 0x04, 0x1B, 0x1B, 0x00}; // Menampilkan output berupa icon heater pada LCD
byte uvChar[] = {0x0E, 0x1F, 0x11, 0x11, 0x0A, 0x0A, 0x04, 0x00}; // Menampilkan output berupa icon lampu uv pada LCD
unsigned long previousMillis = 0; // Perluasan nilai variabel dimulai dari nol

unsigned char tl() { // Deklarasi kondisi tidak lembab
  if (kelembaban < 50) {
    tidaklembab = 1;
  }
  else if (kelembaban >= 50 && kelembaban < 60) {
    tidaklembab = (60 - kelembaban) / 10;
  }
  else if (kelembaban >= 60) {
    tidaklembab = 0;
  }
  return krg;
}
unsigned char kl() { // Deklarasi kondisi kurang lembab
  if (kelembaban < 50) {
    kuranglembab = 0;
  }
  else if (kelembaban >= 50 && kelembaban < 60) {
    kuranglembab = (kelembaban - 50) / 10;
  }
  else if (kelembaban >= 60 && kelembaban < 70) {
    kuranglembab = (70 - kelembaban) / 10;
  }
  else if (kelembaban >= 70){
    kuranglembab = 0;
  }
  return krglmb;
}
unsigned char l() { // Deklarasi kondisi lembab
  if (kelembaban < 60) {
    lembab = 0;
  }
  else if (kelembaban >= 60 && kelembaban < 70) {
    lembab = (kelembaban - 60) / 10;
  }
  else if (kelembaban >= 70 && kelembaban < 80) {
    lembab = (80 - kelembaban) / 10;
  }
  else if (kelembaban >= 80){
    lembab = 0;
  }
  return lmb;
}
unsigned char sl() { // Deklarasi kondisi sangat lembab
  if (kelembaban < 70) {
    sangatlembab = 0;
  }
  else if (kelembaban >= 70 && kelembaban < 80) {
    sangatlembab = (kelembaban - 70) / 10;
  }
  else if (kelembaban >= 80) {
    sangatlembab = 1;
  }
  return sgtlmb;
}

void fuzzifikasi() { // Deklarasi fuzzifikasi dari kondisi kelembaban
  tl();
  kl();
  l();
  sl();
}
void fuzzy_rule() { // Deklarasi hasil perhitungan fuzzy rule dari fuzzifikasi
  fuzzifikasi();
  rule1 = 179 - (tidaklembab * 38);
  rule2 = 217 - (kuranglembab * 38);
  rule3 = 255 - (lembab * 38);
  rule4 = 217 + (sangatlembab * 38);
  pwm = ((rule1 * tidaklembab) + (rule2 * kuranglembab) + (rule3 * lembab) + (rule4 * sangatlembab)) / (tidaklembab + kuranglembab + lembab + sangatlembab);
}

void initWifi() { // inisialisasi Wifi
  WiFi.mode(WIFI_STA);
  WiFi.hostname(newHostname.c_str());
  WiFi.begin(ssid, password); // Mencocokkan SSID dan Password
  lcd.setCursor(0, 0); // Lokasi pada LCD
  lcd.print("   CONNECTING   "); // Menampilkan output "CONNECTING" pada LCD
  while (WiFi.status() != WL_CONNECTED) { // pengecekan status wifi
    delay(250); // Jeda 250ms
  }
  lcd.setCursor(0, 0); // Lokasi pada LCD
  lcd.print("   CONNECTED!   "); // Menampilkan output "CONNECTED" Pada saat wifi terhubung
  lcd.setCursor(0, 1); // Lokasi pada LCD
  lcd.print("IP:"); // Menampilkan "IP Address" yang tertera
  lcd.setCursor(3, 1); // Lokasi pada LCD
  lcd.print(WiFi.localIP().toString()); // Menampilkan IP Address
  delay(2000); // Jeda 2s
  lcd.clear(); // Menghapus semua hasil tampilan pada LCD
}

void setup() { // Menjalankan program satu kali pada awal program dijalankan
  Serial.begin(9600); // Kecepatan data 9600ms

  // Inisiasi Sensor
  Wire.begin();
  sht20.begin();

  // Inisisasi LCD
  lcd.begin();
  lcd.backlight();
  lcd.createChar(1, temperatureChar);
  lcd.createChar(2, humidityChar);
  lcd.createChar(3, fanChar);
  lcd.createChar(4, heaterChar);
  lcd.createChar(5, uvChar);
  lcd.clear();

  // Wifi Start
  initWifi();
  myBot.setTelegramToken(token);

  // Init Aktuator
  pinMode(pinkipas, OUTPUT);
  pinMode(pinheater, OUTPUT);
  pinMode(pinuv, OUTPUT);
  analogWrite(pinkipas, 0);
  digitalWrite (pinheater, HIGH);
  digitalWrite (pinuv, HIGH);
  pengering = false;
}

void loop() { // Menjalankan program berulang pada awal program dijalankan
  // Pembacaan Sensor SHT20
  String heaterStatus = digitalRead(pinheater) ? "OFF" : "ON "; // Deklarasi status heater
  int spdKipas = map(pwm, 0, 255, 0, 100); // Deklarasi kecepatan kipas
  String uvStatus = digitalRead(pinuv) ? "OFF" : "ON "; // Deklarasi status Lampu UV
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 1000) {
    previousMillis = currentMillis;
    suhu = sht20.temperature(); // Pemanggilan pembacaan suhu pada sensor
    kelembaban = sht20.humidity (); // Pemanggilan pembacaan kelembaban pada sensor
  }

  fuzzy_rule(); // Menampilkan Hasil Fuzzy Rule pada Serial monitor
  Serial.print("kering : ");
  Serial.print(tidaklembab);
  Serial.print("t   ");
  Serial.print("kurang lembab : ");
  Serial.print(kuranglembab);
  Serial.print("t   ");
  Serial.print("lembab : ");
  Serial.print(lembab);
  Serial.print("t   ");
  Serial.print("sangat lembab : ");
  Serial.println(sangatlembab);
  Serial.print("t   ");
  Serial.print("Rule1 : ");
  Serial.println(rule1);
  Serial.print("Rule2 : ");
  Serial.println(rule2);
  Serial.print("Rule3 : ");
  Serial.println(rule3);
  Serial.print("Rule4 : ");
  Serial.println(rule4);
  Serial.print("Hasil DeFuzzy: ");
  Serial.println(pwm);
  delay (1000);

  // Show LCD
  lcd.setCursor(0, 0);
  lcd.write(1);
  sprintf(buff, ":%02d.%02dC", (int)suhu, (int)(suhu * 100) % 100);
  lcd.setCursor(1, 0);
  lcd.print(buff);
  lcd.setCursor(0, 1);
  lcd.write(2);
  sprintf(buff, ":%02d.%02d%%", (int)kelembaban, (int)(kelembaban * 100) % 100);
  lcd.setCursor(1, 1);
  lcd.print(buff);
  lcd.setCursor(9, 0);
  lcd.write(5);
  lcd.setCursor(10, 0);
  lcd.print(uvStatus);
  lcd.setCursor(9, 1);
  lcd.write(3);
  sprintf(buff, "%03d%%", spdKipas);
  lcd.setCursor(10, 1);
  lcd.print(buff);

  switch (jenissepatu) { // Pemilihan jenis sepatu untuk menentukan batas nilai 
    case CANVAS:
      minSuhu = 27;
      maxSuhu = 35;
      minKelembaban = 75;
      maxKelembaban = 55;
      break;
    case RUNNING:
      minSuhu = 27;
      maxSuhu = 32;
      minKelembaban = 75;
      maxKelembaban = 53;
      break;
    case LEATHER:
      minSuhu = 27;
      maxSuhu = 31;
      minKelembaban = 75;
      maxKelembaban = 58;
      break;
  }

  // Telegram Control
  TBMessage msg;
  if (myBot.getNewMessage(msg)) {
    if (msg.messageType == CTBotMessageText) {
      if (msg.text.equalsIgnoreCase("/start")) {
        if (pengering) {
          myBot.sendMessage(
            msg.sender.id,
            "Pengering sedang beroperasi"
          );
        } else {
          myBot.sendMessage(
            msg.sender.id,
            "Selamat datang " + msg.sender.firstName + " " + msg.sender.lastName + "\n" +
            "Pengering dinyalakan"
          );
          pengering = true;
        }
      }
      if (msg.text.equalsIgnoreCase("/stop")) {
        if (pengering) {
          myBot.sendMessage(
            msg.sender.id,
            "Pengering dimatikan"
          );
          pengering = false;
        } else {
          myBot.sendMessage(
            msg.sender.id,
            "Pengering tidak beroperasi"
          );
        }
      }
      if (msg.text.equalsIgnoreCase("/monitor")) {
        myBot.sendMessage(
          msg.sender.id,
          "Suhu : " + String(suhu) + "°C \n" +
          "Kelembaban : " + String(kelembaban) + "%\n" +
          "Heater : " + heaterStatus + "\n" +
          "Kipas : " + String(spdKipas) + "% \n" +
          "UV : " + uvStatus
        );
      }
      if (msg.text.equalsIgnoreCase("/canvas")) {
        myBot.sendMessage(
          msg.sender.id,
          "Pengering diatur untuk sepatu canvas\nSuhu Maximum : " + String(maxSuhu) + "\n" +
          "Kelembaban Maksimum : " + String(maxKelembaban)
        );
        jenissepatu = CANVAS;
      }
      if (msg.text.equalsIgnoreCase("/running")) {
        myBot.sendMessage(
          msg.sender.id,
          "Pengering diatur untuk sepatu running\nSuhu Maximum : " + String(maxSuhu) + "\n" +
          "Kelembaban Maksimum : " + String(maxKelembaban)
        );
        jenissepatu = RUNNING;
      }
      if (msg.text.equalsIgnoreCase("/leather")) {
        myBot.sendMessage(
          msg.sender.id,
          "Pengering diatur untuk sepatu leather\nKelembaban Maksimum : " + String(maxKelembaban)
        );
        jenissepatu = LEATHER;
      }
      if (msg.text.equalsIgnoreCase("/status")) {
        String statusPengering = pengering ? "ON" : "OFF";
        myBot.sendMessage(
          msg.sender.id,
          "Pengering dalam kondisi " + statusPengering
        );
      }
    }
  }

  if (pengering) { // Pemilihan kondisi sesuai jenis sepatu
    if (jenissepatu != LEATHER) { // Pemilihan jenis sepatu kecuali LEATHER
      if (kelembaban >= minKelembaban) { // Kelembaban lebih dari batas bawah kelembaban
        analogWrite(pinkipas, pwm); // Kipas Nyala
        digitalWrite(pinuv, LOW); // UV Nyala
        if (suhu <= minSuhu) { // Suhu kurang dari batas atas suhu
          digitalWrite(pinheater, LOW); // Heater nyala
        }
        else if (suhu > minSuhu && suhu <= maxSuhu) {
          digitalWrite (pinheater, LOW);
        }
        else { // Suhu lebih dari batas atas suhu
          digitalWrite(pinheater, HIGH); // heater mati
        }
      }
      else if (kelembaban < minKelembaban && kelembaban >= maxKelembaban) {
        analogWrite(pinkipas, pwm); // Kipas Nyala
        digitalWrite(pinuv, LOW); // UV Nyala
        if (suhu <= minSuhu) { // Suhu kurang dari batas atas suhu
          digitalWrite(pinheater, LOW); // Heater nyala
        }
        else if (suhu > minSuhu && suhu <= maxSuhu) {
          digitalWrite (pinheater, LOW);
        }
        else { // Suhu lebih dari batas atas suhu
          digitalWrite(pinheater, HIGH); // heater mati
        }
      }
      else { // Kelembaban kurang dari batas bawah kelembaban
        digitalWrite(pinheater, HIGH); // heater mati
        if (suhu <= minSuhu) { // Suhu lebih dari batas bawah suhu
          analogWrite(pinkipas, pwm); // Kipas Nyala
          digitalWrite(pinuv, LOW); // UV Nyala
        }
        else if (suhu > minSuhu && suhu <= maxSuhu) {
          analogWrite(pinkipas, pwm); // Kipas Mati
          digitalWrite(pinuv, LOW); // UV Nyala
        }
        else {
          analogWrite(pinkipas, 0); // Kipas Mati
          digitalWrite(pinuv, HIGH); // UV Mati
          pengering = false;
        }
      }
    } else { //LEATHER
      digitalWrite(pinuv, LOW); // UV Nyala
      analogWrite(pinkipas, pwm);
      if (kelembaban >= minKelembaban) {
        if (suhu <= minSuhu) { // Suhu kurang dari batas atas suhu
          digitalWrite(pinheater, LOW); // Heater nyala
        }
        else if (suhu > minSuhu && suhu <= maxSuhu) {
          digitalWrite (pinheater, LOW);
        }
        else { // Suhu lebih dari batas atas suhu
          digitalWrite(pinheater, HIGH); // heater mati
        }
      }
      else if (kelembaban < minKelembaban && kelembaban >= maxKelembaban) {
        analogWrite(pinkipas, pwm); // Kipas Nyala
        digitalWrite(pinuv, LOW); // UV Nyala
        if (suhu <= minSuhu) { // Suhu kurang dari batas atas suhu
          digitalWrite(pinheater, LOW); // Heater nyala
        }
        else if (suhu > minSuhu && suhu <= maxSuhu) {
          digitalWrite (pinheater, LOW);
        }
        else { // Suhu lebih dari batas atas suhu
          digitalWrite(pinheater, HIGH); // heater mati
        }
      }
      else {
        digitalWrite(pinheater, HIGH); // heater mati
        if (suhu <= minSuhu) { // Suhu lebih dari batas bawah suhu
          analogWrite(pinkipas, pwm); // Kipas Nyala
          digitalWrite(pinuv, LOW); // UV Nyala
        }
        else if (suhu > minSuhu && suhu <= maxSuhu) {
          analogWrite(pinkipas, pwm); // Kipas Mati
          digitalWrite(pinuv, LOW); // UV Nyala
        }
        else {
          analogWrite(pinkipas, 0); // Kipas Mati
          digitalWrite(pinuv, HIGH); // UV Mati
          pengering = false;
        }
      }
    }
  } else { // Keadaan terakhir pada saat semua kondisi terpenuhi
    digitalWrite(pinuv, HIGH);
    digitalWrite(pinheater, HIGH);
    analogWrite(pinkipas, 0);
  }
}
