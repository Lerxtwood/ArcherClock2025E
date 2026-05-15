/******************************************************************************************
 *                               Archer Clock 2025E Firmware                              *
 *----------------------------------------------------------------------------------------*
 * Project:   Clock with hypnotic motion and time synchronization via Wi-Fi and NTP.      *
 * Author:    Archer                                                                      *
 * Date:      25/09/2024                                                                  *
 *----------------------------------------------------------------------------------------*
 * Description:                                                                           *
 * - This clock is inspired by the light effects of the "Corpus Clock" located on the     *
 *   campus of the University of Cambridge.                                               *
 *                                                                                        *
 * Features:                                                                              *
 * -  Hypnotic LED movement of the hands.                                                 *
 * -  Time synchronization via Wi-Fi to NTP server.                                       *
 *                                                                                        *
 ******************************************************************************************/
#include <Arduino.h>
#include <FastLED.h>
#include <WiFi.h>

#include "config.hpp"


//#define DEBUG_NIGHT_MODE

typedef enum {
MODE_DAY = 0,
MODE_NIGHT = 1
} display_mode;

CRGB leds[NUM_LEDS];

uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;
uint8_t syncCountdown = SYNC_TIME;
bool switchModes = false;
display_mode currentMode = MODE_DAY;

// SETTINGS
// Brightness
int globalLEDBrightness = GLOBAL_LED_BRIGHTNESS;
int markerLEDBrightness = MARKER_LED_BRIGHTNESS;

// Animation
bool animateSecondHand = ANIMATE_SECOND_HAND;

// Unremark for show
bool showSecondsMarkers = SHOW_SECONDS_MARCKERS;
bool showMinutesMarkers = SHOW_MINUTES_MARCKERS;
bool showHoursMarkers = SHOW_HOURS_MARCKERS;

// Marckers color
CRGB zeroMarkerColor = ZERO_MARKER_COLOR;
CRGB secondsMarkersColor = SECONDS_MARCKERS_COLOR;
CRGB minutesMarkersColor = MINUTES_MARCKERS_COLOR;
CRGB hoursMarkersColor = HOURS_MARCKERS_COLOR;

// Hands color
CRGB secondsHandColor = SECONDS_HAND_COLOR;
CRGB minutesHandColor = MINUTES_HAND_COLOR;
CRGB hoursHandColor = HOURS_HAND_COLOR;



unsigned long previousMillis = 0;

bool inNightMode() {
  #ifdef ENABLE_NIGHT_MODE

    #ifdef DEBUG_NIGHT_MODE
    Serial.println("Debugging Night Mode");
    int currentHour = hours;
    if (NIGHT_MODE_START_HOUR <= hours || hours < 7) 
    {
      if (currentMode != MODE_NIGHT) 
      {
        currentMode = MODE_NIGHT;
        switchModes = true;
      }
      return true;
    } 
    else 
    {
      if (currentMode != MODE_DAY) 
      {
        currentMode = MODE_DAY;
        switchModes = true;
      }
      return false;
    }
    #else
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return false;
    int currentHour = timeinfo.tm_hour;
    if (NIGHT_MODE_START_HOUR <= timeinfo.tm_hour || timeinfo.tm_hour < 7) 
    {
      if (currentMode != MODE_NIGHT) 
      {
        currentMode = MODE_NIGHT;
        switchModes = true;
      }
      return true;
    } 
    else
    {
      if (currentMode != MODE_DAY) 
      {
        currentMode = MODE_DAY;
        switchModes = true;
      }
      return false;
    }
    #endif
  #else
    return false;
  #endif
}


void draw(CRGB leds[], uint8_t s, uint8_t m, uint8_t h) {
  // Scale factor for markers (0..255). 32 is dim; max is 255 (not 256).
  const uint8_t MARKER_SCALE = markerLEDBrightness;

  // Clear all LEDs if we just switched modes
  if (switchModes) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    switchModes = false;
  }

  // Leave the zero marker at default brightness
  CRGB rgbValue = zeroMarkerColor;
  leds[0] = rgbValue;

  // Draw second markers (dimmed)
  if (showSecondsMarkers) {
    for (int c = 5; c <= 55; c += 5) {
      rgbValue = secondsMarkersColor;
      leds[c] = rgbValue.scale8(MARKER_SCALE);
    }
  }

  // Draw minute markers (dimmed)
  if (showMinutesMarkers) {
    for (int c = 0; c <= 55; c += 5) {
      rgbValue = minutesMarkersColor;
      leds[60 + c] = rgbValue.scale8(MARKER_SCALE);
    }
  }

  // Draw hour markers (dimmed)
  if (showHoursMarkers) {
    for (int c = 0; c < 12; ++c) {
      rgbValue = hoursMarkersColor;
      leds[120 + c] = rgbValue.scale8(MARKER_SCALE);
    }
  }

  // Draw hands at full color (no extra scaling)
  leds[s] = secondsHandColor;
  leds[60 + m] = minutesHandColor;
  leds[120 + h] = hoursHandColor;

  FastLED.show();
}


void syncronize() {
  leds[0] = CRGB::Red;
  FastLED.show();
  delay(500);

  leds[0] = CRGB::Orange;
  FastLED.show();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }

  leds[0] = CRGB::Green;
  FastLED.show();

  // Configuration for synchronization with the NTP server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  
  hours = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
  if (hours>12) hours-=12;
  syncCountdown = SYNC_TIME;

  #ifdef DEBUG_NIGHT_MODE
  hours = 23;
  minutes = 59;
  seconds = 55;
  #endif

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}


void setup() {
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setCorrection(CRGB::White);

  // set global brightness (0..255)
  FastLED.setBrightness(GLOBAL_LED_BRIGHTNESS); // <-- adjust as needed

  // optional: limit total current to ~500mA at 5V
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  syncronize();

  previousMillis = millis();

}

void loop() {

  bool isNightMode = inNightMode();
  if (isNightMode) {
  // Brightness
  globalLEDBrightness = NIGHT_GLOBAL_LED_BRIGHTNESS;
  markerLEDBrightness = NIGHT_MARKER_LED_BRIGHTNESS;

  // Animation
  animateSecondHand = NIGHT_ANIMATE_SECOND_HAND;

  // Unremark for show
  showSecondsMarkers = NIGHT_SHOW_SECONDS_MARCKERS;
  showMinutesMarkers = NIGHT_SHOW_MINUTES_MARCKERS;
  showHoursMarkers = NIGHT_SHOW_HOURS_MARCKERS;

  // Marckers color
  zeroMarkerColor = NIGHT_ZERO_MARKER_COLOR;
  secondsMarkersColor = NIGHT_SECONDS_MARCKERS_COLOR;
  minutesMarkersColor = NIGHT_MINUTES_MARCKERS_COLOR;
  hoursMarkersColor = NIGHT_HOURS_MARCKERS_COLOR;

  // Hands color
  secondsHandColor = NIGHT_SECONDS_HAND_COLOR;
  minutesHandColor = NIGHT_MINUTES_HAND_COLOR;
  hoursHandColor = NIGHT_HOURS_HAND_COLOR;
  } else {
  // Brightness
  globalLEDBrightness = GLOBAL_LED_BRIGHTNESS;
  markerLEDBrightness = MARKER_LED_BRIGHTNESS;

  // Animation
  animateSecondHand = ANIMATE_SECOND_HAND;

  // Unremark for show
  showSecondsMarkers = SHOW_SECONDS_MARCKERS;
  showMinutesMarkers = SHOW_MINUTES_MARCKERS;
  showHoursMarkers = SHOW_HOURS_MARCKERS;

  // Marckers color
  zeroMarkerColor = ZERO_MARKER_COLOR;
  secondsMarkersColor = SECONDS_MARCKERS_COLOR;
  minutesMarkersColor = MINUTES_MARCKERS_COLOR;
  hoursMarkersColor = HOURS_MARCKERS_COLOR;

  // Hands color
  secondsHandColor = SECONDS_HAND_COLOR;
  minutesHandColor = MINUTES_HAND_COLOR;
  hoursHandColor = HOURS_HAND_COLOR;
  }  

  FastLED.setBrightness(globalLEDBrightness);


  unsigned long currentMillis = millis();
  while (currentMillis - previousMillis < 1000) {
    currentMillis = millis();
  }
  previousMillis = currentMillis;


  bool move_minutes = false;
  bool move_hours = false;
  uint8_t steps_hour = -1;
  if (seconds == 59) {
    move_minutes = true;
    if (minutes == 59) {
      move_hours = true;
      syncCountdown--;
    }
  }

  uint8_t acceleration = 20;

  // keep the current seconds LED static during the sweep
  uint8_t oldSeconds = seconds;

  for (int i = 0; i<=60; i++) {

    if (animateSecondHand) 
    {
      seconds++;
    }
    
    // clear old second LED
    if (move_minutes) minutes++;

    if (animateSecondHand) {  
        if (seconds>59) seconds = 0;
    }

    if (minutes>59) minutes = 0;

    if (move_hours) {
      steps_hour++;
      if (i==60) hours++;
    }
    if (steps_hour == 5) {
      hours++;
      steps_hour = 0;
    }
    if (hours>11) hours -= 12;

    unsigned long start_timer = millis();
    draw(leds, seconds, minutes, hours);
    unsigned long stop_timer = millis();
    unsigned long total_time = stop_timer - start_timer;
    Serial.println(total_time);

    delay(acceleration);
    if (acceleration > 3) acceleration--;

    if (animateSecondHand) {
      leds[seconds] = CRGB::Black;
    }

    // do not clear the seconds LED while sweeping - keep it static
    // only clear the moving minute/hour pixels so the sweep is visible
    leds[60+minutes] = CRGB::Black;
    leds[120+hours] = CRGB::Black;
  }

  if (!animateSecondHand) {
    // if not animating the second hand, we need to advance seconds here
  // advance seconds exactly once after the sweep
  leds[oldSeconds] = CRGB::Black; // clear old second LED
  seconds++;
  if (seconds>59) seconds = 0;

  // update display to show the new second tick
  draw(leds, seconds, minutes, hours);
  } 


  if (syncCountdown == 0) syncronize();
}

