#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_INA219.h>
#include <TimeLib.h>

#define FINISH_SOUND_TIME 3000
#define START_CURRENT_THRESHOLD 0.02 // 20mA
#define DISPLAY_REFRESH_RATE 500     // ms

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_ADDRESS 0x3C

#define BUZZER_PIN A7

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);
Adafruit_INA219 ina219;

bool measuring = false;
bool finishing = false;
uint64_t startMillis;
uint64_t totalCapacity = 0;
uint64_t totalTime = 0;

void setup()
{
  delay(500);
  Serial.begin(9600);

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setTextColor(SSD1306_WHITE);
  ina219.begin();
  ina219.setCalibration_32V_2A();
}

void loop()
{
  float current = getCurrent();
  float voltage = getVoltage();

  if (!measuring)
  {
    if (current > START_CURRENT_THRESHOLD)
    {
      startMillis = millis();
      totalCapacity = 0;
      measuring = true;
      finishing = false;
    }
    if (finishing)
    {
      displaySummary(totalTime, totalCapacity, voltage);
    }
    else
    {
      displayWelcome();
    }
  }
  else
  {
    totalTime = (millis() - startMillis) / 1000;
    updateCapacity(totalTime, current);

    displayUpdate(totalTime, current, voltage, totalCapacity);

    if (current < START_CURRENT_THRESHOLD)
    {
      measuring = false;
      finishing = true;
      buzzer();
    }
  }
  delay(DISPLAY_REFRESH_RATE);
}

void updateCapacity(int time, float current)
{
  static uint32_t lastMinute = 0;
  static uint64_t samplesCurrent = 0;
  static double sumCurrent = 0;

  sumCurrent += current;
  samplesCurrent++;

  uint8_t timeMinute = minute(time);

  if (timeMinute != lastMinute && timeMinute != 0)
  {
    lastMinute = timeMinute;
    float avgCurrent = sumCurrent / samplesCurrent;
    totalCapacity += avgCurrent * 1000 / 60;

    sumCurrent = 0;
    samplesCurrent = 0;
  }
}

float getCurrent()
{
  return ina219.getCurrent_mA() / 1000;
}

float getVoltage()
{
  float shuntV = ina219.getShuntVoltage_mV();
  float busV = ina219.getBusVoltage_V();
  float loadV = busV + (shuntV / 1000);
  return loadV;
}

float getPower()
{
  return ina219.getPower_mW();
}

void displayWelcome()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(3);
  display.print("WELCOME");
  display.display();
}

void displaySummary(int time, int capacity, float voltage)
{
  char timeBufferS[3];
  char timeBufferM[3];
  char timeBufferH[3];
  char capacityBuffer[5];

  sprintf(timeBufferH, "%d", hour(time));
  sprintf(timeBufferM, "%02d", minute(time));
  sprintf(timeBufferS, "%02d", second(time));
  sprintf(capacityBuffer, "%04d", capacity);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 7);
  display.print("CAPACITY ");
  display.setTextSize(2);
  display.setCursor(54, 0);
  display.print(capacityBuffer);
  display.setTextSize(1);
  display.setCursor(108, 7);
  display.print("mAh");

  display.setCursor(0, 25);
  display.print("VOLTAGE ");
  display.setCursor(46, 25);
  display.print(voltage);
  display.setCursor(76, 25);
  display.print("V");

  display.setTextSize(1);

  display.setCursor(88, 25);
  display.print(timeBufferH);
  display.setCursor(98, 25);
  display.print(timeBufferM);
  display.setCursor(114, 25);
  display.print(timeBufferS);
  display.setCursor(93, 25);
  display.print(":");
  display.setCursor(109, 25);
  display.print(":");

  display.drawLine(0, 20, 128, 20, SSD1306_WHITE);
  display.drawLine(84, 20, 84, 32, SSD1306_WHITE);

  display.display();
}

void displayUpdate(int time, float current, float voltage, int capacity)
{
  char timeBufferS[3];
  char timeBufferM[3];
  char timeBufferH[3];
  char capacityBuffer[5];
  char currentDecBuffer[3];

  sprintf(timeBufferH, "%d", hour(time));
  sprintf(timeBufferM, "%02d", minute(time));
  sprintf(timeBufferS, "%02d", second(time));

  sprintf(capacityBuffer, "%04d", capacity);
  sprintf(currentDecBuffer, "%02d", (int)((current * 100) - ((int)current * 100)));

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print(timeBufferH);
  display.setCursor(16, 0);
  display.print(timeBufferM);
  display.setCursor(44, 0);
  display.print(timeBufferS);

  display.setCursor(8, 0);
  display.print(":");
  display.setCursor(36, 0);
  display.print(":");

  display.setTextSize(1);
  display.setCursor(92, 0);
  display.print(voltage);
  display.print("V");

  drawAnim();

  display.setTextSize(2);
  display.setCursor(82, 18);
  display.print((int)current);
  display.setCursor(98, 18);
  display.print(currentDecBuffer);
  display.setTextSize(1);
  display.setCursor(92, 25);
  display.print(".");

  display.setTextSize(2);
  display.setCursor(0, 18);
  display.print(capacityBuffer);

  display.setTextSize(1);
  display.setCursor(122, 25);
  display.print("A");

  display.setCursor(48, 25);
  display.print("mAh");

  display.display();

  // For Arduino IDE Serial Plotter Tool
  Serial.print("Current:");
  Serial.print(current); // A
  Serial.print(",");
  Serial.print("Voltage:"); // V
  Serial.print(voltage);
  Serial.print(",");
  Serial.print("Capacity:"); // mAh
  Serial.print(capacity);
  Serial.print(",");
  Serial.print("Power:"); // mW
  Serial.println(getPower());
}

void drawAnim()
{
  display.drawRect(73, 2, 8, 12, SSD1306_WHITE);
  display.fillRect(75, 0, 4, 2, SSD1306_WHITE);
  static uint8_t size = 0;
  if (size > 10)
  {
    size = 0;
  }
  display.fillRect(74, 13 - size, 6, size, SSD1306_WHITE);
  size++;

  static bool blink = false;
  if (blink)
  {
    display.setTextSize(1);
    display.setCursor(92, 9);
    display.print("CHARGE");
  }
  blink = !blink;
}

void buzzer()
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(FINISH_SOUND_TIME);
  digitalWrite(BUZZER_PIN, LOW);
}
