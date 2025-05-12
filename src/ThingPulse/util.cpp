/*-------------------------------------------------------------------------------------------------
**
** util.cpp
**
**    Utility routines from ThingPulse. Includes time synchronization, logging,
**    timezone handling, memory diagnostics, and general-purpose helpers for
**    the ESP32 Spotify Remote.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2024-12-26 - Electric Diversions - Copied and renamed to tpUtil.h from util.h
**    2024-12-27 - Electric Diversions - Added source file and split out function declarations
**    2025-05-04 - Electric Diversions - Renamed back to util and moved to ThingPulse folder
** ------------------------------------------------------------------------------------------------
*/

#include <Arduino.h> // Brings in ESP32 logging and standard C library includes

#include "util.h"
#include "time.h"
#include "settings.h"

// Buffer to hold timestamp
char timestampBuffer[26];

/*
** ===================================================================
** getCurrentTimestamp() - Answer the current timestamp as a String
** ===================================================================
*/
String getCurrentTimestamp(const char* format) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    log_e("Failed to obtain time.");
    return "";
  }
  strftime(timestampBuffer, sizeof(timestampBuffer), format, &timeinfo);
  return String(timestampBuffer);
}

/*
** ===================================================================
** initTime() - initialize time from pool.ntp.org
** ===================================================================
*/
boolean initTime() {
  struct tm timeinfo;

  log_i("Synchronizing time.");
  // Connect to NTP server with 0 TZ offset, call setTimezone() later
  configTime(0, 0, "pool.ntp.org");
  // getLocalTime() uses a default timeout of 5s -> the loop takes at most 3*5s to
  for (int i = 0; i < 3; i++) {
    if (getLocalTime(&timeinfo)) {
      log_i("UTC time: %s.", getCurrentTimestamp(SYSTEM_TIMESTAMP_FORMAT).c_str());
      return true;
    }
  }

  log_e("Failed to obtain time.");
  return false;
}

/*
** ===================================================================
** logBanner() - log the app and version in the log as info entries
** ===================================================================
*/
void logBanner() {
  log_i("==============================================");
  log_i("* Spotify Companion v%s *", VERSION);
  log_i("* settings.h compile time: %s", COMPILE_TIME);
  log_i("==============================================");
}

/*
** ===================================================================
** logMemoryStats() - log memory stats as info entries
** ===================================================================
*/
void logMemoryStats() {
  log_i("Total heap: %d", ESP.getHeapSize());
  log_i("Free heap: %d", ESP.getFreeHeap());
  log_i("Total PSRAM: %d", ESP.getPsramSize());
  log_i("Free PSRAM: %d", ESP.getFreePsram());
}

/*
** ===================================================================
** setTimezone() - set timezone
** ===================================================================
*/
void setTimezone(const char* timezone) {
  log_i("Setting timezone to '%s'.", timezone);
  // Clock settings are adjusted to show the new local time
  setenv("TZ", timezone, 1);
  tzset();
}

/*
** ===================================================================
** setTimezone() - Answer days from epoch
**     Algorithm: http://howardhinnant.github.io/date_algorithms.html
** ===================================================================
*/
int days_from_epoch(int y, int m, int d) {
  y -= m <= 2;
  int era = y / 400;
  int yoe = y - era * 400;                                  // [0, 399]
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;          // [0, 146096]
  return era * 146097 + doe - 719468;
}

/*
** ===================================================================
** mkgmtime() - Not sure...
**    https://stackoverflow.com/a/58037981/131929
**    aka timegm() but that's already defined in the Weather Station 
**    lib but not accessible
** ===================================================================
*/

time_t mkgmtime(struct tm const *t) {
  int year = t->tm_year + 1900;
  int month = t->tm_mon; // 0-11
  if (month > 11) {
    year += month / 12;
    month %= 12;
  } else if (month < 0) {
    int years_diff = (11 - month) / 12;
    year -= years_diff;
    month += 12 * years_diff;
  }
  int days_since_epoch = days_from_epoch(year, month + 1, t->tm_mday);

  return 60 * (60 * (24L * days_since_epoch + t->tm_hour) + t->tm_min) + t->tm_sec;
}

// ================== Electric Diversions added functons ===========================

/*
** ===================================================================
** simpleDecrypt - Performs a simple decryption of provided string 
**                 and key.
**
** Replace logic with decryption routing of choice.  By default,
** this will just return back the same value that was passed
** in unmodified.
**
** ===================================================================
*/

void simpleDecrypt(const char *hex, const char *key, char *output) {
  (void)key;  // Mark key as unused to avoid compiler warnings
  strcpy(output, hex);
}

/*
** ===================================================================
** truncateString()
**    Ensures a string isn't too long to display.  This probably
** could go to something more general, but leaving it here for now.
** ===================================================================
*/
String truncateString(const char* psz, size_t maxLength) 
{
    // Check if the input string is shorter than or equal to maxLength
    size_t inputLength = strlen(psz);
    if (inputLength <= maxLength) {
        return String(psz); // No truncation needed
    }

    // Calculate the maximum length for characters before adding "..."
    size_t truncatedLength = maxLength > 3 ? maxLength - 3 : 0;

    // Create a new String object with the truncated content and append "..."
    String result = String(psz).substring(0, truncatedLength);
    result += "...";

    return result; // Return the truncated string
}