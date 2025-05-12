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

#include "SCLogger.h"
#include <Arduino.h>
#include <cstdarg>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*
** ===================================================================
** getInstance()
**    Returns the singleton instance of the SCLogger class.
** ===================================================================
*/
SCLogger& SCLogger::getInstance() {
    static SCLogger instance;
    return instance;
}

/*
** ===================================================================
** setLogLevel()
**    Sets the log level for a specific tag.
** ===================================================================
*/
void SCLogger::setLogLevel(const char* tag, esp_log_level_t level) {
    std::lock_guard<std::mutex> lock(mutex);
    tagLevels[tag] = level;
}

/*
** ===================================================================
** getLogLevel()
**    Gets the current log level for a specific tag.
** ===================================================================
*/
esp_log_level_t SCLogger::getLogLevel(const char* tag) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = tagLevels.find(tag);
    if (it != tagLevels.end()) {
        return it->second;
    }
    return ESP_LOG_NONE; // Default to no logging if not set
}

/*
** ===================================================================
** Helper Method: logMessage()
** Logs a message at the specified log level if enabled.
** ===================================================================
*/
void SCLogger::logMessage(esp_log_level_t level, const char* tag, const char* format, va_list args)
{
    // For debugging
    // Serial.printf("SCLogger::logMessage called with level: %d (%d), tag: %s\n", level, getLogLevel(tag), tag);

    // Switched VERBOSE and DEBUG to really use ESP_LOGI
    // so global setting can be raised to 3 and it
    // will still honor SCLogger settings.

    if (level <= getLogLevel(tag)) {
        char buffer[256]; // Adjust size as needed
        vsnprintf(buffer, sizeof(buffer), format, args);

        // Log the message at the appropriate ESP_LOG level
        // Have Verbose and Debug at I so that they will 
        // show if enabled and system logging is surpressed
        // at that level.
        switch (level) {
            case ESP_LOG_VERBOSE:
                ESP_LOGI(tag, "%s[V] (%s)", buffer, pcTaskGetTaskName(nullptr));
                break;
            case ESP_LOG_DEBUG:
                ESP_LOGI(tag, "%s[D] (%s)", buffer, pcTaskGetTaskName(nullptr));
                break;
            case ESP_LOG_INFO:
                ESP_LOGI(tag, "%s[I] (%s)", buffer, pcTaskGetTaskName(nullptr));
                break;
            case ESP_LOG_WARN:
                ESP_LOGW(tag, "***>> %s[W] (%s)", buffer, pcTaskGetTaskName(nullptr));
                break;
            case ESP_LOG_ERROR:
                ESP_LOGE(tag, "***>> %s[E] (%s)", buffer, pcTaskGetTaskName(nullptr));
                break;
            default:
                break;
        }
    }
}

/*
** ===================================================================
** logVerbose()
** ===================================================================
*/
void SCLogger::logVerbose(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logMessage(ESP_LOG_VERBOSE, tag, format, args);
    va_end(args);
}

/*
** ===================================================================
** logDebug()
** ===================================================================
*/
void SCLogger::logDebug(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logMessage(ESP_LOG_DEBUG, tag, format, args);
    va_end(args);
}

/*
** ===================================================================
** logInfo()
** ===================================================================
*/
void SCLogger::logInfo(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logMessage(ESP_LOG_INFO, tag, format, args);
    va_end(args);
}

/*
** ===================================================================
** logWarn()
** ===================================================================
*/
void SCLogger::logWarn(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logMessage(ESP_LOG_WARN, tag, format, args);
    va_end(args);
}

/*
** ===================================================================
** logError()
** ===================================================================
*/
void SCLogger::logError(const char* tag, const char* format, ...) {
    va_list args;
    va_start(args, format);
    logMessage(ESP_LOG_ERROR, tag, format, args);
    va_end(args);
}