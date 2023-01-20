#include <arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LoRa.h>
#include "Cipher.h"
#include "SPIFFSTest.h"

// All pins for sensor readings
#define SENSOR1 26
#define SENSOR2 25
#define BATTERYVOLTAGE1 34 //34
#define BATTERYVOLTAGE2 39 //39
#define ENERGYPUMP1 12 //12
#define ENERGYPUMP2 27 //27
#define FLOWRATEREADING 4 //4

#define ID_ESP32 1
#define ID_SENSOR1 1
#define ID_SENSOR2 2
#define ID_SENSOR3 3
#define ID_SENSOR4 4

// Min and max values of all sensors 
#define MINVALUESENSOR1 5
#define MAXVALUESENSOR1 -15
#define MINVALUESENSOR2 5
#define MAXVALUESENSOR2 -15
#define MINFLOWRATE 0
#define MAXFLOWRATE 5
#define MINENERGYPUMP1 3.10
#define MAXENERGYPUMP1 5.50
#define MINENERGYPUMP2 3.10
#define MAXENERGYPUMP2 5.50
#define MINVOLTAGEBAT1 11.50
#define MAXVOLTAGEBAT1 12.70
#define MINVOLTAGEBAT2 11.50
#define MAXVOLTAGEBAT2 12.70

#define MINENERGY 0
#define MAXENERGY 10
#define MINVOLTAGE 10
#define MAXVOLTAGE 15

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define ss 32
#define rst 15
#define dio0 14

// All variables which store the data for each sensor
float sensor1Value = 0.00;
float sensor2Value = 0.00;
float flowRate = 0.00;
float energyPump1Value = 0.00;
float energyPump2Value = 0.00;
float batterySensor1 = 0.00;
float batterySensor2 = 0.00;

String packetEncrypted;
String packetDecrypted;

float converter(float value, float min_val, float max_val)
{
  // Converting the analog value (0<->4095) to the value needed (min<->max)
  return (((value - 0.0) * (max_val - min_val)) / (4096.0 - 0.0)) + min_val;
}

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Cipher * cipher = new Cipher();

CSPIFFS mSpiffs;

void setup() {
  Serial.begin(115200);
  
  char * key = "encrypted&secure";
  cipher->setKey(key);
  SPIFFS.begin(true);
  mSpiffs.listDir(SPIFFS, "/", 0);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  while (!Serial);
  Serial.println("LoRa Sender");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(866E6)) {
    Serial.println(".");
    delay(500);
  }
   // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

void loop() {
  String packet;

  // Reading potentiometer value
  sensor1Value = converter(analogRead(SENSOR1), MINVALUESENSOR1, MAXVALUESENSOR1);
  sensor2Value = converter(analogRead(SENSOR2), MINVALUESENSOR2, MAXVALUESENSOR2);
  batterySensor1 = converter(analogRead(BATTERYVOLTAGE1), MINVOLTAGEBAT1, MAXVOLTAGEBAT1);
  batterySensor2 = converter(analogRead(BATTERYVOLTAGE2), MINVOLTAGEBAT2, MAXVOLTAGEBAT2);
  energyPump1Value = converter(analogRead(ENERGYPUMP1), MINENERGY, MAXENERGY);
  energyPump2Value = converter(analogRead(ENERGYPUMP2), MINENERGY, MAXENERGY);
  flowRate = converter(analogRead(FLOWRATEREADING), MINFLOWRATE, MAXFLOWRATE);

  // showing the values on the serial monitor
  Serial.print("Sensor 1: ");
  Serial.print(sensor1Value);
  Serial.print("m | ");
  Serial.print("Sensor 2: ");
  Serial.print(sensor2Value);
  Serial.print("m | ");
  Serial.print("Battery sensor 1: ");
  Serial.print(batterySensor1);
  Serial.print("V | ");
  Serial.print("Battery sensor 2: ");
  Serial.print(batterySensor2);
  Serial.println("V");
  Serial.print("Energy pump 1: ");
  Serial.print(energyPump1Value);
  Serial.print("A | ");
  Serial.print("Energy pump 2: ");
  Serial.print(energyPump2Value);
  Serial.print("A | ");
  Serial.print("Flow rate: ");
  Serial.print(flowRate);
  Serial.println("m³/s");
  Serial.println();

  packet.concat(ID_ESP32);
  packet.concat("|");
  packet.concat(ID_SENSOR1);
  packet.concat("|");
  packet.concat(sensor1Value);
  packet.concat("|");
  packet.concat(ID_SENSOR2);
  packet.concat("|");
  packet.concat(sensor2Value);
  packet.concat("|");
  packet.concat(flowRate);
  packet.concat("|");
  packet.concat(energyPump1Value);
  packet.concat("|");
  packet.concat(ID_SENSOR4);
  packet.concat("|");
  packet.concat(energyPump2Value);
  packet.concat("|");
  packet.concat(batterySensor1);
  packet.concat("|");
  packet.concat(batterySensor2);

  Serial.print("Packet to encrypt: ");
  Serial.println(packet);
  Serial.println();

  Serial.println("Encrypting packet...");

  packetEncrypted = cipher->encryptString(packet);

  mSpiffs.writeFile(SPIFFS, "/encrypted.txt", packetEncrypted);

  Serial.print("Encrypted packet: ");
  Serial.println(packetEncrypted);
  Serial.println();

  Serial.println("Decrypting packet...");

  String packetDecrypted = cipher->decryptString(mSpiffs.getFile(SPIFFS, "/encrypted.txt"));

  packetDecrypted = packetDecrypted.substring(0, packetDecrypted.indexOf(0x2400));

  Serial.print("Decrypted packet: ");
  Serial.println(packetDecrypted);
  Serial.println();

  // Sending LoRa package
  LoRa.beginPacket();
  LoRa.print(packetEncrypted);
  LoRa.endPacket();

  // Display Text
	display.setTextSize(1.8);
	display.setTextColor(WHITE);
	display.setCursor(0,0);
  display.print("Sensor 1: ");
	display.print(sensor1Value);
  display.println("m");
  display.setCursor(0,9);
  display.print("Sensor 2: ");
	display.print(sensor2Value);
  display.println("m");  
  display.setCursor(0,18);
  display.print("Bat voltage 1: ");
	display.print(batterySensor1);
  display.println("V");  
  display.setCursor(0,27);
  display.print("Bat voltage 2: ");
	display.print(batterySensor2);
  display.println("V");	
  display.setCursor(0,36);
  display.print("Energy pump 1: ");
	display.print(energyPump1Value);
  display.print("A");	
  display.setCursor(0,45);
  display.print("Energy pump 2: ");
	display.print(energyPump2Value);
  display.print("A");	
  display.setCursor(0,54);
  display.print("Flow rate: ");
	display.print(flowRate);
  display.println("m3/s");	
  display.display();
  delay(5000);
	display.clearDisplay();
}

