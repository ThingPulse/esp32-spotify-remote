/*-------------------------------------------------------------------------------------------------
**
** util.h
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


#pragma once

#include "ThingPulse/util.h"  
#include "time.h"
#include "settings.h"

/*
** ===================================================================
** getCurrentTimestamp() - Answer the current timestamp as a String
** ===================================================================
*/
String getCurrentTimestamp(const char* format);

/*
** ===================================================================
** initTime() - initialize time from pool.ntp.org
** ===================================================================
*/
boolean initTime();

/*
** ===================================================================
** logBanner() - log the app and version in the log as info entries
** ===================================================================
*/
void logBanner();
/*
** ===================================================================
** logMemoryStats() - log memory stats as info entries
** ===================================================================
*/
void logMemoryStats();

/*
** ===================================================================
** setTimezone() - set timezone
** ===================================================================
*/
void setTimezone(const char* timezone);

/*
** ===================================================================
** setTimezone() - Answer days from epoch
**     Algorithm: http://howardhinnant.github.io/date_algorithms.html
** ===================================================================
*/
int days_from_epoch(int y, int m, int d);

/*
** ===================================================================
** mkgmtime() - Not sure...
**    https://stackoverflow.com/a/58037981/131929
**    aka timegm() but that's already defined in the Weather Station 
**    lib but not accessible
** ===================================================================
*/

time_t mkgmtime(struct tm const *t);

// ================== Electric Diversions added functons ===========================

/*
** ===================================================================
** simpleDecrypt - Performs a simple decryption of provided string 
**                 and key.
**
**  Decryption portion of command line routine used to do simple encryption
**  Note: This is largely Gen AI generated code.  In the event of any weird
**  point/memory/crash behavior dig deeper here.
** ===================================================================
*/

void simpleDecrypt(const char *hex, const char *key, char *output);

/*
** ===================================================================
** truncateString()
**    Ensures a string isn't too long to display.  This probably
** could go to something more general, but leaving it here for now.
** ===================================================================
*/
String truncateString(const char* psz, size_t maxLength);