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


//#define DEBUG_NIGHT_MODE

typedef enum {
MODE_DAY = 0,
MODE_NIGHT = 1
} display_mode;

CRGB leds[NUM_LEDS];
Preferences preferences;
WebServer configServer(80);
DNSServer dnsServer;

uint8_t seconds = 0;
uint8_t minutes = 0;
uint8_t hours = 0;
uint8_t syncCountdown = SYNC_TIME;
bool switchModes = false;
display_mode currentMode = MODE_DAY;
bool configPortalActive = false;
bool webServerStarted = false;
#ifdef ENABLE_OTA
bool otaStarted = false;
#endif

#ifdef ENABLE_NIGHT_MODE
bool nightModeEnabled = true;
#else
bool nightModeEnabled = false;
#endif
uint8_t nightModeStartHour = NIGHT_MODE_START_HOUR;
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

  page += F("<label for='");
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

  page += F("</select></div>");
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
  preferences.begin("display", true);
  nightModeEnabled = preferences.getBool("nightEnabled", nightModeEnabled);
  nightModeStartHour = preferences.getUChar("nightStart", nightModeStartHour);
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
  preferences.end();

  if (nightModeStartHour > 23) {
    nightModeStartHour = NIGHT_MODE_START_HOUR;
  }
}

void saveDisplaySettings(
  bool newNightModeEnabled,
  uint8_t newNightModeStartHour,
  bool newShowSecondsMarkers,
  bool newShowMinutesMarkers,
  bool newShowHoursMarkers,
  CRGB newZeroMarkerColor,
  CRGB newSecondsMarkersColor,
  CRGB newMinutesMarkersColor,
  CRGB newHoursMarkersColor,
  CRGB newSecondsHandColor,
  CRGB newMinutesHandColor,
  CRGB newHoursHandColor
) {
  preferences.begin("display", false);
  preferences.putBool("nightEnabled", newNightModeEnabled);
  preferences.putUChar("nightStart", newNightModeStartHour);
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
  preferences.end();

  nightModeEnabled = newNightModeEnabled;
  nightModeStartHour = newNightModeStartHour;
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
  switchModes = true;
}

String configPage(const String& message = "") {
  String page;
  page.reserve(2800);
  page += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<title>Archer Clock Setup</title><style>");
  page += F("body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;background:#111;color:#f7f7f7}");
  page += F("main{max-width:520px;margin:0 auto;padding:28px 20px}label{display:block;margin:18px 0 6px}");
  page += F("input{box-sizing:border-box;width:100%;padding:12px;border-radius:6px;border:1px solid #555;background:#1d1d1d;color:#fff;font-size:16px}");
  page += F("input[type=checkbox]{width:auto;margin-right:8px;transform:scale(1.2)}");
  page += F("select{box-sizing:border-box;width:100%;padding:12px;border-radius:6px;border:1px solid #555;background:#1d1d1d;color:#fff;font-size:16px}");
  page += F(".color-row{display:grid;grid-template-columns:34px 1fr;gap:10px;align-items:center}.swatch{display:block;width:30px;height:30px;border-radius:6px;border:1px solid #777}");
  page += F("button,a.button{display:inline-block;margin-top:20px;padding:11px 14px;border:0;border-radius:6px;background:#f4c542;color:#111;font-weight:700;text-decoration:none}");
  page += F("a.button.secondary{background:#333;color:#fff;margin-left:8px}.note{color:#bbb;line-height:1.45}.msg{padding:10px 12px;background:#173b28;border:1px solid #2c8053;border-radius:6px}");
  page += F("script{display:none}");
  page += F("</style></head><body><main><h1>Archer Clock Setup</h1>");
  if (message.length() > 0) {
    page += F("<p class='msg'>");
    page += htmlEscape(message);
    page += F("</p>");
  }
  page += F("<p class='note'>Enter the Wi-Fi network the clock should use for NTP time sync");
#ifdef ENABLE_OTA
  page += F(" and OTA firmware updates");
#endif
  page += F(".</p><form method='post' action='/save'>");
  page += F("<label for='ssid'>Wi-Fi name</label><input id='ssid' name='ssid' autocomplete='off' required>");
  page += F("<label for='password'>Wi-Fi password</label><input id='password' name='password' type='password' autocomplete='current-password'>");
  page += F("<button type='submit'>Save and Restart</button>");
  page += F("<a class='button secondary' href='/clear'>Forget Saved Wi-Fi</a>");
  page += F("<a class='button secondary' href='/firmware'>Firmware Update</a>");
  page += F("</form><h2>Night Mode</h2>");
  page += F("<form method='post' action='/settings'>");
  page += F("<label><input name='night_enabled' type='checkbox' value='1'");
  if (nightModeEnabled) {
    page += F(" checked");
  }
  page += F("> Enable night mode</label>");
  page += F("<label for='night_start'>Night mode start hour</label>");
  page += F("<input id='night_start' name='night_start' type='number' min='0' max='23' value='");
  page += String(nightModeStartHour);
  page += F("' required>");
  page += F("<h2>Markers</h2>");
  page += F("<label><input name='show_hour_markers' type='checkbox' value='1'");
  if (configuredShowHoursMarkers) {
    page += F(" checked");
  }
  page += F("> Show hour markers</label>");
  page += F("<label><input name='show_minute_markers' type='checkbox' value='1'");
  if (configuredShowMinutesMarkers) {
    page += F(" checked");
  }
  page += F("> Show minute markers</label>");
  page += F("<label><input name='show_second_markers' type='checkbox' value='1'");
  if (configuredShowSecondsMarkers) {
    page += F(" checked");
  }
  page += F("> Show second markers</label>");
  page += F("<h2>Colors</h2>");
  appendColorSelect(page, "zero_marker_color", "Zero marker color", configuredZeroMarkerColor);
  appendColorSelect(page, "seconds_markers_color", "Second markers color", configuredSecondsMarkersColor);
  appendColorSelect(page, "minutes_markers_color", "Minute markers color", configuredMinutesMarkersColor);
  appendColorSelect(page, "hours_markers_color", "Hour markers color", configuredHoursMarkersColor);
  appendColorSelect(page, "seconds_hand_color", "Second hand color", configuredSecondsHandColor);
  appendColorSelect(page, "minutes_hand_color", "Minute hand color", configuredMinutesHandColor);
  appendColorSelect(page, "hours_hand_color", "Hour hand color", configuredHoursHandColor);
  page += F("<button type='submit'>Save Display Settings and Restart</button>");
  page += F("</form><p class='note'>Setup AP: ");
  String apSsid = WiFi.softAPSSID();
  page += htmlEscape(apSsid.length() > 0 ? apSsid : String("not active"));
  page += F("<br>Address: ");
  page += htmlEscape(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString());
  page += F("<br>Device: ");
  page += htmlEscape(String(OTA_HOSTNAME) + "-" + deviceSuffix());
  page += F("</p><script>");
  page += F("document.querySelectorAll('.color-row').forEach(row=>{const swatch=row.querySelector('.swatch');const select=row.querySelector('select');select.addEventListener('change',()=>{swatch.style.background=select.value;});});");
  page += F("</script></main></body></html>");
  return page;
}

String firmwarePage(const String& message = "") {
  String page;
  page.reserve(2400);
  page += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  page += F("<title>Archer Clock Firmware</title><style>");
  page += F("body{font-family:system-ui,-apple-system,Segoe UI,sans-serif;margin:0;background:#111;color:#f7f7f7}");
  page += F("main{max-width:520px;margin:0 auto;padding:28px 20px}label{display:block;margin:18px 0 6px}");
  page += F("input{box-sizing:border-box;width:100%;padding:12px;border-radius:6px;border:1px solid #555;background:#1d1d1d;color:#fff;font-size:16px}");
  page += F("button,a.button{display:inline-block;margin-top:20px;padding:11px 14px;border:0;border-radius:6px;background:#f4c542;color:#111;font-weight:700;text-decoration:none}");
  page += F("a.button.secondary{background:#333;color:#fff;margin-left:8px}.note{color:#bbb;line-height:1.45}.msg{padding:10px 12px;background:#173b28;border:1px solid #2c8053;border-radius:6px}");
  page += F("progress{width:100%;height:18px;margin-top:18px}.progress-text{margin-top:8px;color:#f4c542;font-weight:700}");
  page += F("</style></head><body><main><h1>Firmware Update</h1>");
  if (message.length() > 0) {
    page += F("<p class='msg'>");
    page += htmlEscape(message);
    page += F("</p>");
  }
  page += F("<p class='note'>Choose a PlatformIO firmware binary, usually <code>.pio/build/wemos_d1_mini32/firmware.bin</code>. The clock will restart after a successful upload.</p>");
  page += F("<form id='firmware-form' method='post' action='/update' enctype='multipart/form-data'>");
  page += F("<label for='firmware'>Firmware file</label><input id='firmware' name='firmware' type='file' accept='.bin,application/octet-stream' required>");
  page += F("<button type='submit'>Upload Firmware</button>");
  page += F("<a class='button secondary' href='/'>Back</a>");
  page += F("<progress id='upload-progress' value='0' max='100' hidden></progress>");
  page += F("<div id='upload-percent' class='progress-text' hidden>0%</div>");
  page += F("</form><script>");
  page += F("const form=document.getElementById('firmware-form');");
  page += F("const bar=document.getElementById('upload-progress');");
  page += F("const label=document.getElementById('upload-percent');");
  page += F("form.addEventListener('submit',event=>{event.preventDefault();");
  page += F("const data=new FormData(form);const request=new XMLHttpRequest();");
  page += F("bar.hidden=false;label.hidden=false;bar.value=0;label.textContent='0%';");
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
  int requestedStartHour = configServer.arg("night_start").toInt();
  if (requestedStartHour < 0 || requestedStartHour > 23) {
    configServer.send(400, "text/html", configPage("Night mode start hour must be between 0 and 23."));
    return;
  }

  CRGB newZeroMarkerColor;
  CRGB newSecondsMarkersColor;
  CRGB newMinutesMarkersColor;
  CRGB newHoursMarkersColor;
  CRGB newSecondsHandColor;
  CRGB newMinutesHandColor;
  CRGB newHoursHandColor;

  if (!parseHexColor(configServer.arg("zero_marker_color"), newZeroMarkerColor) ||
      !parseHexColor(configServer.arg("seconds_markers_color"), newSecondsMarkersColor) ||
      !parseHexColor(configServer.arg("minutes_markers_color"), newMinutesMarkersColor) ||
      !parseHexColor(configServer.arg("hours_markers_color"), newHoursMarkersColor) ||
      !parseHexColor(configServer.arg("seconds_hand_color"), newSecondsHandColor) ||
      !parseHexColor(configServer.arg("minutes_hand_color"), newMinutesHandColor) ||
      !parseHexColor(configServer.arg("hours_hand_color"), newHoursHandColor)) {
    configServer.send(400, "text/html", configPage("One or more color values were invalid."));
    return;
  }

  saveDisplaySettings(
    configServer.hasArg("night_enabled"),
    static_cast<uint8_t>(requestedStartHour),
    configServer.hasArg("show_second_markers"),
    configServer.hasArg("show_minute_markers"),
    configServer.hasArg("show_hour_markers"),
    newZeroMarkerColor,
    newSecondsMarkersColor,
    newMinutesMarkersColor,
    newHoursMarkersColor,
    newSecondsHandColor,
    newMinutesHandColor,
    newHoursHandColor
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

bool inNightMode() {
  #ifdef ENABLE_NIGHT_MODE
    if (!nightModeEnabled) {
      if (currentMode != MODE_DAY) {
        currentMode = MODE_DAY;
        switchModes = true;
      }
      return false;
    }

    #ifdef DEBUG_NIGHT_MODE
    Serial.println("Debugging Night Mode");
    int currentHour = hours;
    if (nightModeStartHour <= hours || hours < 7) 
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
    if (nightModeStartHour <= timeinfo.tm_hour || timeinfo.tm_hour < 7) 
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
  serviceNetwork();
}

