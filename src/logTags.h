/*-------------------------------------------------------------------------------------------------
**
** logTags.h
**
**    Defines logging tags for various modules in the project. Centralizing
**    logging tags ensures consistency and supports structured log filtering
**    during development and diagnostics.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-01-03 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#pragma once

#define LOGTAG_GENERAL         "General"     // General logging
#define LOGTAG_GUI             "GUI"         // Logging info
#define LOGTAG_INPUT           "Input"       // Input related
#define LOGTAG_MULTITASK       "Multitask"   // General logging
#define LOGTAG_PLAYER          "Player"      // Spotify playing
#define LOGTAG_SONG_DATA       "SongData"    // Current song data
#define LOGTAG_METRICS         "Metrics"     // Timings
#define LOGTAG_HEAP            "Heap"        // Heap
#define LOGTAG_TRACE           "Trace"       // Explicit tracing for troubleshooting
#define LOGTAG_FILEIO          "FileIO"      // File IO Library
#define LOGTAG_CACHE           "Cache"       // Album Cache Logging
#define LOGTAG_DISPLAY_MODE    "DispMode"    // Display Modes
#define LOGTAG_VAULT           "Vault"       // Credential Vaulting

/*
** ===================================================================
** ESP-IDF Log Macros
** These macros are used to log messages at different severity levels.
** Each macro allows for formatted output and optionally supports tags 
** for categorizing log messages.
**
** log_e: Error level, used for serious issues or failures.
** log_w: Warning level, used for recoverable issues or potential problems.
** log_i: Info level, used for general informational messages.
** log_d: Debug level, used for detailed debug information during development.
** log_v: Verbose level, used for very detailed and granular debug information.
** ===================================================================
*/
