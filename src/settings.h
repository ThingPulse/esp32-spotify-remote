/*-------------------------------------------------------------------------------------------------
**
** settings.h
**
**    Main constants used by the application, including versioning,
**    compile-time info, time zone, display rotation, and system behavior
**    for the ESP32 Spotify Remote.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2024-12-26 - Electric Diversions - Copied from ThingPulse Spotify Remote.
**    2024-12-27 - Electric Diversions - Removed blocking declarations and dead code.
** ------------------------------------------------------------------------------------------------
*/

#pragma once
#include "compile_time.h"

// ****************************************************************************
// User settings
//
// Note: You can also use an optional 'user.ini' file.  This will externalize 
//       settings outside of the code, is already added to .gitignore,
//       and provides advanced privacy and encryption options.  See Vault
//       class for details.
// ****************************************************************************

// WiFi Credentials
static const char *SSID                  = "SSID Goes Here";
static const char *WIFI_PWD              = "WiFi Password Goes Here";

// Spotify Credentials
static const char *SPOTIFY_CLIENT_ID     = "SPOTIFY_CLIENT_ID goes here";
static const char *SPOTIFY_CLIENT_SECRET = "SPOTIFY_CLIENT_SECRET goes here";

// Timezone
// Europe/Zurich as per
// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// format specifiers: https://cplusplus.com/reference/ctime/strftime/
// values below are tested. updating the formatting may affect UI layout.

  // Set to true to use US date/time formatting (12 hour times and US dates)
  static const bool Use_US_Date_Time_Format = false;

  // Formatting consistent with norms in the United States
  #define UI_DATE_FORMAT_US "%A %B %d %Y"
  #define UI_TIME_FORMAT_US "%l:%M %p"

  // Formatting consistent with many countries outside the United States
  #define UI_DATE_FORMAT "%A %d %B %Y"
  #define UI_TIME_FORMAT "%H:%M"

/*
** ===================================================================
** Version and Name Information
** ===================================================================
*/
constexpr const char* APP_NAME     = "ESP32 Spotify Remote";
constexpr const char* VERSION      = "2.0.0";
constexpr const char* COMPILE_TIME = SC_COMPILE_TIME;


/*
** ===================================================================
** System Settings - Per ThingPulse, do not modify unless you 
**                   understand what you are doing!
** ===================================================================
*/

// 2: portrait, on/off switch right side -> 0/0 top left
// 3: landscape, on/off switch at the top -> 0/0 top left
#define TFT_ROTATION 3 
// all other TFT_xyz flags are defined in platformio.ini as PIO build flags

#define TOUCH_ROTATION 0
#define TOUCH_SENSITIVITY 40
#define TOUCH_SDA 23
#define TOUCH_SCL 22
// Initial LCD Backlight brightness
#define TFT_LED_BRIGHTNESS 200

#define SYSTEM_TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"
#define CLOCK_TASK_INTERVAL_MILLIS 3600000

