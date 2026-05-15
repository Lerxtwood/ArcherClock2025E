#define FIRM_VERSION 0.8

// Wi-Fi credentials
const char* ssid = "lerxt1";
const char* password = "thisisatest";

#define LED_PIN   16
#define NUM_LEDS  132

#define ENABLE_NIGHT_MODE
#define NIGHT_MODE_START_HOUR  23   


// NIGHT MODE SETTINGS
// Brightness
#define NIGHT_GLOBAL_LED_BRIGHTNESS  64
#define NIGHT_MARKER_LED_BRIGHTNESS  64

// Animation
#define NIGHT_ANIMATE_SECOND_HAND   0

// Unremark for show
#define NIGHT_SHOW_SECONDS_MARCKERS 1
#define NIGHT_SHOW_MINUTES_MARCKERS 0
#define NIGHT_SHOW_HOURS_MARCKERS   0

// Marckers color
#define NIGHT_ZERO_MARKER_COLOR CRGB::Orange
#define NIGHT_SECONDS_MARCKERS_COLOR CRGB::Red
#define NIGHT_MINUTES_MARCKERS_COLOR CRGB::Red
#define NIGHT_HOURS_MARCKERS_COLOR CRGB::Red

// Hands color
#define NIGHT_SECONDS_HAND_COLOR CRGB::Blue
#define NIGHT_MINUTES_HAND_COLOR CRGB::Blue
#define NIGHT_HOURS_HAND_COLOR CRGB::Blue



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

// Time zone and NTP server
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -25200; // Time offset in seconds (GMT+1)
const int daylightOffset_sec = 3600; // Daylight Saving Time (in seconds, optional)

// Synchronization time in hours
#define SYNC_TIME 24
