#define FIRM_VERSION "0.9.2"

// Wi-Fi credentials are configured at runtime from the clock's setup page
// and stored in ESP32 non-volatile storage.
#define CONFIG_AP_SSID_PREFIX "ArcherClock-"
#define CONFIG_AP_PASSWORD ""   // Leave empty for an open setup AP.

// OTA updates require the clock to stay connected to Wi-Fi after time sync.
#define ENABLE_OTA
#define OTA_HOSTNAME "archer-clock"
#define OTA_PASSWORD ""         // Optional. Set at build time or here if desired.

#define WIFI_CONNECT_TIMEOUT_MS 20000

#define LED_PIN   16
#define NUM_LEDS  132

// SCHEDULED THEME SETTINGS
#define SCHEDULED_THEME_ENABLED 1
#define SCHEDULED_THEME_ID "classicNight"
#define SCHEDULED_THEME_START_HOUR 23
#define SCHEDULED_THEME_END_HOUR 7



// NORMAL MODE SETTINGS
// Brightness
#define GLOBAL_LED_BRIGHTNESS  255
#define MARKER_LED_BRIGHTNESS  128

// Animation
#define ANIMATE_SECOND_HAND 1

// Unremark for show
#define SHOW_SECONDS_MARCKERS   1
#define SHOW_MINUTES_MARCKERS   1
#define SHOW_HOURS_MARCKERS     1

// Marckers color
#define ZERO_MARKER_COLOR CRGB::Red
#define SECONDS_MARCKERS_COLOR CRGB::Blue
#define MINUTES_MARCKERS_COLOR CRGB::Blue
#define HOURS_MARCKERS_COLOR CRGB::Blue

// Hands color
#define SECONDS_HAND_COLOR CRGB::Gold
#define MINUTES_HAND_COLOR CRGB::Gold
#define HOURS_HAND_COLOR CRGB::Gold

// TIME TRAIL SETTINGS
#define TIME_TRAIL_ENABLED 0
#define TIME_TRAIL_SECONDS_COLOR CRGB::Green
#define TIME_TRAIL_MINUTES_COLOR CRGB::Cyan
#define TIME_TRAIL_HOURS_COLOR CRGB::Purple

// Time zone and NTP server
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -25200; // Time offset in seconds (GMT+1)
const int daylightOffset_sec = 3600; // Daylight Saving Time (in seconds, optional)

// Synchronization time in hours
#define SYNC_TIME 24
