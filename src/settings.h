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
// ****************************************************************************

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
** Time Zone to use
** ===================================================================
**
** timezone Europe/Zurich as per 
** https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
*/
#define TIMEZONE "CST6CDT,M3.2.0,M11.1.0"

/*
** ===================================================================
** System Settings - Per ThingPulse, do not modify unless you 
**                   understand what you are doing!
** ===================================================================
*/

// 2: portrait, on/off switch right side -> 0/0 top left
// 3: landscape, on/off switch at the top -> 0/0 top left
#define TFT_ROTATION 3  //2
// all other TFT_xyz flags are defined in platformio.ini as PIO build flags

// 0: portrait, on/off switch right side -> 0/0 top left
// 1: landscape, on/off switch at the top -> 0/0 top left
#define TOUCH_ROTATION 0
#define TOUCH_SENSITIVITY 40
#define TOUCH_SDA 23
#define TOUCH_SCL 22
// Initial LCD Backlight brightness
#define TFT_LED_BRIGHTNESS 200

#define SYSTEM_TIMESTAMP_FORMAT "%Y-%m-%d %H:%M:%S"
#define CLOCK_TASK_INTERVAL_MILLIS 3600000

