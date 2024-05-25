#ifndef _AUDIO_H
#define _AUDIO_H

#include <Arduino.h>
#include "WiFiNINA.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>

enum Authentication {
  USE_ACCESSTOKEN,
  USE_APIKEY
};

// 16bit, monoral, 16000Hz,  linear PCM
class Audio {
  static const int headerSize = 44;
  static const int i2sBufferSize = 12000;
  char i2sBuffer[i2sBufferSize];
  void UpdateSampleRate(unsigned long time);

  bool transcribing = false;
  String transcriptionStr = "";

  void PrintHttpBody2();
  void PrintPaddedHeader();
  Authentication authentication;

public:
  static constexpr double amplification = 3.0;
  static const int sampleMult = 8;
  static const int wavDataSize = 45000;                   // It must be multiple of dividedWavDataSize. Recording time is about 1.9 second.
  static const int dividedWavDataSize = i2sBufferSize/4;
  char** wavData;                                         // It's divided. Because large continuous memory area can't be allocated in esp32.
  byte paddedHeader[headerSize + 4] = {0};                // The size must be multiple of 3 for Base64 encoding. Additional byte size must be even because wave data is 16bit.
  long sampleRate = 16000;
  String transcriptionResult = "";
  WiFiSSLClient client;

  Audio(Authentication authentication);
  ~Audio();
  void Record(Adafruit_SSD1331* display, bool firstRun);
  void CreateWavHeader(byte* header, int waveDataSize);
  void PrintHttpHeader();
  void ConnectToServer(bool first);
};

#endif // _AUDIO_H
