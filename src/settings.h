// SPDX-FileCopyrightText: 2023 ThingPulse Ltd., https://thingpulse.com
// SPDX-License-Identifier: MIT

#pragma once

// ****************************************************************************
// User settings
// ****************************************************************************
// WiFi
const char *SSID = "";
const char *WIFI_PWD = "";

// timezone Europe/Zurich as per https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// Spotify settings
const char *SPOTIFY_CLIENT_ID = "";
const char *SPOTIFY_CLIENT_SECRET = "";
// Use http://<value-configured-here>.local/callback/ as the redirect URI for the app on Spotify.
// Hence, the default URI is http://tp-spotify.local/callback/.
// If you change the value here, you need to modify the redirect URI on Spotify as well.
#define SPOTIFY_ESPOTIFIER_NODE_NAME "tp-spotify"


// ****************************************************************************
// System settings - do not modify unless you understand what you are doing!
// ****************************************************************************
typedef struct RectangleDef {
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
} RectangleDef;

RectangleDef timeSpritePos = {0, 0, 320, 88};

// 2: portrait, on/off switch right side -> 0/0 top left
// 3: landscape, on/off switch at the top -> 0/0 top left
#define TFT_ROTATION 2
// all other TFT_xyz flags are defined in platformio.ini as PIO build flags

// 0: portrait, on/off switch right side -> 0/0 top left
// 1: landscape, on/off switch at the top -> 0/0 top left
#define TOUCH_ROTATION 0
#define TOUCH_SENSITIVITY 40
#define TOUCH_SDA 23
#define TOUCH_SCL 22
// Initial LCD Backlight brightness
#define TFT_LED_BRIGHTNESS 200

// the medium blue in the TP logo is 0x0067B0 which converts to 0x0336 in 16bit RGB565
#define TFT_TP_BLUE 0x0336

// format specifiers: https://cplusplus.com/reference/ctime/strftime/
#ifdef DATE_TIME_FORMAT_US
  int timePosX = 29;
  #define UI_DATE_FORMAT "%m/%d/%Y"
  #define UI_TIME_FORMAT "%I:%M:%S %P"
  #define UI_TIME_FORMAT_NO_SECONDS "%I:%M %P"
  #define UI_TIMESTAMP_FORMAT (UI_DATE_FORMAT + " " + UI_TIME_FORMAT)
#else
  int timePosX = 68;
  #define UI_DATE_FORMAT "%d.%m.%Y"
  #define UI_TIME_FORMAT "%H:%M:%S"
  #define UI_TIME_FORMAT_NO_SECONDS "%H:%M"
  #define UI_TIMESTAMP_FORMAT (UI_DATE_FORMAT + " " + UI_TIME_FORMAT)
#endif

#define SYSTEM_TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"
#define CLOCK_TASK_INTERVAL_MILLIS 3600000

// the spotify-api-arduino library sets this to 1 (i.e. enabled) by default
// #define SPOTIFY_DEBUG 0

#define SPOTIFY_REFRESH_TOKEN_FILE_NAME "/refresh-token.txt"
// the '/callback/' path is essential as spotify.h#fetchSpotifyAuthCode() registers a handler for it
#define SPOTIFY_REDIRECT_URI "http%3A%2F%2F" SPOTIFY_ESPOTIFIER_NODE_NAME ".local%2Fcallback%2F"

#define APP_NAME "ESP32 Spotify Remote"
#define VERSION "1.0.0"
