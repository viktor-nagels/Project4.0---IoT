#include <Arduino.h>
/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-button-toggle-relay
 */

#define BUTTON_PIN 16 // ESP32 pin GIOP22 connected to button's pin
#define RELAY_PIN 27  // ESP32 pin GIOP27 connected to relay's pin
#define LED_PIN 17


// variables will change:
int relay_state = HIGH; // the current state of relay
int button_state;       // the current state of button
int led_pin;

void setup()
{
  Serial.begin(9600);                // initialize serial
  pinMode(BUTTON_PIN, INPUT_PULLUP); // set ESP32 pin to input pull-up mode
  pinMode(RELAY_PIN, OUTPUT);        // set ESP32 pin to output mode
  pinMode(LED_PIN, OUTPUT);

  button_state = digitalRead(BUTTON_PIN);
}

void loop()
{

  button_state = digitalRead(BUTTON_PIN); // read new state
  digitalWrite(LED_PIN, HIGH);
  if (relay_state == HIGH)
  {
    digitalWrite(LED_PIN, LOW);
  }
  else
  {
    digitalWrite(LED_PIN, HIGH);
  }

  if (button_state == LOW)
  {
    Serial.println("Emergency button is pressed");
    digitalWrite(RELAY_PIN, HIGH);
    relay_state = LOW;
  }
  delay(500);
}
