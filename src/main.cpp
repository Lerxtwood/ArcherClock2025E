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
#include "config.hpp"

#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <FastLED.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>


typedef enum {
MODE_DAY = 0,
MODE_SCHEDULED = 1
} display_mode;

CRGB leds[NUM_LEDS];
Preferences preferences;
WebServer configServer(80);
DNSServer dnsServer;

uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;
uint8_t trailSeconds = 0;
uint8_t trailMinutes = 0;
uint8_t trailHours = 0;
uint8_t syncCountdown = SYNC_TIME;
bool switchModes = false;
display_mode currentMode = MODE_DAY;
bool configPortalActive = false;
bool webServerStarted = false;
#ifdef ENABLE_OTA
bool otaStarted = false;
#endif

bool scheduledThemeEnabled = SCHEDULED_THEME_ENABLED;
uint8_t scheduledThemeStartHour = SCHEDULED_THEME_START_HOUR;
uint8_t scheduledThemeEndHour = SCHEDULED_THEME_END_HOUR;
String scheduledThemeId = SCHEDULED_THEME_ID;
String configuredThemeId;
uint8_t configuredGlobalLEDBrightness = GLOBAL_LED_BRIGHTNESS;
uint8_t configuredMarkerLEDBrightness = MARKER_LED_BRIGHTNESS;
bool configuredAnimateSecondHand = ANIMATE_SECOND_HAND;
bool configuredShowSecondsMarkers = SHOW_SECONDS_MARCKERS;
bool configuredShowMinutesMarkers = SHOW_MINUTES_MARCKERS;
bool configuredShowHoursMarkers = SHOW_HOURS_MARCKERS;
CRGB configuredZeroMarkerColor = ZERO_MARKER_COLOR;
CRGB configuredSecondsMarkersColor = SECONDS_MARCKERS_COLOR;
CRGB configuredMinutesMarkersColor = MINUTES_MARCKERS_COLOR;
CRGB configuredHoursMarkersColor = HOURS_MARCKERS_COLOR;
CRGB configuredSecondsHandColor = SECONDS_HAND_COLOR;
CRGB configuredMinutesHandColor = MINUTES_HAND_COLOR;
CRGB configuredHoursHandColor = HOURS_HAND_COLOR;
bool configuredTimeTrailEnabled = TIME_TRAIL_ENABLED;
CRGB configuredTimeTrailSecondsColor = TIME_TRAIL_SECONDS_COLOR;
CRGB configuredTimeTrailMinutesColor = TIME_TRAIL_MINUTES_COLOR;
CRGB configuredTimeTrailHoursColor = TIME_TRAIL_HOURS_COLOR;
bool timeTrailEnabled = TIME_TRAIL_ENABLED;
CRGB timeTrailSecondsColor = TIME_TRAIL_SECONDS_COLOR;
CRGB timeTrailMinutesColor = TIME_TRAIL_MINUTES_COLOR;
CRGB timeTrailHoursColor = TIME_TRAIL_HOURS_COLOR;

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

struct NamedColor {
  const char* name;
  uint32_t value;
};

struct ThemePreset {
  const char* id;
  const char* name;
  bool showHours;
  bool showMinutes;
  bool showSeconds;
  bool animateSeconds;
  bool timeTrail;
  uint32_t zeroMarker;
  uint32_t secondsMarkers;
  uint32_t minutesMarkers;
  uint32_t hoursMarkers;
  uint32_t secondsHand;
  uint32_t minutesHand;
  uint32_t hoursHand;
  uint32_t timeTrailSeconds;
  uint32_t timeTrailMinutes;
  uint32_t timeTrailHours;
  uint8_t globalBrightness;
  uint8_t markerBrightness;
};

const NamedColor LED_COLOR_OPTIONS[] = {
  {"Red", 0xFF0000},
  {"Orange", 0xFF8000},
  {"Amber", 0xFFBF00},
  {"Yellow", 0xFFFF00},
  {"Gold", 0xFFD700},
  {"Green", 0x00FF00},
  {"Cyan", 0x00FFFF},
  {"Blue", 0x0000FF},
  {"Purple", 0x8000FF},
  {"Magenta", 0xFF00FF},
  {"Pink", 0xFF66CC},
  {"Warm White", 0xFFC080},
  {"White", 0xFFFFFF}
};

ThemePreset THEME_PRESETS[] = {
  {"classicCorpus", "Classic Corpus", true, true, true, true, false, 0xFF0000, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFD700, 0xFFD700, 0xFFD700, 0x00FF00, 0x00FFFF, 0x8000FF, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"solarFlare", "Solar Flare", true, true, true, true, true, 0xFF0000, 0xFF8000, 0xFFBF00, 0xFF8000, 0xFFD700, 0xFFD700, 0xFFD700, 0xFFFF00, 0xFF8000, 0xFF0000, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"deepSpace", "Deep Space", true, true, true, true, true, 0xFFFFFF, 0x0000FF, 0x0000FF, 0x8000FF, 0x00FFFF, 0x00FFFF, 0x00FFFF, 0x0000FF, 0x8000FF, 0x00FFFF, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"aurora", "Aurora", true, true, true, true, true, 0x00FF00, 0x00FFFF, 0x00FFFF, 0x0000FF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0x00FF00, 0x00FFFF, 0x0000FF, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"synthwave", "Synthwave", true, true, true, true, true, 0xFF00FF, 0x8000FF, 0x8000FF, 0xFF00FF, 0x00FFFF, 0x00FFFF, 0x00FFFF, 0xFF66CC, 0xFF00FF, 0x0000FF, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"ember", "Ember", true, true, false, true, true, 0xFFBF00, 0xFF0000, 0xFF8000, 0xFF0000, 0xFF8000, 0xFF8000, 0xFF8000, 0xFF0000, 0xFF8000, 0xFFD700, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"iceClock", "Ice Clock", true, true, true, true, true, 0x00FFFF, 0x0000FF, 0x0000FF, 0x00FFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0x00FFFF, 0x0000FF, 0xFFFFFF, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"minimal", "Minimal", true, false, false, false, false, 0xFF0000, 0x0000FF, 0x0000FF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0x00FF00, 0x00FFFF, 0x8000FF, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"trailblazer", "Trailblazer", true, true, true, true, true, 0xFFFFFF, 0x0000FF, 0x0000FF, 0x0000FF, 0xFFD700, 0xFFD700, 0xFFD700, 0x00FF00, 0x00FFFF, 0x8000FF, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"classicNight", "Classic Night", false, false, true, false, false, 0xFF8000, 0xFF0000, 0xFF0000, 0xFF0000, 0x0000FF, 0x0000FF, 0x0000FF, 0x00FF00, 0x00FFFF, 0x8000FF, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS},
  {"stealthNight", "Stealth Night", false, false, true, false, true, 0xFF8000, 0xFF0000, 0x0000FF, 0x8000FF, 0x0000FF, 0x0000FF, 0x0000FF, 0x8000FF, 0x0000FF, 0x8000FF, GLOBAL_LED_BRIGHTNESS, MARKER_LED_BRIGHTNESS}
};

String deviceSuffix() {
  uint64_t chipId = ESP.getEfuseMac();
  char suffix[7];
  snprintf(suffix, sizeof(suffix), "%06X", static_cast<uint32_t>(chipId & 0xFFFFFF));
  return String(suffix);
}

void setStatusLed(CRGB color) {
  leds[0] = color;
  FastLED.show();
}

String htmlEscape(const String& value) {
  String escaped = value;
  escaped.replace("&", "&amp;");
  escaped.replace("<", "&lt;");
  escaped.replace(">", "&gt;");
  escaped.replace("\"", "&quot;");
  return escaped;
}

uint32_t crgbToUint(CRGB color) {
  return (static_cast<uint32_t>(color.r) << 16) |
         (static_cast<uint32_t>(color.g) << 8) |
         static_cast<uint32_t>(color.b);
}

CRGB uintToCrgb(uint32_t value) {
  return CRGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}

String crgbToHex(CRGB color) {
  char hexColor[8];
  snprintf(hexColor, sizeof(hexColor), "#%02X%02X%02X", color.r, color.g, color.b);
  return String(hexColor);
}

String uintToHex(uint32_t value) {
  char hexColor[8];
  snprintf(hexColor, sizeof(hexColor), "#%06X", static_cast<unsigned int>(value & 0xFFFFFF));
  return String(hexColor);
}

bool parseHexColor(const String& rawValue, CRGB& color) {
  String value = rawValue;
  value.trim();
  if (value.startsWith("#")) {
    value.remove(0, 1);
  }

  if (value.length() != 6) {
    return false;
  }

  char* end = nullptr;
  uint32_t parsed = strtoul(value.c_str(), &end, 16);
  if (end == nullptr || *end != '\0') {
    return false;
  }

  color = uintToCrgb(parsed);
  return true;
}

uint32_t closestPaletteColor(CRGB color) {
  uint32_t bestValue = LED_COLOR_OPTIONS[0].value;
  uint32_t bestDistance = UINT32_MAX;

  for (const NamedColor& option : LED_COLOR_OPTIONS) {
    CRGB optionColor = uintToCrgb(option.value);
    int32_t redDelta = static_cast<int32_t>(color.r) - optionColor.r;
    int32_t greenDelta = static_cast<int32_t>(color.g) - optionColor.g;
    int32_t blueDelta = static_cast<int32_t>(color.b) - optionColor.b;
    uint32_t distance = redDelta * redDelta + greenDelta * greenDelta + blueDelta * blueDelta;

    if (distance < bestDistance) {
      bestDistance = distance;
      bestValue = option.value;
    }
  }

  return bestValue;
}

void appendColorSelect(String& page, const char* id, const char* label, CRGB currentColor) {
  uint32_t selectedValue = closestPaletteColor(currentColor);
  String selectedHex = uintToHex(selectedValue);

  page += F("<div class='color-field'><label for='");
  page += id;
  page += F("'>");
  page += label;
  page += F("</label><div class='color-row'><span class='swatch' style='background:");
  page += selectedHex;
  page += F("'></span><select id='");
  page += id;
  page += F("' name='");
  page += id;
  page += F("'>");

  for (const NamedColor& option : LED_COLOR_OPTIONS) {
    String optionHex = uintToHex(option.value);
    page += F("<option value='");
    page += optionHex;
    page += F("'");
    if (option.value == selectedValue) {
      page += F(" selected");
    }
    page += F(">");
    page += option.name;
    page += F("</option>");
  }

  page += F("</select></div></div>");
}

ThemePreset* findThemePreset(const String& themeId) {
  for (ThemePreset& theme : THEME_PRESETS) {
    if (themeId == theme.id) {
      return &theme;
    }
  }

  return nullptr;
}

void appendThemeOptions(String& page, const String& selectedThemeId) {
  for (const ThemePreset& theme : THEME_PRESETS) {
    page += F("<option value='");
    page += theme.id;
    page += F("'");
    if (selectedThemeId == theme.id) {
      page += F(" selected");
    }
    page += F(">");
    page += theme.name;
    page += F("</option>");
  }
}

void applyThemePreset(const ThemePreset& theme) {
  globalLEDBrightness = theme.globalBrightness;
  markerLEDBrightness = theme.markerBrightness;
  showHoursMarkers = theme.showHours;
  showMinutesMarkers = theme.showMinutes;
  showSecondsMarkers = theme.showSeconds;
  animateSecondHand = theme.animateSeconds;
  timeTrailEnabled = theme.timeTrail;
  zeroMarkerColor = uintToCrgb(theme.zeroMarker);
  secondsMarkersColor = uintToCrgb(theme.secondsMarkers);
  minutesMarkersColor = uintToCrgb(theme.minutesMarkers);
  hoursMarkersColor = uintToCrgb(theme.hoursMarkers);
  secondsHandColor = uintToCrgb(theme.secondsHand);
  minutesHandColor = uintToCrgb(theme.minutesHand);
  hoursHandColor = uintToCrgb(theme.hoursHand);
  timeTrailSecondsColor = uintToCrgb(theme.timeTrailSeconds);
  timeTrailMinutesColor = uintToCrgb(theme.timeTrailMinutes);
  timeTrailHoursColor = uintToCrgb(theme.timeTrailHours);
}

bool loadWifiCredentials(String& storedSsid, String& storedPassword) {
  preferences.begin("wifi", true);
  storedSsid = preferences.getString("ssid", "");
  storedPassword = preferences.getString("password", "");
  preferences.end();
  return storedSsid.length() > 0;
}

void saveWifiCredentials(const String& newSsid, const String& newPassword) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", newSsid);
  preferences.putString("password", newPassword);
  preferences.end();
}

void clearWifiCredentials() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
}

void loadDisplaySettings() {
  // Theme IDs are stable NVS keys; pack both brightness bytes into one value.
  preferences.begin("brightness", true);
  for (ThemePreset& theme : THEME_PRESETS) {
    uint16_t defaults = theme.globalBrightness | (static_cast<uint16_t>(theme.markerBrightness) << 8);
    uint16_t saved = preferences.getUShort(theme.id, defaults);
    theme.globalBrightness = saved & 0xFF;
    theme.markerBrightness = saved >> 8;
  }
  preferences.end();

  preferences.begin("display", true);
  configuredThemeId = preferences.getString("theme", "");
  configuredGlobalLEDBrightness = preferences.getUChar("globalBright", GLOBAL_LED_BRIGHTNESS);
  configuredMarkerLEDBrightness = preferences.getUChar("markerBright", MARKER_LED_BRIGHTNESS);
  scheduledThemeEnabled = preferences.getBool("schedEnabled", scheduledThemeEnabled);
  scheduledThemeStartHour = preferences.getUChar("schedStart", scheduledThemeStartHour);
  scheduledThemeEndHour = preferences.getUChar("schedEnd", scheduledThemeEndHour);
  scheduledThemeId = preferences.getString("schedTheme", scheduledThemeId);
  configuredAnimateSecondHand = preferences.getBool("animateSec", configuredAnimateSecondHand);
  configuredShowSecondsMarkers = preferences.getBool("showSecMark", configuredShowSecondsMarkers);
  configuredShowMinutesMarkers = preferences.getBool("showMinMark", configuredShowMinutesMarkers);
  configuredShowHoursMarkers = preferences.getBool("showHourMark", configuredShowHoursMarkers);
  configuredZeroMarkerColor = uintToCrgb(preferences.getUInt("zeroColor", crgbToUint(configuredZeroMarkerColor)));
  configuredSecondsMarkersColor = uintToCrgb(preferences.getUInt("secMarkColor", crgbToUint(configuredSecondsMarkersColor)));
  configuredMinutesMarkersColor = uintToCrgb(preferences.getUInt("minMarkColor", crgbToUint(configuredMinutesMarkersColor)));
  configuredHoursMarkersColor = uintToCrgb(preferences.getUInt("hourMarkColor", crgbToUint(configuredHoursMarkersColor)));
  configuredSecondsHandColor = uintToCrgb(preferences.getUInt("secHandColor", crgbToUint(configuredSecondsHandColor)));
  configuredMinutesHandColor = uintToCrgb(preferences.getUInt("minHandColor", crgbToUint(configuredMinutesHandColor)));
  configuredHoursHandColor = uintToCrgb(preferences.getUInt("hourHandColor", crgbToUint(configuredHoursHandColor)));
  configuredTimeTrailEnabled = preferences.getBool("trailEnabled", configuredTimeTrailEnabled);
  configuredTimeTrailSecondsColor = uintToCrgb(preferences.getUInt("trailSecColor", crgbToUint(configuredTimeTrailSecondsColor)));
  configuredTimeTrailMinutesColor = uintToCrgb(preferences.getUInt("trailMinColor", crgbToUint(configuredTimeTrailMinutesColor)));
  configuredTimeTrailHoursColor = uintToCrgb(preferences.getUInt("trailHourColor", crgbToUint(configuredTimeTrailHoursColor)));
  preferences.end();

  if (findThemePreset(configuredThemeId) == nullptr) {
    configuredThemeId = "";
  }
  if (scheduledThemeStartHour > 23) {
    scheduledThemeStartHour = SCHEDULED_THEME_START_HOUR;
  }
  if (scheduledThemeEndHour > 23) {
    scheduledThemeEndHour = SCHEDULED_THEME_END_HOUR;
  }
  if (findThemePreset(scheduledThemeId) == nullptr) {
    scheduledThemeId = SCHEDULED_THEME_ID;
  }
}

void saveDisplaySettings(
  const String& newThemeId,
  uint8_t newGlobalBrightness,
  uint8_t newMarkerBrightness,
  bool newScheduledThemeEnabled,
  uint8_t newScheduledThemeStartHour,
  uint8_t newScheduledThemeEndHour,
  const String& newScheduledThemeId,
  bool newAnimateSecondHand,
  bool newShowSecondsMarkers,
  bool newShowMinutesMarkers,
  bool newShowHoursMarkers,
  CRGB newZeroMarkerColor,
  CRGB newSecondsMarkersColor,
  CRGB newMinutesMarkersColor,
  CRGB newHoursMarkersColor,
  CRGB newSecondsHandColor,
  CRGB newMinutesHandColor,
  CRGB newHoursHandColor,
  bool newTimeTrailEnabled,
  CRGB newTimeTrailSecondsColor,
  CRGB newTimeTrailMinutesColor,
  CRGB newTimeTrailHoursColor
) {
  ThemePreset* theme = findThemePreset(newThemeId);
  if (theme != nullptr) {
    preferences.begin("brightness", false);
    preferences.putUShort(theme->id, newGlobalBrightness | (static_cast<uint16_t>(newMarkerBrightness) << 8));
    preferences.end();
    theme->globalBrightness = newGlobalBrightness;
    theme->markerBrightness = newMarkerBrightness;
  }

  preferences.begin("display", false);
  preferences.putString("theme", newThemeId);
  preferences.putUChar("globalBright", newGlobalBrightness);
  preferences.putUChar("markerBright", newMarkerBrightness);
  preferences.putBool("schedEnabled", newScheduledThemeEnabled);
  preferences.putUChar("schedStart", newScheduledThemeStartHour);
  preferences.putUChar("schedEnd", newScheduledThemeEndHour);
  preferences.putString("schedTheme", newScheduledThemeId);
  preferences.putBool("animateSec", newAnimateSecondHand);
  preferences.putBool("showSecMark", newShowSecondsMarkers);
  preferences.putBool("showMinMark", newShowMinutesMarkers);
  preferences.putBool("showHourMark", newShowHoursMarkers);
  preferences.putUInt("zeroColor", crgbToUint(newZeroMarkerColor));
  preferences.putUInt("secMarkColor", crgbToUint(newSecondsMarkersColor));
  preferences.putUInt("minMarkColor", crgbToUint(newMinutesMarkersColor));
  preferences.putUInt("hourMarkColor", crgbToUint(newHoursMarkersColor));
  preferences.putUInt("secHandColor", crgbToUint(newSecondsHandColor));
  preferences.putUInt("minHandColor", crgbToUint(newMinutesHandColor));
  preferences.putUInt("hourHandColor", crgbToUint(newHoursHandColor));
  preferences.putBool("trailEnabled", newTimeTrailEnabled);
  preferences.putUInt("trailSecColor", crgbToUint(newTimeTrailSecondsColor));
  preferences.putUInt("trailMinColor", crgbToUint(newTimeTrailMinutesColor));
  preferences.putUInt("trailHourColor", crgbToUint(newTimeTrailHoursColor));
  preferences.end();

  configuredThemeId = newThemeId;
  configuredGlobalLEDBrightness = newGlobalBrightness;
  configuredMarkerLEDBrightness = newMarkerBrightness;
  scheduledThemeEnabled = newScheduledThemeEnabled;
  scheduledThemeStartHour = newScheduledThemeStartHour;
  scheduledThemeEndHour = newScheduledThemeEndHour;
  scheduledThemeId = newScheduledThemeId;
  configuredAnimateSecondHand = newAnimateSecondHand;
  configuredShowSecondsMarkers = newShowSecondsMarkers;
  configuredShowMinutesMarkers = newShowMinutesMarkers;
  configuredShowHoursMarkers = newShowHoursMarkers;
  configuredZeroMarkerColor = newZeroMarkerColor;
  configuredSecondsMarkersColor = newSecondsMarkersColor;
  configuredMinutesMarkersColor = newMinutesMarkersColor;
  configuredHoursMarkersColor = newHoursMarkersColor;
  configuredSecondsHandColor = newSecondsHandColor;
  configuredMinutesHandColor = newMinutesHandColor;
  configuredHoursHandColor = newHoursHandColor;
  configuredTimeTrailEnabled = newTimeTrailEnabled;
  configuredTimeTrailSecondsColor = newTimeTrailSecondsColor;
  configuredTimeTrailMinutesColor = newTimeTrailMinutesColor;
  configuredTimeTrailHoursColor = newTimeTrailHoursColor;
  switchModes = true;
}

void appendPageStyles(String& page) {
  page += F("html{min-height:100%;background:#000}body{font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,Helvetica,Arial,sans-serif;margin:0;min-height:100%;color:#f5f5f7;background:linear-gradient(180deg,#161617 0%,#050505 55%,#000 100%)}");
  page += F("*{box-sizing:border-box}.page{position:relative;max-width:760px;margin:0 auto;padding:30px 18px 48px}");
  page += F("h1,h2,.eyebrow{font-family:-apple-system,BlinkMacSystemFont,Segoe UI,Roboto,Helvetica,Arial,sans-serif;letter-spacing:0;color:#f5f5f7;text-transform:uppercase}");
  page += F("h1{position:relative;flex:0 0 100%;order:3;margin:0;padding-top:22px;font-size:28px;line-height:1.08;font-weight:850}");
  page += F("h1:before{content:\"\";position:absolute;left:0;right:0;top:0;height:1px;background:linear-gradient(90deg,transparent,rgba(255,255,255,.18),transparent)}");
  page += F("h2{margin:0 0 16px;font-size:20px;line-height:1.12;font-weight:850}.eyebrow{display:inline-block;order:1;margin:0;padding:6px 12px;border:1px solid rgba(255,255,255,.16);border-radius:999px;background:rgba(255,255,255,.08);box-shadow:0 8px 24px rgba(0,0,0,.24);font-size:32px;line-height:1;font-weight:900;text-decoration:none;backdrop-filter:blur(18px)}");
  page += F(".eyebrow:hover,.eyebrow:focus{border-color:#2997ff;background:rgba(0,113,227,.18);outline:none}.top{display:flex;flex-wrap:wrap;justify-content:space-between;gap:18px;align-items:flex-start;margin:8px 0 22px}.top>div{display:contents}");
  page += F("nav{display:flex;order:2;gap:8px;flex-wrap:wrap;padding:5px;background:rgba(36,36,38,.72);border:1px solid rgba(255,255,255,.12);border-radius:999px;box-shadow:0 12px 32px rgba(0,0,0,.34);backdrop-filter:blur(18px)}");
  page += F("nav a{padding:8px 13px;border-radius:999px;color:#d2d2d7;text-decoration:none;font-size:14px;font-weight:700}nav a.active,nav a:hover{color:#fff;background:#0071e3}");
  page += F(".panel,.status,.msg{background:rgba(28,28,30,.78);border:1px solid rgba(255,255,255,.12);border-radius:8px;box-shadow:0 18px 42px rgba(0,0,0,.42);backdrop-filter:blur(20px)}.panel{padding:22px;margin-top:18px}.status,.msg{padding:13px 15px;color:#a1a1a6;line-height:1.45}");
  page += F(".msg{color:#f5f5f7;border-color:rgba(48,209,88,.42);background:rgba(48,209,88,.14)}p{line-height:1.48}.note{color:#a1a1a6}.hint{display:grid;gap:5px;padding:13px;background:rgba(44,44,46,.82);border:1px solid rgba(255,255,255,.10);border-radius:8px;color:#a1a1a6}.hint strong{color:#f5f5f7}");
  page += F("label{display:block;margin-top:15px;color:#f5f5f7;font-weight:700;font-size:14px}input,select{width:100%;padding:12px 13px;margin-top:7px;font-size:16px;color:#f5f5f7;background:rgba(44,44,46,.9);border:1px solid rgba(255,255,255,.16);border-radius:8px;outline:none}");
  page += F("input:focus,select:focus{border-color:#2997ff;box-shadow:0 0 0 4px rgba(41,151,255,.18)}input[type=checkbox]{width:auto;margin:0 9px 0 0;accent-color:#0071e3}.check{display:flex;align-items:center;gap:0}.inline{display:grid;grid-template-columns:1fr 1fr;gap:12px}.inline label{min-width:0}");
  page += F(".color-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:0 12px}.color-field{min-width:0}.color-row{display:grid;grid-template-columns:34px 1fr;gap:10px;align-items:center}.swatch{display:block;width:30px;height:30px;border-radius:8px;border:1px solid rgba(255,255,255,.24);box-shadow:inset 0 0 0 1px rgba(0,0,0,.35)}");
  page += F("button,a.button{display:inline-block;border:1px solid rgba(255,255,255,.16);border-radius:999px;background:rgba(44,44,46,.9);color:#f5f5f7;padding:11px 15px;font-size:15px;font-weight:800;text-decoration:none;cursor:pointer;box-shadow:0 8px 20px rgba(0,0,0,.28)}button:hover,a.button:hover{border-color:#2997ff;color:#2997ff}button:disabled{opacity:.45;cursor:not-allowed}button:disabled:hover{border-color:rgba(255,255,255,.16);color:#f5f5f7}");
  page += F(".primary{width:100%;margin-top:20px;background:#0071e3;border-color:#0071e3;color:white}.primary:hover{background:#0077ed;color:white}.actions{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-top:14px}.actions.single{grid-template-columns:1fr}.actions .button{width:100%;text-align:center}");
  page += F(".uploadBox{position:relative;display:block;padding:28px 20px;margin-top:14px;text-align:center;background:rgba(44,44,46,.82);border:1px dashed rgba(255,255,255,.24);border-radius:8px;cursor:pointer}.uploadBox:hover,.uploadBox:focus-within{border-color:#2997ff;background:rgba(0,113,227,.12)}.uploadBox span{display:block;color:#f5f5f7;font-size:18px;font-weight:800}.uploadBox small{display:block;margin-top:5px;color:#a1a1a6;font-size:12px;font-weight:800;text-transform:uppercase}.uploadBox input{position:absolute;inset:0;width:100%;height:100%;margin:0;opacity:0;cursor:pointer}.fileStatus{margin-top:10px;color:#a1a1a6;font-size:13px;font-weight:800;overflow-wrap:anywhere}.fileStatus.ready{color:#30d158}.fileStatus.invalid{color:#ff453a}");
  page += F("progress{width:100%;height:24px;margin-top:18px;accent-color:#0071e3}.progress-text{font-weight:800;margin-top:8px;color:#2997ff}code{color:#2997ff}script{display:none}@media(max-width:620px){.page{padding:22px 14px 36px}h1{font-size:25px}.eyebrow{font-size:28px}nav{justify-content:center}.panel{padding:18px}.inline,.color-grid,.actions{grid-template-columns:1fr}}");
}

void appendPageHeader(String& page, const char* title, const char* activePath) {
  page += F("<header class='top'><div><a class='eyebrow' href='/'>Archer Clock</a><h1>");
  page += title;
  page += F("</h1></div><nav><a");
  if (strcmp(activePath, "/") == 0) {
    page += F(" class='active'");
  }
  page += F(" href='/'>Settings</a><a");
  if (strcmp(activePath, "/firmware") == 0) {
    page += F(" class='active'");
  }
  page += F(" href='/firmware'>Firmware</a></nav></header>");
}

String configPage(const String& message = "") {
  String page;
  page.reserve(7600);
  page += F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<title>Archer Clock</title><style>");
  appendPageStyles(page);
  page += F("</style></head><body><main class='page'>");
  appendPageHeader(page, "Settings", "/");
  if (message.length() > 0) {
    page += F("<p class='msg'>");
    page += htmlEscape(message);
    page += F("</p>");
  }
  page += F("<p class='status'>Setup AP: ");
  String apSsid = WiFi.softAPSSID();
  page += htmlEscape(apSsid.length() > 0 ? apSsid : String("not active"));
  page += F("<br>Address: ");
  page += htmlEscape(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
  page += F("<br>Device: ");
  page += htmlEscape(String(OTA_HOSTNAME) + "-" + deviceSuffix());
  page += F("</p>");
  String storedSsid;
  String storedPassword;
  loadWifiCredentials(storedSsid, storedPassword);
  page += F("<form method='post' action='/save'><section class='panel'><h2>Wi-Fi</h2>");
  page += F("<p class='note'>Enter the network the clock should use for NTP time sync");
#ifdef ENABLE_OTA
  page += F(" and OTA firmware updates");
#endif
  page += F(".</p>");
  page += F("<label for='ssid'>Wi-Fi name</label><input id='ssid' name='ssid' autocomplete='off' value='");
  page += htmlEscape(storedSsid);
  page += F("' required>");
  page += F("<label for='password'>Wi-Fi password</label><input id='password' name='password' type='password' autocomplete='current-password' value='");
  page += htmlEscape(storedPassword);
  page += F("'>");
  page += F("<button class='primary' type='submit'>Save Wi-Fi and restart</button>");
  page += F("<div class='actions single'><a class='button' href='/clear'>Forget saved Wi-Fi</a></div>");
  page += F("</section></form>");
  page += F("<form method='post' action='/settings'>");
  page += F("<section class='panel'><h2>Scheduled Theme</h2>");
  page += F("<label class='check'><input name='scheduled_theme_enabled' type='checkbox' value='1'");
  if (scheduledThemeEnabled) {
    page += F(" checked");
  }
  page += F(">Enable scheduled theme</label>");
  page += F("<label for='scheduled_theme_id'>Scheduled theme</label>");
  page += F("<select id='scheduled_theme_id' name='scheduled_theme_id'>");
  appendThemeOptions(page, scheduledThemeId);
  page += F("</select>");
  page += F("<div class='inline'><label for='scheduled_start'>Start hour");
  page += F("<input id='scheduled_start' name='scheduled_start' type='number' min='0' max='23' value='");
  page += String(scheduledThemeStartHour);
  page += F("' required></label>");
  page += F("<label for='scheduled_end'>End hour");
  page += F("<input id='scheduled_end' name='scheduled_end' type='number' min='0' max='23' value='");
  page += String(scheduledThemeEndHour);
  page += F("' required></label></div>");
  page += F("</section><section class='panel'><h2>Themes</h2>");
  page += F("<label for='theme_select'>Theme</label>");
  page += F("<select id='theme_select' name='theme_id'><option value=''>Custom</option>");
  appendThemeOptions(page, configuredThemeId);
  page += F("</select>");
  page += F("<p class='hint'><strong>Preset preview</strong>Selecting a theme updates the brightness, marker, hand, and trail controls below before you save. Brightness is saved separately for each theme, including when it runs on a schedule.</p>");
  page += F("</section><section class='panel'><h2>Brightness</h2>");
  page += F("<p class='note'>Values range from 0 (off) to 255 (full brightness). Marker brightness scales the hour, minute, and second markers in addition to global brightness.</p>");
  page += F("<div class='inline'><label for='global_brightness'>Global LED brightness");
  page += F("<input id='global_brightness' name='global_brightness' type='number' min='0' max='255' step='1' value='");
  page += String(configuredGlobalLEDBrightness);
  page += F("' required></label><label for='marker_brightness'>Marker LED brightness");
  page += F("<input id='marker_brightness' name='marker_brightness' type='number' min='0' max='255' step='1' value='");
  page += String(configuredMarkerLEDBrightness);
  page += F("' required></label></div>");
  page += F("</section><section class='panel'><h2>Markers</h2>");
  page += F("<label class='check'><input name='animate_second_hand' type='checkbox' value='1'");
  if (configuredAnimateSecondHand) {
    page += F(" checked");
  }
  page += F(">Animate second hand</label>");
  page += F("<label class='check'><input name='show_hour_markers' type='checkbox' value='1'");
  if (configuredShowHoursMarkers) {
    page += F(" checked");
  }
  page += F(">Show hour markers</label>");
  page += F("<label class='check'><input name='show_minute_markers' type='checkbox' value='1'");
  if (configuredShowMinutesMarkers) {
    page += F(" checked");
  }
  page += F(">Show minute markers</label>");
  page += F("<label class='check'><input name='show_second_markers' type='checkbox' value='1'");
  if (configuredShowSecondsMarkers) {
    page += F(" checked");
  }
  page += F(">Show second markers</label>");
  page += F("</section><section class='panel'><h2>Colors</h2><div class='color-grid'>");
  appendColorSelect(page, "zero_marker_color", "Zero marker color", configuredZeroMarkerColor);
  appendColorSelect(page, "seconds_markers_color", "Second markers color", configuredSecondsMarkersColor);
  appendColorSelect(page, "minutes_markers_color", "Minute markers color", configuredMinutesMarkersColor);
  appendColorSelect(page, "hours_markers_color", "Hour markers color", configuredHoursMarkersColor);
  appendColorSelect(page, "seconds_hand_color", "Second hand color", configuredSecondsHandColor);
  appendColorSelect(page, "minutes_hand_color", "Minute hand color", configuredMinutesHandColor);
  appendColorSelect(page, "hours_hand_color", "Hour hand color", configuredHoursHandColor);
  page += F("</div></section><section class='panel'><h2>Time Trail</h2>");
  page += F("<label class='check'><input name='time_trail_enabled' type='checkbox' value='1'");
  if (configuredTimeTrailEnabled) {
    page += F(" checked");
  }
  page += F(">Enable Time Trail</label><div class='color-grid'>");
  appendColorSelect(page, "time_trail_seconds_color", "Second trail color", configuredTimeTrailSecondsColor);
  appendColorSelect(page, "time_trail_minutes_color", "Minute trail color", configuredTimeTrailMinutesColor);
  appendColorSelect(page, "time_trail_hours_color", "Hour trail color", configuredTimeTrailHoursColor);
  page += F("</div></section><button class='primary' type='submit'>Save display settings and restart</button>");
  page += F("</form><script>");
  page += F("const themes={");
  page += F("classicCorpus:{hour:true,minute:true,second:true,animate:true,trail:false,zero:'#FF0000',secMark:'#0000FF',minMark:'#0000FF',hourMark:'#0000FF',secHand:'#FFD700',minHand:'#FFD700',hourHand:'#FFD700',trailSec:'#00FF00',trailMin:'#00FFFF',trailHour:'#8000FF'},");
  page += F("solarFlare:{hour:true,minute:true,second:true,animate:true,trail:true,zero:'#FF0000',secMark:'#FF8000',minMark:'#FFBF00',hourMark:'#FF8000',secHand:'#FFD700',minHand:'#FFD700',hourHand:'#FFD700',trailSec:'#FFFF00',trailMin:'#FF8000',trailHour:'#FF0000'},");
  page += F("deepSpace:{hour:true,minute:true,second:true,animate:true,trail:true,zero:'#FFFFFF',secMark:'#0000FF',minMark:'#0000FF',hourMark:'#8000FF',secHand:'#00FFFF',minHand:'#00FFFF',hourHand:'#00FFFF',trailSec:'#0000FF',trailMin:'#8000FF',trailHour:'#00FFFF'},");
  page += F("aurora:{hour:true,minute:true,second:true,animate:true,trail:true,zero:'#00FF00',secMark:'#00FFFF',minMark:'#00FFFF',hourMark:'#0000FF',secHand:'#FFFFFF',minHand:'#FFFFFF',hourHand:'#FFFFFF',trailSec:'#00FF00',trailMin:'#00FFFF',trailHour:'#0000FF'},");
  page += F("synthwave:{hour:true,minute:true,second:true,animate:true,trail:true,zero:'#FF00FF',secMark:'#8000FF',minMark:'#8000FF',hourMark:'#FF00FF',secHand:'#00FFFF',minHand:'#00FFFF',hourHand:'#00FFFF',trailSec:'#FF66CC',trailMin:'#FF00FF',trailHour:'#0000FF'},");
  page += F("ember:{hour:true,minute:true,second:false,animate:true,trail:true,zero:'#FFBF00',secMark:'#FF0000',minMark:'#FF8000',hourMark:'#FF0000',secHand:'#FF8000',minHand:'#FF8000',hourHand:'#FF8000',trailSec:'#FF0000',trailMin:'#FF8000',trailHour:'#FFD700'},");
  page += F("iceClock:{hour:true,minute:true,second:true,animate:true,trail:true,zero:'#00FFFF',secMark:'#0000FF',minMark:'#0000FF',hourMark:'#00FFFF',secHand:'#FFFFFF',minHand:'#FFFFFF',hourHand:'#FFFFFF',trailSec:'#00FFFF',trailMin:'#0000FF',trailHour:'#FFFFFF'},");
  page += F("minimal:{hour:true,minute:false,second:false,animate:false,trail:false,zero:'#FF0000',secMark:'#0000FF',minMark:'#0000FF',hourMark:'#FFFFFF',secHand:'#FFFFFF',minHand:'#FFFFFF',hourHand:'#FFFFFF',trailSec:'#00FF00',trailMin:'#00FFFF',trailHour:'#8000FF'},");
  page += F("trailblazer:{hour:true,minute:true,second:true,animate:true,trail:true,zero:'#FFFFFF',secMark:'#0000FF',minMark:'#0000FF',hourMark:'#0000FF',secHand:'#FFD700',minHand:'#FFD700',hourHand:'#FFD700',trailSec:'#00FF00',trailMin:'#00FFFF',trailHour:'#8000FF'},");
  page += F("classicNight:{hour:false,minute:false,second:true,animate:false,trail:false,zero:'#FF8000',secMark:'#FF0000',minMark:'#FF0000',hourMark:'#FF0000',secHand:'#0000FF',minHand:'#0000FF',hourHand:'#0000FF',trailSec:'#00FF00',trailMin:'#00FFFF',trailHour:'#8000FF'},");
  page += F("stealthNight:{hour:false,minute:false,second:true,animate:false,trail:true,zero:'#FF8000',secMark:'#FF0000',minMark:'#0000FF',hourMark:'#8000FF',secHand:'#0000FF',minHand:'#0000FF',hourHand:'#0000FF',trailSec:'#8000FF',trailMin:'#0000FF',trailHour:'#8000FF'}");
  page += F("};");
  page += F("const brightness={");
  for (const ThemePreset& theme : THEME_PRESETS) {
    page += theme.id;
    page += F(":{global:");
    page += String(theme.globalBrightness);
    page += F(",marker:");
    page += String(theme.markerBrightness);
    page += F("},");
  }
  page += F("'':{global:");
  page += String(configuredGlobalLEDBrightness);
  page += F(",marker:");
  page += String(configuredMarkerLEDBrightness);
  page += F("}};");
  page += F("const fieldMap={zero:'zero_marker_color',secMark:'seconds_markers_color',minMark:'minutes_markers_color',hourMark:'hours_markers_color',secHand:'seconds_hand_color',minHand:'minutes_hand_color',hourHand:'hours_hand_color',trailSec:'time_trail_seconds_color',trailMin:'time_trail_minutes_color',trailHour:'time_trail_hours_color'};");
  page += F("function setCheck(name,value){const input=document.querySelector(`[name='${name}']`);if(input){input.checked=value;}}");
  page += F("function setColor(id,value){const select=document.getElementById(id);if(!select)return;select.value=value;select.dispatchEvent(new Event('change'));}");
  page += F("document.getElementById('theme_select').addEventListener('change',event=>{const levels=brightness[event.target.value];document.getElementById('global_brightness').value=levels.global;document.getElementById('marker_brightness').value=levels.marker;const theme=themes[event.target.value];if(!theme)return;");
  page += F("setCheck('show_hour_markers',theme.hour);setCheck('show_minute_markers',theme.minute);setCheck('show_second_markers',theme.second);setCheck('animate_second_hand',theme.animate);setCheck('time_trail_enabled',theme.trail);");
  page += F("Object.keys(fieldMap).forEach(key=>setColor(fieldMap[key],theme[key]));});");
  page += F("document.querySelectorAll('.color-row').forEach(row=>{const swatch=row.querySelector('.swatch');const select=row.querySelector('select');select.addEventListener('change',()=>{swatch.style.background=select.value;});});");
  page += F("</script></main></body></html>");
  return page;
}

String firmwarePage(const String& message = "") {
  String page;
  page.reserve(5000);
  page += F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<title>Archer Clock Firmware</title><style>");
  appendPageStyles(page);
  page += F("</style></head><body><main class='page'>");
  appendPageHeader(page, "Firmware", "/firmware");
  if (message.length() > 0) {
    page += F("<p class='msg'>");
    page += htmlEscape(message);
    page += F("</p>");
  }
  page += F("<p class='status'>Upload a PlatformIO firmware binary. The clock will restart after a successful update.</p>");
  page += F("<form id='firmware-form' method='post' action='/update' enctype='multipart/form-data'><section class='panel'><h2>Firmware Update</h2>");
  page += F("<p class='hint'><strong>Expected file</strong><code>.pio/build/wemos_d1_mini32/firmware.bin</code></p>");
  page += F("<label class='uploadBox' for='firmware'><span>Select firmware binary</span><small>.bin or application/octet-stream</small><div id='firmware-file-status' class='fileStatus'>No file selected</div><input id='firmware' name='firmware' type='file' accept='.bin,application/octet-stream' required></label>");
  page += F("<progress id='upload-progress' value='0' max='100'></progress>");
  page += F("<div id='upload-percent' class='progress-text'>0%</div>");
  page += F("</section><button id='upload-button' class='primary' type='submit' disabled>Upload firmware</button>");
  page += F("</form><script>");
  page += F("const form=document.getElementById('firmware-form');");
  page += F("const firmware=document.getElementById('firmware');");
  page += F("const uploadButton=document.getElementById('upload-button');");
  page += F("const fileStatus=document.getElementById('firmware-file-status');");
  page += F("const bar=document.getElementById('upload-progress');");
  page += F("const label=document.getElementById('upload-percent');");
  page += F("function validFirmwareFile(){const file=firmware.files[0];if(!file)return false;const name=file.name.toLowerCase();return name.endsWith('.bin')||file.type==='application/octet-stream';}");
  page += F("firmware.addEventListener('change',()=>{const file=firmware.files[0];const valid=validFirmwareFile();uploadButton.disabled=!valid;fileStatus.className='fileStatus';if(!file){fileStatus.textContent='No file selected';return;}if(valid){fileStatus.textContent=file.name;fileStatus.classList.add('ready');}else{fileStatus.textContent='Invalid file: '+file.name;fileStatus.classList.add('invalid');}});");
  page += F("form.addEventListener('submit',event=>{event.preventDefault();if(!validFirmwareFile()){uploadButton.disabled=true;return;}");
  page += F("const data=new FormData(form);const request=new XMLHttpRequest();");
  page += F("bar.value=0;label.textContent='0%';");
  page += F("request.upload.onprogress=event=>{if(event.lengthComputable){const percent=Math.round((event.loaded/event.total)*100);bar.value=percent;label.textContent=percent+'%';}};");
  page += F("request.onload=()=>{document.open();document.write(request.responseText);document.close();};");
  page += F("request.onerror=()=>{label.textContent='Upload failed';};");
  page += F("request.open('POST','/update');request.send(data);});");
  page += F("</script></main></body></html>");
  return page;
}

void handleConfigRoot() {
  configServer.send(200, "text/html", configPage());
}

void handleFirmwareRoot() {
  configServer.send(200, "text/html", firmwarePage());
}

void handleConfigSave() {
  String newSsid = configServer.arg("ssid");
  String newPassword = configServer.arg("password");
  newSsid.trim();

  if (newSsid.length() == 0) {
    configServer.send(400, "text/html", configPage("Wi-Fi name is required."));
    return;
  }

  saveWifiCredentials(newSsid, newPassword);
  configServer.send(200, "text/html", configPage("Saved. The clock is restarting now."));
  delay(750);
  ESP.restart();
}

void handleSettingsSave() {
  String requestedThemeId = configServer.arg("theme_id");
  if (requestedThemeId.length() > 0 && findThemePreset(requestedThemeId) == nullptr) {
    configServer.send(400, "text/html", configPage("Theme was invalid."));
    return;
  }
  // Reject missing, fractional, signed, or out-of-range values before saving.
  auto parseBrightness = [](const String& value, uint8_t& result) {
    if (value.length() == 0 || value.length() > 3) return false;
    unsigned int parsed = 0;
    for (unsigned int i = 0; i < value.length(); ++i) {
      if (value[i] < '0' || value[i] > '9') return false;
      parsed = parsed * 10 + (value[i] - '0');
    }
    if (parsed > 255) return false;
    result = static_cast<uint8_t>(parsed);
    return true;
  };
  uint8_t newGlobalBrightness;
  uint8_t newMarkerBrightness;
  if (!parseBrightness(configServer.arg("global_brightness"), newGlobalBrightness) ||
      !parseBrightness(configServer.arg("marker_brightness"), newMarkerBrightness)) {
    configServer.send(400, "text/html", configPage("Brightness values must be whole numbers between 0 and 255."));
    return;
  }
  int requestedStartHour = configServer.arg("scheduled_start").toInt();
  int requestedEndHour = configServer.arg("scheduled_end").toInt();
  String requestedScheduledThemeId = configServer.arg("scheduled_theme_id");

  if (requestedStartHour < 0 || requestedStartHour > 23 || requestedEndHour < 0 || requestedEndHour > 23) {
    configServer.send(400, "text/html", configPage("Scheduled theme hours must be between 0 and 23."));
    return;
  }

  if (findThemePreset(requestedScheduledThemeId) == nullptr) {
    configServer.send(400, "text/html", configPage("Scheduled theme was invalid."));
    return;
  }

  CRGB newZeroMarkerColor;
  CRGB newSecondsMarkersColor;
  CRGB newMinutesMarkersColor;
  CRGB newHoursMarkersColor;
  CRGB newSecondsHandColor;
  CRGB newMinutesHandColor;
  CRGB newHoursHandColor;
  CRGB newTimeTrailSecondsColor;
  CRGB newTimeTrailMinutesColor;
  CRGB newTimeTrailHoursColor;

  if (!parseHexColor(configServer.arg("zero_marker_color"), newZeroMarkerColor) ||
      !parseHexColor(configServer.arg("seconds_markers_color"), newSecondsMarkersColor) ||
      !parseHexColor(configServer.arg("minutes_markers_color"), newMinutesMarkersColor) ||
      !parseHexColor(configServer.arg("hours_markers_color"), newHoursMarkersColor) ||
      !parseHexColor(configServer.arg("seconds_hand_color"), newSecondsHandColor) ||
      !parseHexColor(configServer.arg("minutes_hand_color"), newMinutesHandColor) ||
      !parseHexColor(configServer.arg("hours_hand_color"), newHoursHandColor) ||
      !parseHexColor(configServer.arg("time_trail_seconds_color"), newTimeTrailSecondsColor) ||
      !parseHexColor(configServer.arg("time_trail_minutes_color"), newTimeTrailMinutesColor) ||
      !parseHexColor(configServer.arg("time_trail_hours_color"), newTimeTrailHoursColor)) {
    configServer.send(400, "text/html", configPage("One or more color values were invalid."));
    return;
  }

  saveDisplaySettings(
    requestedThemeId,
    newGlobalBrightness,
    newMarkerBrightness,
    configServer.hasArg("scheduled_theme_enabled"),
    static_cast<uint8_t>(requestedStartHour),
    static_cast<uint8_t>(requestedEndHour),
    requestedScheduledThemeId,
    configServer.hasArg("animate_second_hand"),
    configServer.hasArg("show_second_markers"),
    configServer.hasArg("show_minute_markers"),
    configServer.hasArg("show_hour_markers"),
    newZeroMarkerColor,
    newSecondsMarkersColor,
    newMinutesMarkersColor,
    newHoursMarkersColor,
    newSecondsHandColor,
    newMinutesHandColor,
    newHoursHandColor,
    configServer.hasArg("time_trail_enabled"),
    newTimeTrailSecondsColor,
    newTimeTrailMinutesColor,
    newTimeTrailHoursColor
  );

  configServer.send(200, "text/html", configPage("Display settings saved. The clock is restarting now."));
  delay(750);
  ESP.restart();
}

void handleConfigClear() {
  clearWifiCredentials();
  configServer.send(200, "text/html", configPage("Saved Wi-Fi credentials were cleared."));
}

void handleFirmwareUploadDone() {
  bool updateOk = !Update.hasError();
  configServer.send(
    updateOk ? 200 : 500,
    "text/html",
    firmwarePage(updateOk ? "Firmware uploaded. The clock is restarting now." : "Firmware upload failed.")
  );

  if (updateOk) {
    delay(750);
    ESP.restart();
  }
}

void handleFirmwareUpload() {
  HTTPUpload& upload = configServer.upload();

  if (upload.status == UPLOAD_FILE_START) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    setStatusLed(CRGB::Aqua);

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      setStatusLed(CRGB::Red);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      setStatusLed(CRGB::Red);
    } else {
      uint8_t lit = map(upload.totalSize, 0, ESP.getFreeSketchSpace(), 0, NUM_LEDS);
      fill_solid(leds, NUM_LEDS, CRGB::Black);
      for (uint8_t i = 0; i < lit && i < NUM_LEDS; ++i) {
        leds[i] = CRGB::Aqua;
      }
      FastLED.show();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      setStatusLed(CRGB::Green);
    } else {
      setStatusLed(CRGB::Red);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    setStatusLed(CRGB::Red);
  }
}

void redirectToConfigPortal() {
  IPAddress address = WiFi.status() == WL_CONNECTED ? WiFi.localIP() : WiFi.softAPIP();
  String location = "http://" + address.toString() + "/";
  configServer.sendHeader("Location", location, true);
  configServer.send(302, "text/plain", "");
}

void startConfigWebServer() {
  if (webServerStarted) {
    return;
  }

  configServer.on("/", HTTP_GET, handleConfigRoot);
  configServer.on("/firmware", HTTP_GET, handleFirmwareRoot);
  configServer.on("/save", HTTP_POST, handleConfigSave);
  configServer.on("/settings", HTTP_POST, handleSettingsSave);
  configServer.on("/clear", HTTP_GET, handleConfigClear);
  configServer.on("/update", HTTP_POST, handleFirmwareUploadDone, handleFirmwareUpload);
  configServer.on("/generate_204", HTTP_GET, redirectToConfigPortal);
  configServer.on("/fwlink", HTTP_GET, redirectToConfigPortal);
  configServer.onNotFound(redirectToConfigPortal);
  configServer.begin();
  webServerStarted = true;
}

void startConfigPortal() {
  configPortalActive = true;
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  setStatusLed(CRGB::Purple);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);

  String apSsid = String(CONFIG_AP_SSID_PREFIX) + deviceSuffix();
  if (strlen(CONFIG_AP_PASSWORD) >= 8) {
    WiFi.softAP(apSsid.c_str(), CONFIG_AP_PASSWORD);
  } else {
    WiFi.softAP(apSsid.c_str());
  }

  dnsServer.start(53, "*", WiFi.softAPIP());
  startConfigWebServer();

  while (configPortalActive) {
    dnsServer.processNextRequest();
    configServer.handleClient();
    delay(10);
  }
}

void serviceNetwork() {
  if (configPortalActive) {
    dnsServer.processNextRequest();
  }

  if (webServerStarted) {
    configServer.handleClient();
  }

#ifdef ENABLE_OTA
  if (otaStarted) {
    ArduinoOTA.handle();
  }
#endif
}

bool connectToConfiguredWifi() {
  String storedSsid;
  String storedPassword;
  if (!loadWifiCredentials(storedSsid, storedPassword)) {
    startConfigPortal();
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOSTNAME);
  WiFi.begin(storedSsid.c_str(), storedPassword.c_str());

  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    startConfigWebServer();
    if (MDNS.begin(OTA_HOSTNAME)) {
      MDNS.addService("http", "tcp", 80);
    }
    return true;
  }

  startConfigPortal();
  return false;
}

#ifdef ENABLE_OTA
void startOta() {
  if (otaStarted || WiFi.status() != WL_CONNECTED) {
    return;
  }

  ArduinoOTA.setHostname(OTA_HOSTNAME);
  if (strlen(OTA_PASSWORD) > 0) {
    ArduinoOTA.setPassword(OTA_PASSWORD);
  }
  ArduinoOTA.onStart([]() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    setStatusLed(CRGB::Aqua);
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    uint8_t lit = map(progress, 0, total, 0, NUM_LEDS);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    for (uint8_t i = 0; i < lit && i < NUM_LEDS; ++i) {
      leds[i] = CRGB::Aqua;
    }
    FastLED.show();
  });
  ArduinoOTA.onError([](ota_error_t) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    setStatusLed(CRGB::Red);
  });
  ArduinoOTA.begin();
  otaStarted = true;
}
#endif

bool hourInWindow(uint8_t currentHour, uint8_t startHour, uint8_t endHour) {
  if (startHour == endHour) {
    return false;
  }

  if (startHour < endHour) {
    return currentHour >= startHour && currentHour < endHour;
  }

  return currentHour >= startHour || currentHour < endHour;
}

bool scheduledThemeIsActive() {
  if (!scheduledThemeEnabled) {
    return false;
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return hourInWindow(hours, scheduledThemeStartHour, scheduledThemeEndHour);
  }

  return hourInWindow(timeinfo.tm_hour, scheduledThemeStartHour, scheduledThemeEndHour);
}

void draw(CRGB leds[], uint8_t s, uint8_t m, uint8_t h) {
  // Scale factor for markers (0..255). 32 is dim; max is 255 (not 256).
  const uint8_t MARKER_SCALE = markerLEDBrightness;

  // Clear all LEDs if we just switched modes
  if (switchModes) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    switchModes = false;
  }

  if (timeTrailEnabled) {
    fill_solid(leds, 60, CRGB::Black);
    fill_solid(&leds[60], 60, CRGB::Black);
    fill_solid(&leds[120], 12, CRGB::Black);
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

  if (timeTrailEnabled) {
    for (int c = 0; c < trailSeconds; ++c) {
      leds[c] = timeTrailSecondsColor;
    }

    for (int c = 0; c < trailMinutes; ++c) {
      leds[60 + c] = timeTrailMinutesColor;
    }

    for (int c = 0; c <= trailHours && c < 12; ++c) {
      leds[120 + c] = timeTrailHoursColor;
    }
  }

  // Draw hands at full color (no extra scaling)
  leds[s] = secondsHandColor;
  leds[60 + m] = minutesHandColor;
  leds[120 + h] = hoursHandColor;

  FastLED.show();
}


void syncronize() {
  setStatusLed(CRGB::Red);
  delay(500);

  setStatusLed(CRGB::Orange);
  if (!connectToConfiguredWifi()) {
    return;
  }

  setStatusLed(CRGB::Green);

  #ifdef ENABLE_OTA
  startOta();
  #endif

  // Configuration for synchronization with the NTP server
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;
  
  hours = timeinfo.tm_hour % 12;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
  trailHours = hours;
  trailMinutes = minutes;
  trailSeconds = seconds;
  syncCountdown = SYNC_TIME;

  #ifdef DEBUG_NIGHT_MODE
  hours = 23;
  minutes = 59;
  seconds = 55;
  #endif

  #ifdef ENABLE_OTA
  startOta();
  #else
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
  #endif
}


void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setCorrection(CRGB::White);
  loadDisplaySettings();

  // set global brightness (0..255)
  FastLED.setBrightness(configuredGlobalLEDBrightness);

  // optional: limit total current to ~500mA at 5V
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 500);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  syncronize();

  previousMillis = millis();

}

void loop() {

  // Brightness
  globalLEDBrightness = configuredGlobalLEDBrightness;
  markerLEDBrightness = configuredMarkerLEDBrightness;

  // Animation
  animateSecondHand = configuredAnimateSecondHand;

  // Unremark for show
  showSecondsMarkers = configuredShowSecondsMarkers;
  showMinutesMarkers = configuredShowMinutesMarkers;
  showHoursMarkers = configuredShowHoursMarkers;

  // Marckers color
  zeroMarkerColor = configuredZeroMarkerColor;
  secondsMarkersColor = configuredSecondsMarkersColor;
  minutesMarkersColor = configuredMinutesMarkersColor;
  hoursMarkersColor = configuredHoursMarkersColor;

  // Hands color
  secondsHandColor = configuredSecondsHandColor;
  minutesHandColor = configuredMinutesHandColor;
  hoursHandColor = configuredHoursHandColor;

  timeTrailEnabled = configuredTimeTrailEnabled;
  timeTrailSecondsColor = configuredTimeTrailSecondsColor;
  timeTrailMinutesColor = configuredTimeTrailMinutesColor;
  timeTrailHoursColor = configuredTimeTrailHoursColor;

  bool useScheduledTheme = scheduledThemeIsActive();
  display_mode nextMode = useScheduledTheme ? MODE_SCHEDULED : MODE_DAY;
  if (currentMode != nextMode) {
    currentMode = nextMode;
    switchModes = true;
  }

  if (useScheduledTheme) {
    const ThemePreset* scheduledTheme = findThemePreset(scheduledThemeId);
    if (scheduledTheme != nullptr) {
      applyThemePreset(*scheduledTheme);
      timeTrailEnabled = configuredTimeTrailEnabled;
    }
  }

  FastLED.setBrightness(globalLEDBrightness);


  unsigned long currentMillis = millis();
  while (currentMillis - previousMillis < 1000) {
    serviceNetwork();
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
  if (timeTrailEnabled) {
    trailSeconds = seconds;
    trailMinutes = minutes;
    trailHours = hours;
  }

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
    serviceNetwork();
    if (acceleration > 3) acceleration--;

    if (animateSecondHand) {
      if (!timeTrailEnabled) {
        leds[seconds] = CRGB::Black;
      }
    }

    // do not clear the seconds LED while sweeping - keep it static
    // only clear the moving minute/hour pixels so the sweep is visible
    if (!timeTrailEnabled) {
      leds[60+minutes] = CRGB::Black;
      leds[120+hours] = CRGB::Black;
    }
  }

  if (timeTrailEnabled && animateSecondHand) {
    trailSeconds = seconds;
    trailMinutes = minutes;
    trailHours = hours;
    draw(leds, seconds, minutes, hours);
  }

  if (!animateSecondHand) {
    // if not animating the second hand, we need to advance seconds here
  // advance seconds exactly once after the sweep
  leds[oldSeconds] = CRGB::Black; // clear old second LED
  seconds++;
  if (seconds>59) seconds = 0;

  if (timeTrailEnabled) {
    trailSeconds = seconds;
    trailMinutes = minutes;
    trailHours = hours;
  }

  // update display to show the new second tick
  draw(leds, seconds, minutes, hours);
  } 


  if (syncCountdown == 0) syncronize();
  serviceNetwork();
}

