#include "Audio.h"
#include <mbed.h>
#include <SPI.h>
#include <ArduinoJson.h>

using namespace rtos;
 
using namespace std::chrono_literals;
 
Thread t1;

#define sclk 13 // CLK
#define mosi 11 // DIN
#define cs   10 // CS
#define rst  9 // RES
#define dc   8 // D/C

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

// Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, mosi, sclk, rst);
Adafruit_SSD1331 display = Adafruit_SSD1331(&SPI, cs, dc, rst);
Audio* audio;

void setup() {
  Serial.begin(115200);

  display.begin();
  display.fillScreen(0);
  display.setTextColor(WHITE);
  display.setTextSize(2);

  delay(500);

  t1.start(func1);

  display.setCursor(0, 10);
  display.print("Loading...");
  audio = new Audio(USE_APIKEY);
  display.fillScreen(0);
  display.setCursor(0, 10);
  display.print("Loaded!");
}

bool firstRun = true;
bool connected = true;

void loop() {
  if (Serial.available() != 0)
  {
    while(true){}
  }

  Serial.print(connected);
  Serial.print(" - ");
  Serial.println(audio->client.connected());
  if (connected != audio->client.connected())
  {
    if (connected == true)
    {
      drawConnectivitySign();
    }
    else
    {
      clearConnectivitySign();
    }
  }
  connected = audio->client.connected();
  if (!connected)
  {
    audio->ConnectToServer(false);
  }
  audio->Record(&display, firstRun);
  firstRun = false;
}

void func1() {
  ThisThread::sleep_for(5000);
  String str = "";
  bool transcribing = false;
  while (true)
  {
    if (audio->client.available())
    {
      char temp = audio->client.read();
      str += temp;
      transcribing = true;
    }
    else
    {
      if (transcribing == true)
      {
        int start = str.indexOf('{');
        int end = str.lastIndexOf('}') + 1;
        String ans = str.substring(start, end);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, ans);
        if (error) {
          Serial.print("Deserialization error: ");
          Serial.println(error.c_str());
        }

        String result = doc["results"][0]["alternatives"][0]["transcript"];
        Serial.print("Result: ");
        Serial.println(result);
        display.fillScreen(BLACK);
        display.setCursor(0, 0);
        if (result.length() > 35)
        {
          display.setTextSize(1);
        }
        else
        {
          display.setTextSize(2);
        }
        if (result != "null")
        {
          display.print(result);
        }
        connected = audio->client.connected();
        if (!connected) drawConnectivitySign();
        str = "";
        transcribing = false;
      }
      ThisThread::sleep_for(100);
    }
  }
}

void drawConnectivitySign()
{
  display.setCursor(96 - 12, 64 - 15);
  display.setTextColor(BLUE);
  display.print("!");
  display.setTextColor(WHITE);
}

void clearConnectivitySign()
{
  display.fillRect(96 - 12, 64 - 15, 12, 15, BLACK);
}