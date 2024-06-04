//RECEIVER
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <LoRa.h>

//lora
#define TXEN 32
#define RXEN 33
#define STATUS_LED 5

#define LORA_SPI_SS_PIN 15
SPIClass hspi(HSPI);

//display
#define RST -1
#define CE 21
#define DC 4
#define DIN 16
#define CLK 17
#define LCD_BL 2

bool STATUS_LED_STATE = 0;

const unsigned long TIMEOUT = 2000;  // Timeout period in milliseconds (5 seconds)
unsigned long lastPacketTime = 0;

Adafruit_PCD8544 display = Adafruit_PCD8544(CLK, DIN, DC, CE, RST);

// Define the LEDC channel, timer, and output pin
const int ledcChannel = 0;
const int ledcTimer = 0;
const int ledcOutputPin = 26;           // GPIO26
const int frequency = 1000;             // Frequency of the tone in Hz
const int resolution = 8;               // LEDC PWM resolution
const int beepDuration = 50;            // Duration of each beep in milliseconds
const int pauseDuration = 100;          // Pause duration between beeps in milliseconds
const int intervalBetweenBeeps = 1000;  // Interval between beep sequences

unsigned long previousMillis = 0;
bool isBeeping = false;
int beepState = 0;


void DispSetup() {
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  Serial.begin(115200);
  Serial.println("PCD test");
  display.begin();

  display.setContrast(60);

  display.clearDisplay();
  display.setRotation(0);
  display.setTextSize(1);
  display.setTextColor(BLACK);
  display.setCursor(0, 0);
  display.println("Init Display");
  display.setTextSize(1);
  display.println("Done..");
  display.display();
  delay(1000);
}

void setup() {

  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  // pinMode(TXEN, OUTPUT);
  // pinMode(RXEN, OUTPUT);
  // digitalWrite(TXEN, LOW);  // tx
  // digitalWrite(RXEN, HIGH); // rx

  //setup display
  DispSetup();

  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("LoRa Receiver");

  LoRa.setSPI(hspi);
  LoRa.setPins(LORA_SPI_SS_PIN, -1, -1);

  if (!LoRa.begin(915E6)) {
    Serial.println("Starting LoRa failed!");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Starting LoRa failed!");
    display.display();
    delay(1000);
    while (1)
      ;
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Receiver Mode");
  display.display();

  // Set up the LEDC peripheral
  ledcSetup(ledcChannel, frequency, resolution);

  // Attach the LEDC channel to the output pin
  ledcAttachPin(ledcOutputPin, ledcChannel);
}

String data;
int packetSize;
void loop() {
  // Try to parse packet
  packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Clear the data string
    data = "";

    // Read packet
    while (LoRa.available()) {
      char receivedChar = (char)LoRa.read();
      data += receivedChar;
    }
    Serial.printf("Received packet %s with RSSI %d\n", data.c_str(), LoRa.packetRssi());

    // Update the display
    display.clearDisplay();
    display.setCursor(12, 0);
    display.printf("RF Receiver");
    display.setCursor(0, 15);
    display.printf("Data:%s", data.c_str());
    display.setCursor(0, 25);
    display.printf("RSSI:%d", LoRa.packetRssi());
    display.display();

    // Update the status LED
    STATUS_LED_STATE = !STATUS_LED_STATE;
    digitalWrite(STATUS_LED, STATUS_LED_STATE);

    // Update the last packet time
    lastPacketTime = millis();
    beepState = 0;
  } else {
    // Check if no data has been received for a while
    if (millis() - lastPacketTime > TIMEOUT) {
      Serial.println("No data received for a while.");
      Serial.printf("Packet size %d\n", packetSize);

      // Update the display to indicate no data
      display.clearDisplay();
      display.setCursor(12, 0);
      display.printf("RF Receiver");
      display.setCursor(0, 15);
      display.printf("Out of range, No data received.");
      display.display();

      beepBeep();

      // Optionally, turn off the status LED or blink it in a different pattern
      digitalWrite(STATUS_LED, LOW);
    }
  }
}

void beepBeep() {
  unsigned long currentMillis = millis();

  switch (beepState) {
    case 0:
      if (currentMillis - previousMillis >= intervalBetweenBeeps) {
        previousMillis = currentMillis;
        ledcWriteTone(ledcChannel, frequency);
        isBeeping = true;
        beepState = 1;
      }
      break;

    case 1:
      if (currentMillis - previousMillis >= beepDuration) {
        previousMillis = currentMillis;
        ledcWriteTone(ledcChannel, 0);  // Stop the tone
        beepState = 2;
      }
      break;

    case 2:
      if (currentMillis - previousMillis >= pauseDuration) {
        previousMillis = currentMillis;
        ledcWriteTone(ledcChannel, frequency);
        beepState = 3;
      }
      break;

    case 3:
      if (currentMillis - previousMillis >= beepDuration) {
        previousMillis = currentMillis;
        ledcWriteTone(ledcChannel, 0);  // Stop the tone
        isBeeping = false;
        beepState = 0;
      }
      break;
  }
}
