#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

WiFiManager wifiManager;

WiFiManagerParameter modeSetting("mode", "Mode (purdue or... ?)", "", 10);

bool shouldSaveConfig = false;
bool purdueMode = false;

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
const long EST_OFFSET = -5 * 3600;
const long EDT_OFFSET = -4 * 3600;

#define HOUR_OFFSET 12
#define MINUTE_OFFSET 6
#define SECOND_OFFSET 0

int lastHour = 0;
int lastMinute = 0;
uint32_t baseHourColor = 0;
uint32_t baseMinuteColor = 0;

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
  if (purdueMode) {
     return raw + (dst ? EDT_OFFSET : EST_OFFSET);
  }
  else {
     return raw + (dst ? CDT_OFFSET : CST_OFFSET);
  }
}

void inConfigMode(WiFiManager *myWifiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWifiManager->getConfigPortalSSID());

  strip.clear();
  strip.setPixelColor(HOUR_OFFSET, strip.ColorHSV(0, 255, 128));
  strip.setPixelColor(HOUR_OFFSET + 4, strip.ColorHSV(0, 255, 128));
  strip.setPixelColor(MINUTE_OFFSET + 2, strip.ColorHSV(0, 255, 128));
  strip.setPixelColor(MINUTE_OFFSET + 3, strip.ColorHSV(0, 255, 128));
  strip.setPixelColor(SECOND_OFFSET, strip.ColorHSV(0, 255, 128));
  strip.setPixelColor(SECOND_OFFSET + 5, strip.ColorHSV(0, 255, 128));
  strip.show();
}

void saveModeSettingToEEPROM(String modeValue) {
  for (int i = 0; i < modeValue.length(); ++i) {
    EEPROM.write(i, modeValue[i]);
  }
  EEPROM.write(modeValue.length(), '\0'); // null terminator
  EEPROM.commit();
}

String loadModeSettingFromEEPROM() {
  char data[11]; // match size from parameter
  for (int i = 0; i < 10; ++i) {
    data[i] = EEPROM.read(i);
    if (data[i] == '\0') break;
  }
  data[10] = '\0';
  return String(data);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);

  // LED setup
  strip.begin();
  strip.setBrightness(128);
  strip.show();

  //temporary...
  //wifiManager.resetSettings();

  wifiManager.addParameter(&modeSetting);
  wifiManager.setClass("invert");
  wifiManager.setConfigPortalTimeout(300);
  wifiManager.setAPCallback(inConfigMode);
  wifiManager.setSaveConfigCallback([]() {
    Serial.println("WiFiManager config was changed — marking to save custom params");
    shouldSaveConfig = true;
  });


  wifiManager.autoConnect("clockyness");

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(random(65535), 255, 128)));
    }
    strip.show();
    Serial.print(".");
    delay(50);
  } 

  if (shouldSaveConfig) {
     String modeValue = modeSetting.getValue();
     saveModeSettingToEEPROM(modeValue);
     purdueMode = modeValue.indexOf("purdue") >= 0;
  }
  else {
     String modeValue = loadModeSettingFromEEPROM();
     purdueMode = modeValue.indexOf("purdue") >= 0;
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

uint32_t gimmeAColor() {
 //deep aqua blue return strip.gamma32(strip.ColorHSV(hue, 128, 250));
 // seems a bit brighter?  return strip.gamma32(strip.ColorHSV(hue, 255, 250));
 // less bright... much more green. return strip.gamma32(strip.ColorHSV(hue, 255, 128));
 // return strip.gamma32(strip.ColorHSV(hue, 255, 128)); // start here...
// washed out white trash  return strip.gamma32(strip.ColorHSV(9984, 77, 207)); // start here...
// looks yellow - but too bright  return strip.gamma32(strip.ColorHSV(9984, 177, 207));
// awful - looks white return strip.gamma32(strip.ColorHSV(9984, 127, 207));
 // very muted, but interesting. a slight bump of the hue and bright? return strip.gamma32(strip.ColorHSV(9984, 177, 64));
  // good setting... just find the right hue. return strip.gamma32(strip.ColorHSV(9984, 177, 90));
//  return strip.gamma32(strip.ColorHSV(hue, 177, 90));
// best so far:  return strip.gamma32(strip.ColorHSV(8800, 177, 90));

  
  if (purdueMode) {
    return strip.gamma32(strip.ColorHSV(8800, 177, 90));
  }
  else {
    return strip.gamma32(strip.ColorHSV(random(65535), 255, 128));
  }
}




void loop() {
  long loopStart = millis();
  timeClient.update();
  ArduinoOTA.handle();

  // Get local time adjusted for DST
  time_t localTime = getLocalTime();
  struct tm *ptm = localtime(&localTime);
  Serial.print("Holy dinkles, the year is: ");
  Serial.println(ptm->tm_year);

  // years are super weird... they're an offset of the year 1900.   
 if (ptm->tm_year < 100) {
    for (int i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(random(65535), 255, 128)));
    }
    strip.show();
    delay(50);
    return;
  } 

  // Debug print
  char timeStr[32];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", ptm);
  Serial.println(timeStr);

  int hour = ptm->tm_hour;
  if (hour != lastHour) {
     baseHourColor = gimmeAColor();
  }
  for (int i = 0; i < 5; i++) {
    int twoth = toTheTwoth(i);
    if ((hour & twoth) == twoth) {
      //strip.setPixelColor(i + HOUR_OFFSET, strip.gamma32(strip.ColorHSV(random(65535), 255, 128)));
      strip.setPixelColor(i + HOUR_OFFSET, baseHourColor);
    }
    else {
     strip.setPixelColor(i + HOUR_OFFSET, strip.Color(0, 0, 0));
    }
  }

  int minute = ptm->tm_min;
  if (minute != lastMinute) {
     baseMinuteColor = gimmeAColor();
  }
  for (int i = 0; i < 6; i++) {
    int twoth = toTheTwoth(i);
    if ((minute & twoth) == twoth) {
      strip.setPixelColor(5 - i + MINUTE_OFFSET, baseMinuteColor);
      //strip.setPixelColor(5 - i + MINUTE_OFFSET, strip.gamma32(strip.ColorHSV(random(65535), 255, 128)));
      Serial.print(1);
    }
    else {
     strip.setPixelColor(5 - i + MINUTE_OFFSET, strip.Color(0, 0, 0));
      Serial.print(0);
    }
  }
  Serial.println();

  int second = ptm->tm_sec;
  uint32_t secondColor = gimmeAColor();

  for (int i = 5; i >= 0; i--) {
    int twoth = toTheTwoth(i);

    if ((second & twoth) == twoth) {
     strip.setPixelColor(i + SECOND_OFFSET, secondColor);
     //strip.setPixelColor(i + SECOND_OFFSET, strip.Color(0, 0, 255));
    }
    else {
     strip.setPixelColor(i + SECOND_OFFSET, strip.Color(0, 0, 0));
    }
  }

  
  strip.show();
  Serial.println();

  lastHour = hour;
  lastMinute = minute;

  Serial.print("sleeping for: ");
  Serial.print(1000 - (millis() - loopStart));
  Serial.print(" but constrained to: ");
  Serial.println(constrain(1000 - (millis() - loopStart), 10, 1000));

  delay(constrain(1000 - (millis() - loopStart), 10, 1000));  // update every secondish
}

