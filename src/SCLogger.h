/*-------------------------------------------------------------------------------------------------
**
** SCLogger.h
**
**    A singleton logging utility that wraps ESP_LOGx macros. Allows dynamic
**    log level configuration per tag and provides formatted output with
**    file and line references. Intended as a tactical workaround for tag-based
**    log filtering issues.  Created due to not being able to get log_x and ESP_LOGx
**    macros to respect the tags and levels.  
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

#include <esp_log.h>
#include <string>
#include <map>
#include <mutex>

/*
** ===================================================================
** SCLogger
** ===================================================================
*/
class SCLogger {
public:
    /*
    ** ===================================================================
    ** getInstance()
    **    Returns the singleton instance of the SCLogger class.
    ** ===================================================================
    */
    static SCLogger& getInstance();

    /*
    ** ===================================================================
    ** setLogLevel()
    **    Sets the log level for a specific tag.
    ** Parameters:
    **    tag - The tag for which the log level is being set.
    **    level - The log level to set (ESP_LOG_VERBOSE, ESP_LOG_INFO, etc.).
    ** ===================================================================
    */
    void setLogLevel(const char* tag, esp_log_level_t level);

    /*
    ** ===================================================================
    ** getLogLevel()
    **    Gets the current log level for a specific tag.
    ** Parameters:
    **    tag - The tag for which to get the log level.
    ** Returns:
    **    The current log level for the specified tag.
    ** ===================================================================
    */
    esp_log_level_t getLogLevel(const char* tag);

    /*
    ** ===================================================================
    ** Logging Methods
    ** Logs a message at the specified log level if the level is enabled
    ** for the given tag.
    ** ===================================================================
    */
    void logVerbose(const char* tag, const char* format, ...);
    void logDebug(const char* tag, const char* format, ...);
    void logInfo(const char* tag, const char* format, ...);
    void logWarn(const char* tag, const char* format, ...);
    void logError(const char* tag, const char* format, ...);

private:
    SCLogger() = default;                             // Private constructor
    SCLogger(const SCLogger&) = delete;               // Prevent copy
    SCLogger& operator=(const SCLogger&) = delete;    // Prevent assignment
    void logMessage(esp_log_level_t level, const char* tag, const char* format, va_list args);
    std::map<std::string, esp_log_level_t> tagLevels; // Map to store log levels per tag
    std::mutex mutex;                                 // Mutex to protect the map
};
// Define this to enable logging; comment out to disable
#define ENABLE_SCLOGGING

// Define macros for logging
#ifdef ENABLE_SCLOGGING

#define spLogV(tag, format, ...) SCLogger::getInstance().logVerbose(tag, format " [%s:%d]", ##__VA_ARGS__, __FILE__, __LINE__)
#define spLogD(tag, format, ...) SCLogger::getInstance().logDebug(tag, format " [%s:%d]", ##__VA_ARGS__, __FILE__, __LINE__)
#define spLogI(tag, format, ...) SCLogger::getInstance().logInfo(tag, format " [%s:%d]", ##__VA_ARGS__, __FILE__, __LINE__)
#define spLogW(tag, format, ...) SCLogger::getInstance().logWarn(tag, format " [%s:%d]", ##__VA_ARGS__, __FILE__, __LINE__)
#define spLogE(tag, format, ...) SCLogger::getInstance().logError(tag, format " [%s:%d]", ##__VA_ARGS__, __FILE__, __LINE__)

#else // If logging is disabled

#define spLogV(tag, ...) (void)0
#define spLogD(tag, ...) (void)0
#define spLogI(tag, ...) (void)0
#define spLogW(tag, ...) (void)0
#define spLogE(tag, ...) (void)0

#endif // ENABLE_SCLOGGING