# HearTech

This is the code for HearTech, a project for the Academy for Math, Science, and Engineering's PED 3 class. HearTech is a clip-on attachment to glasses that takes in microphone input, transcribes it using the Google Speech-to-Text API, and displays the transcribed text on an OLED. I do not plan on updating this code.

## Components

- Runs on [Arduino Nano RP2040 Connect](https://store.arduino.cc/products/arduino-nano-rp2040-connect)
- Microphone input from an [electret microphone](https://www.sparkfun.com/products/12758) on A0
- Display output to 96x64 [OLED](https://www.amazon.com/HiLetgo-Colorful-Display-SSD1331-Resolution/dp/B0711RKXB5) on D8-D13
- Powered by 3.7V battery on VIN

## How it works

Most of the functionality is handled in the `Audio` class. The Arduino first attempts to connect to WiFi through the `ConnectToServer` function, cycling between the two provided WiFi credentials. The program will attempt to automatically reconnect to WiFi if connection is lost. A blue exclamation mark will appear in the bottom right corner if WiFi is lost.

The `Record` function is responsible for recording audio input and sending it to Google. Audio is stored as [wav data](https://docs.fileformat.com/audio/wav/). Because the sample rate of the Arduino is so fast, 8 samples are taken and averaged for each byte of the wav data. Wav data is stored as a number of arrays as Arduino does not allow storing large amounts of contiguous memory. The recorded wav data is sent to Google through HTTP for processing.

The amount of recording time can be changed by changing the amount of wav data stored, configured by the `wavDataSize` variable. Right now, the program records for approximately 2.6 seconds with a 0.5 second processing time, storing 45000 bytes of data. I have experimented with 5.2 seconds recording with 1.1 second processing (90000 bytes), and 10.4 seconds recording with 2.2 second processing (180000 bytes). 2.6 and 5.2 seem to be the best, but with 5.2 there can sometimes be too much text on the screen.

Receiving the data from Google is done on a separate thread. Despite having two cores, the Arduino RP2040 does not really support utilizing both (see [here](https://www.reddit.com/r/arduino/comments/18jm0f7/how_do_i_make_use_of_second_core_on_arduino_nano/)). Consequently, a second RTOS thread is used to receive data from HTTP.

## Limitations

Due to the single-core nature of the Arduino, there is a small processing time when sending wav data to the server. I experimented with trying to send small amounts of data in between each microphone sample, but this proved ineffective as HTTP was too slow. As a result, there is a small processing time that consists of about 20% of the total runtime of the program during which microphone input is disabled.

The default font size is 2. The OLED [does not support non-integral font sizes](https://forum.arduino.cc/t/tft-oled-adjusting-font-size-between-1-and-2-possible/1013510), so when there are too many characters the font size reduces to 1 and becomes significantly smaller.

## Secrets

Define the following secrets in `arduino_secrets.h`.

- `SECRET_APIKEY` - Google Speech-to-Text API key
- `SECRET_SSID_1` - SSID for primary WiFi network
- `SECRET_PASS_1` - Password for primary WiFi network
- `SECRET_SSID_2` - SSID for secondary WiFi network
- `SECRET_PASS_2` - Password for secondary WiFi network
