// SPDX-FileCopyrightText: 2023 ThingPulse Ltd., https://thingpulse.com
// SPDX-License-Identifier: MIT

#pragma once

#include "time.h"
#include "settings.h"

char timestampBuffer[26];


String getCurrentTimestamp(const char* format) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    log_e("Failed to obtain time.");
    return "";
  }
  strftime(timestampBuffer, sizeof(timestampBuffer), format, &timeinfo);
  return String(timestampBuffer);
}

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

void logBanner() {
  log_i("**********************************************");
  log_i("* ThingPulse Spotify Controller v%s *", VERSION);
  log_i("**********************************************");
}

void logMemoryStats() {
  log_i("Total heap: %d", ESP.getHeapSize());
  log_i("Free heap: %d", ESP.getFreeHeap());
  log_i("Total PSRAM: %d", ESP.getPsramSize());
  log_i("Free PSRAM: %d", ESP.getFreePsram());
}

void setTimezone(const char* timezone) {
  log_i("Setting timezone to '%s'.", timezone);
  // Clock settings are adjusted to show the new local time
  setenv("TZ", timezone, 1);
  tzset();
}

// Algorithm: http://howardhinnant.github.io/date_algorithms.html
int days_from_epoch(int y, int m, int d) {
  y -= m <= 2;
  int era = y / 400;
  int yoe = y - era * 400;                                  // [0, 399]
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;          // [0, 146096]
  return era * 146097 + doe - 719468;
}
// https://stackoverflow.com/a/58037981/131929
// aka timegm() but that's already defined in the Weather Station lib but not accessible
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
