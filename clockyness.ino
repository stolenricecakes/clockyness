#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>


#include "/home/awall/ESP8266/accesspoint.h"
// start of accessinfo.h contents ----------------------------
#ifndef AP_INFO_H
  #define AP_INFO_H
  #define AP_SSID	"your_wifi_router_SSID_here"
  #define AP_PASSWORD "your_accesspoint_password"
#endif // AP_INFO_H

// NTP client setup
WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);  // UTC, update every 60 seconds
NTPClient timeClient(ntpUDP, "us.pool.ntp.org", 0, 3600000);  // UTC, update every hour

// LED setup
#define LED_PIN     D5
#define NUM_LEDS    17

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Timezone offset (in seconds)
const long CST_OFFSET = -6 * 3600;
const long CDT_OFFSET = -5 * 3600;

#define HOUR_OFFSET 12
#define MINUTE_OFFSET 6
#define SECOND_OFFSET 0

// Check if DST is in effect (basic U.S. rule)
bool isDST(int dayOfWeek, int month, int day, int hour) {
  // Only March to November needs checking
  if (month < 3 || month > 11) return false;
  if (month > 3 && month < 11) return true;

  if (month == 3) {
    // DST starts on the second Sunday in March
    int secondSunday = (14 - (1 + 6 - (day - 1) % 7) % 7);
    return (day > secondSunday || (day == secondSunday && hour >= 2));
  }
  if (month == 11) {
    // DST ends on the first Sunday in November
    int firstSunday = 7 - (1 + 6 - (day - 1) % 7) % 7;
    return (day < firstSunday || (day == firstSunday && hour < 2));
  }
  return false;
}

// Convert UNIX time to local time with DST
time_t getLocalTime() {
  time_t raw = timeClient.getEpochTime();
  struct tm *ptm = gmtime(&raw);

  bool dst = isDST(ptm->tm_wday, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour);
  return raw + (dst ? CDT_OFFSET : CST_OFFSET);
}

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(AP_SSID, AP_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  ArduinoOTA.setHostname("clockyness");

  ArduinoOTA.onStart([]() {
    Serial.println("Start OTA update");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA update complete!");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  // Time setup
  timeClient.begin();

  // LED setup
  strip.begin();
  strip.setBrightness(128);
  strip.show();

  ArduinoOTA.begin();
  Serial.println("OTA all ready to go, mofo.");

  randomSeed(analogRead(0));
}

int toTheTwoth(int val) {
  int result = 1;
  for (int i = 0; i < val; i++) {
    result *= 2;
  }
  return result;
}

void loop() {
  timeClient.update();
  ArduinoOTA.handle();

  // Get local time adjusted for DST
  time_t localTime = getLocalTime();
  struct tm *ptm = localtime(&localTime);

  // Debug print
  char timeStr[32];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", ptm);
  Serial.println(timeStr);

  int hour = ptm->tm_hour;
  for (int i = 0; i < 5; i++) {
    int twoth = toTheTwoth(i);
    if ((hour & twoth) == twoth) {
      strip.setPixelColor(i + HOUR_OFFSET, strip.gamma32(strip.ColorHSV(random(65535), 255, 128)));
      //strip.setPixelColor(i + HOUR_OFFSET, strip.Color(255, 0, 0));
    }
    else {
     strip.setPixelColor(i + HOUR_OFFSET, strip.Color(0, 0, 0));
    }
  }

  int minute = ptm->tm_min;
  Serial.printf("minute is: %d\n", minute);
  for (int i = 0; i < 6; i++) {
    int twoth = toTheTwoth(i);
    if ((minute & twoth) == twoth) {
      strip.setPixelColor(5 - i + MINUTE_OFFSET, strip.gamma32(strip.ColorHSV(random(65535), 255, 128)));
      //strip.setPixelColor(5 - i + MINUTE_OFFSET, strip.Color(0, 255, 0));
      Serial.print(1);
    }
    else {
     strip.setPixelColor(5 - i + MINUTE_OFFSET, strip.Color(0, 0, 0));
      Serial.print(0);
    }
  }
  Serial.println();

  int second = ptm->tm_sec;
  for (int i = 5; i >= 0; i--) {
    int twoth = toTheTwoth(i);

    if ((second & twoth) == twoth) {
     strip.setPixelColor(i + SECOND_OFFSET, strip.gamma32(strip.ColorHSV(random(65535), 255, 128)));
     //strip.setPixelColor(i + SECOND_OFFSET, strip.Color(0, 0, 255));
    }
    else {
     strip.setPixelColor(i + SECOND_OFFSET, strip.Color(0, 0, 0));
    }
  }
  
  strip.show();
  Serial.println();

  delay(1000);  // update every second
}

