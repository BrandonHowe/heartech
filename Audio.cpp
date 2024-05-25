#include "Audio.h"
#include "network_param.h"
#include <ArduinoJson.h>
#include "arduino_base64.hpp"

const int microphonePin = A0;
int credentialNum = 1;

Audio::Audio(Authentication authentication) {
  wavData = new char*[wavDataSize/dividedWavDataSize];
  for (int i = 0; i < wavDataSize/dividedWavDataSize; ++i) wavData[i] = new char[dividedWavDataSize];

  this->authentication = authentication;
  Serial.println("1");
  ConnectToServer(true);
  Serial.println("2");
}

void Audio::ConnectToServer(bool first)
{
  Serial.println("Trying to connect...");
  while (WiFi.status() != WL_CONNECTED)
  {
    auto ssid = credentialNum == 1 ? ssid1 : ssid2;
    auto password = credentialNum == 1 ? password1 : password2;
    Serial.print("Connecting with credential ");
    Serial.println(credentialNum);
    int status = WiFi.begin(ssid, password);
    if (first && status != WL_CONNECTED)
    {
      credentialNum = credentialNum == 1 ? 2 : 1;
    }
    delay(5000);
  }
  if (client.connect(server, 443)) {
    Serial.println("Connection succeeded!");
  } else {
    Serial.println("Connection failed!");
  }
}

Audio::~Audio() {
  client.stop();
  WiFi.disconnect();
  for (int i = 0; i < wavDataSize/dividedWavDataSize; ++i) delete[] wavData[i];
  delete[] wavData;
}

void Audio::CreateWavHeader(byte* header, int waveDataSize){
  header[0] = 'R';
  header[1] = 'I';
  header[2] = 'F';
  header[3] = 'F';
  unsigned int fileSizeMinus8 = waveDataSize + 44 - 8;
  header[4] = (byte)(fileSizeMinus8 & 0xFF);
  header[5] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
  header[6] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
  header[7] = (byte)((fileSizeMinus8 >> 24) & 0xFF);
  header[8] = 'W';
  header[9] = 'A';
  header[10] = 'V';
  header[11] = 'E';
  header[12] = 'f';
  header[13] = 'm';
  header[14] = 't';
  header[15] = ' ';
  header[16] = 0x10;  // linear PCM
  header[17] = 0x00;
  header[18] = 0x00;
  header[19] = 0x00;
  header[20] = 0x01;  // linear PCM
  header[21] = 0x00;
  header[22] = 0x01;  // monoral
  header[23] = 0x00;
  header[24] = 0x80;  // sampling rate 16000
  header[25] = 0x3E;
  header[26] = 0x00;
  header[27] = 0x00;
  header[28] = 0x00;  // Byte/sec = 16000x2x1 = 32000
  header[29] = 0x7D;
  header[30] = 0x00;
  header[31] = 0x00;
  header[32] = 0x02;  // 16bit monoral
  header[33] = 0x00;
  header[34] = 0x10;  // 16bit
  header[35] = 0x00;
  header[36] = 'd';
  header[37] = 'a';
  header[38] = 't';
  header[39] = 'a';
  header[40] = (byte)(waveDataSize & 0xFF);
  header[41] = (byte)((waveDataSize >> 8) & 0xFF);
  header[42] = (byte)((waveDataSize >> 16) & 0xFF);
  header[43] = (byte)((waveDataSize >> 24) & 0xFF);
}

void Audio::PrintHttpBody2()
{
  char encRaw[base64::encodeLength(sizeof(paddedHeader))];
  base64::encode(paddedHeader, sizeof(paddedHeader), encRaw);
  String enc = String(encRaw);
  enc.replace("\n", "");
  client.print(enc);
  auto enc2Length = base64::encodeLength(dividedWavDataSize);
  for (int j = 0; j < wavDataSize / dividedWavDataSize; ++j) {
    char enc2Raw[enc2Length];
    base64::encode((byte*)wavData[j], dividedWavDataSize, enc2Raw);
    String enc2 = String(enc2Raw);
    enc2.replace("\n", "");
    client.print(enc2);
  }
}

void Audio::PrintHttpHeader() {
  String HttpBody1a = "{\"config\":{\"encoding\":\"LINEAR16\",\"model\":\"default\",\"sampleRateHertz\":";
  String HttpBody1c = ",\"languageCode\":\"en-US\"},\"audio\":{\"content\":\"";
  int body1Length = HttpBody1a.length() + (sampleRate > 9999 ? 5 : 4) + HttpBody1c.length();
  String HttpBody3 = "\"}}\r\n\r\n";
  int httpBody2Length = (wavDataSize + sizeof(paddedHeader)) * 4 / 3; // 4/3 is from base64 encoding
  String ContentLength = String(body1Length + httpBody2Length + HttpBody3.length());
  String HttpHeader;
  HttpHeader = String("POST /v1/speech:recognize?key=") + ApiKey
               + String(" HTTP/1.1\r\nHost: speech.googleapis.com\r\nContent-Type: application/json\r\nContent-Length: ") + ContentLength + String("\r\n\r\n");
  String request = HttpHeader + HttpBody1a + sampleRate + HttpBody1c;
  client.print(request);
}

void Audio::PrintPaddedHeader()
{
  char encRaw[base64::encodeLength(sizeof(paddedHeader))];
  base64::encode(paddedHeader, sizeof(paddedHeader), encRaw);
  String enc = String(encRaw);
  enc.replace("\n", "");
  client.print(enc);
}

void Audio::UpdateSampleRate(unsigned long time)
{
  double avgTime = (double)(wavDataSize / 2) / (double)(time);
  sampleRate = (long)((double)1000000 * avgTime);

  paddedHeader[24] = sampleRate % 256;  // sampling rate
  paddedHeader[25] = sampleRate / 256;
}

void Audio::Record(Adafruit_SSD1331* display, bool firstRun) {
  CreateWavHeader(paddedHeader, wavDataSize);
  // if (!firstRun)
  // {
  //   paddedHeader[24] = sampleRate % 256;  // sampling rate
  //   paddedHeader[25] = sampleRate / 256;
  //   PrintHttpHeader();
  // }

  unsigned long start = micros();
  unsigned long end = micros();

  int rowLimit = wavDataSize / dividedWavDataSize;
  int colLimit = i2sBufferSize / 8;
  Serial.println("Recording...");

  byte sendQueue[6] = {0};

  for (int row = 0; row < rowLimit; ++row) {
    for (int col = 0; col < colLimit; ++col) {
      unsigned long sampleStart = micros();
      long res = 0;
      for (int j = 0; j < sampleMult; j++) {
        long val = analogRead(A0);
        val = map(val, 0, 1023, -32767 * amplification, 32767 * amplification);
        res += val;
      }
      res /= sampleMult;
      if (res > 32767) {
        res = 32767;
      } else if (res < -32767) {
        res = -32767;
      }
      wavData[row][2 * col] = (byte)(res % 256);
      wavData[row][2 * col + 1] = (byte)(res / 256);
      sendQueue[(col / 2) % 3] = (byte)(res % 256);
      sendQueue[(col / 2) % 3 + 1] = (byte)(res / 256);
      unsigned long sampleEnd = micros();
    }
  }

  end = micros();
  unsigned long t1 = micros();
  bool connected = client.connected();
  Serial.print("Processing... ");
  Serial.println(connected);
  if (!connected)
  {
    ConnectToServer(false);
  }
  // if (firstRun)
  // {
    UpdateSampleRate(end - start);
  // }
  // paddedHeader[24] = sampleRate % 256;  // sampling rate
  // paddedHeader[25] = sampleRate / 256;
  PrintHttpHeader();
  PrintHttpBody2();

  String HttpBody3 = "\"}}\r\n\r\n";
  client.print(HttpBody3);
  // UpdateSampleRate(end - start);

  unsigned long t2 = micros();
  Serial.print("Processing done: ");
  Serial.print(end - start);
  Serial.print(" - ");
  Serial.print(t2 - t1);
  Serial.print(" - ");
  Serial.print(sampleRate);
  Serial.println("hz");
}