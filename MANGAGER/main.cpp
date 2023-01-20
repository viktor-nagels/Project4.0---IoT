#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "Cipher.h"
#include "SPIFFSTest.h"

// BUTTONS
#define BUTTON1_PIN 16
#define BUTTON2_PIN 17
#define BUTTON3_PIN 21
#define RELAY_PIN 27
// LEDS
#define LED_GREEN_PIN 15
#define LED_YELLOW_PIN 32
#define LED_RED_PIN 14
// LORA
#define ss 27
#define rst 12
#define dio0 33

// variables
int relay_state = HIGH;
int button1_state;
int counter_sender = 0;
int button2_state;
int button3_state;
int last_button1_state;
int last_button2_state;
int last_button3_state;
char separator;
const char *ssid = "DESKTOP-7TUV9M0 4088";
const char *password = "o}33412S";
String sensor_id = "00001";
String status_sensor = "Active";
String id_esp32;
String id_sensor1;
String sensor1;
String id_sensor2;
String sensor2;
String sensor3;
String id_sensor4;
String sensor4;
String id_sensor5;
String sensor5;
String sensor6;
String sensor7;
const char *host = "hooyberghsapi20230117150416.azurewebsites.net";

String banaan = "{\"pumpModelID\":1,\"position\":\"Nieuwe pomp\",\"wharfID\":43}";

LiquidCrystal_I2C lcd(0x27, 20, 4);

Cipher *cipher = new Cipher();

CSPIFFS mSpiffs;

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);

  button1_state = digitalRead(BUTTON1_PIN);
  button2_state = digitalRead(BUTTON2_PIN);
  button3_state = digitalRead(BUTTON3_PIN);

  //! WIFI
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("*");
  }

  //! LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Welcome");
  lcd.setCursor(0, 1);
  lcd.print("hooyberghs NV");
  lcd.setCursor(16, 3);
  lcd.print("next");

  //! LEDS
  digitalWrite(LED_GREEN_PIN, 1);
  digitalWrite(LED_YELLOW_PIN, 1);
  digitalWrite(LED_RED_PIN, 1);

  //! LORA
  LoRa.setPins(ss, rst, dio0);

  while (!LoRa.begin(866E6))
  {
    Serial.println(".");
    delay(500);
  }

  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");

  char *key = "encrypted&secure";
  cipher->setKey(key);
  SPIFFS.begin(true);
  mSpiffs.listDir(SPIFFS, "/", 0);
}

void loop()
{
  last_button1_state = button1_state;
  last_button2_state = button2_state;
  last_button3_state = button3_state;

  button1_state = digitalRead(BUTTON1_PIN);
  button2_state = digitalRead(BUTTON2_PIN);
  button3_state = digitalRead(BUTTON3_PIN);

  if (last_button1_state == HIGH && button1_state == LOW)
  {
    Serial.println("LCD - Previous button pressed");
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("sensor_id prev: " + sensor_id);
    lcd.setCursor(0, 1);
    lcd.print("status: " + status_sensor);
    lcd.setCursor(0, 3);
    lcd.print("Previous");
    lcd.setCursor(16, 3);
    lcd.print("next");
  }

  if (last_button2_state == HIGH && button2_state == LOW)
  {
    Serial.println("LCD - Next button pressed");

    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("sensor_id_next : " + sensor_id);
    lcd.setCursor(0, 1);
    lcd.print("status: " + status_sensor);
    lcd.setCursor(0, 3);
    lcd.print("Previous");
    lcd.setCursor(16, 3);
    lcd.print("next");
  }

  if (last_button3_state == HIGH && button3_state == LOW)
  {
    Serial.println("button Relay is pressed");
    relay_state = !relay_state;
    digitalWrite(RELAY_PIN, relay_state);
    Serial.print("sending data");
    LoRa.beginPacket();
    LoRa.print("motor_status");
    LoRa.print("|");
    LoRa.print(relay_state);
    LoRa.endPacket();
    counter_sender++;

    //! post to database
    // Use WiFiClient class to create TCP connections
    WiFiClientSecure client;
    const int httpPort = 443; // 80 is for HTTP / 443 is for HTTPS!

    client.setInsecure(); // this is the magical line that makes everything work

    if (!client.connect(host, httpPort))
    { // works!
      Serial.println("connection failed");
      return;
    }

    // We now create a URI for the request
    String url = "/api/Pump/addPump";

    Serial.print("Requesting URL: ");
    Serial.println(url);

    // This will send the request to the server
    client.print("POST " + url + " HTTP/1.1\r\n"
                                 "Host: " +
                 host + "\r\n"
                        "Content-Type: application/json\r\n"
                        "Content-Length: " +
                 banaan.length() + "\r\n"
                                   "\r\n" +
                 banaan);

    String line = client.readStringUntil('!');
    Serial.println(line);
    Serial.println("closing connection");

    int packetSize = LoRa.parsePacket();
    if (packetSize)
    {
      // received a packet
      Serial.print("Received packet '");

      // read packet
      while (LoRa.available())
      {
        String packetEncrypted = LoRa.readString();
        mSpiffs.writeFile(SPIFFS, "/encrypted.txt", packetEncrypted);
        String LoRaData = cipher->decryptString(mSpiffs.getFile(SPIFFS, "/encrypted.txt"));
        LoRaData = LoRaData.substring(0, LoRaData.indexOf(0x2400));
        Serial.print(LoRaData);

        for (int i = 0; i < 12; i++)
        {

          separator = LoRaData.indexOf('|');
          switch (i)
          {
          case 0:
            id_esp32 = LoRaData.substring(0, separator);
            break;
          case 1:
            id_sensor1 = LoRaData.substring(0, separator);
            break;
          case 2:
            sensor1 = LoRaData.substring(0, separator);
            break;
          case 3:
            id_sensor2 = LoRaData.substring(0, separator);
            break;
          case 4:
            sensor2 = LoRaData.substring(0, separator);
            break;
          case 5:
            sensor3 = LoRaData.substring(0, separator);
            break;
          case 6:
            id_sensor4 = LoRaData.substring(0, separator);
            break;
          case 7:
            sensor4 = LoRaData.substring(0, separator);
            break;
          case 8:
            id_sensor5 = LoRaData.substring(0, separator);
            break;
          case 9:
            sensor5 = LoRaData.substring(0, separator);
            break;
          case 10:
            sensor6 = LoRaData.substring(0, separator);
            break;
          case 11:
            sensor7 = LoRaData.substring(0, separator);
            break;

          default:
            break;
          }
          LoRaData = LoRaData.substring(separator + 1, LoRaData.length());
        }
      }

      // print RSSI of packet
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
      Serial.println();
      Serial.println(id_esp32);
      Serial.println(id_sensor1);
      Serial.println(sensor1);
      Serial.println(id_sensor2);
      Serial.println(sensor2);
      Serial.println(sensor3);
      Serial.println(id_sensor4);
      Serial.println(sensor4);
      Serial.println(id_sensor5);
      Serial.println(sensor5);
      Serial.println(sensor6);
      Serial.println(sensor7);
    }
  }
}
