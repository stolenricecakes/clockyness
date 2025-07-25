#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
//#define FASTLED_ESP8266_NODEMCU_PIN_ORDER
// works, but freezes #define FASTLED_ESP8266_RAW_PIN_ORDER
// works, but freezes #define FASTLED_ESP8266_D1_PIN_ORDER
#include <FastLED.h>


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
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

// Timezone offset (in seconds)
const long CST_OFFSET = -6 * 3600;
const long CDT_OFFSET = -5 * 3600;

#define HOUR_OFFSET 0
#define MINUTE_OFFSET 5
#define SECOND_OFFSET 11
#define HOUR_DIRECTION 1
#define MINUTE_DIRECTION -1
#define SECOND_DIRECTION 1

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

  // Time setup
  timeClient.begin();

  // LED setup
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.clear();
  FastLED.show();
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

  // Get local time adjusted for DST
  time_t localTime = getLocalTime();
  struct tm *ptm = localtime(&localTime);

  // Debug print
  char timeStr[32];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", ptm);
  Serial.println(timeStr);

  // Example LED pattern: light up based on seconds
  /*
  int hour = ptm->tm_hour;
  for (int i = 0; i < 5; i++) {
    int twoth = toTheTwoth(i);
    if (hour & twoth == twoth) {
      leds[i + HOUR_OFFSET] = CHSV(random8(), 255, 100);
    }
    else {
      leds[i + HOUR_OFFSET] = CRGB::Black;
    }
  }

  int minute = ptm->tm_min;
  for (int i = 0; i < 6; i++) {
    int twoth = toTheTwoth(i);
    if (minute & twoth == twoth) {
      leds[5 - i + MINUTE_OFFSET] = CHSV(random8(), 255, 100);
    }
    else {
      leds[5 - i + MINUTE_OFFSET] = CRGB::Black;
    }
  }
  */


  int second = ptm->tm_sec;
  for (int i = 5; i >= 0; i--) {
    int twoth = toTheTwoth(i);
/*    Serial.print(second);
    Serial.print(" & ");
    Serial.print(twoth);
    Serial.print(" === ");
    Serial.print(second & twoth);
    Serial.print(" *** ");*/

    if ((second & twoth) == twoth) {
     // leds[i + SECOND_OFFSET] = CHSV(random8(), 255, 100);
      leds[i + SECOND_OFFSET] = CRGB::Green;
      Serial.print(1);
    }
    else {
      leds[i + SECOND_OFFSET] = CRGB::Black;
      Serial.print(0);
    }
  }
  FastLED.show();
  Serial.println();

  delay(1000);  // update every second
}

