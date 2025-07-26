/*-------------------------------------------------------------------------------------------------
**
** Monitor.cpp
**
**    Utility class for performance and resource monitoring. Provides timing
**    instrumentation, heap tracking, and queue depth monitoring using static
**    methods and internal tracking structures.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-01-07 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include "Monitor.h"
#include "SCLogger.h"
#include "logTags.h"
#include <Arduino.h>

// Initialize the static timers map
std::map<int, Monitor::TimerInfo> Monitor::timers;

// Initialize id descriptors
std::map<int, std::string> Monitor::idDescriptions;

// Initialize the static HeapStats struct
Monitor::HeapStats Monitor::heapStats;


uint32_t Monitor::maxQueueDepth = 0; // Max queue depth

/*
** ===================================================================
** registerDescription()
**    Associates a human-readable description with a monitor ID.
**    This description is included in the statistics dump for better
**    clarity and context.
**
** Parameters:
**    id          - Unique identifier for the timer.
**    description - A human-readable string describing the purpose
**                  or context of the timer.
**
** Example Usage:
**    Monitor::registerDescription(MONITOR_ID_SPOTIFY_GET_CURRENTLY_PLAYING,
**                                 "Spotify: Get Currently Playing");
**
** Notes:
**    - Descriptions should be registered early in the application's
**      lifecycle, such as during initialization.
**    - If a description is not registered for an ID, "Unknown" will
**      appear in the statistics dump for that ID.
**
** ===================================================================
*/
void Monitor::registerDescription(int id, const std::string& description)
{
    idDescriptions[id] = description;
}

/*
** ===================================================================
** start()
**    Starts the monitor for a given timer ID.
**
** Parameters:
**    id  - Unique identifier for the timer
**    tag - Logging tag for the timer
**    msg - Message to log when starting the timer
**
** ===================================================================
*/
void Monitor::start(int id, const char* tag, const char* msg)
{
    TimerInfo& info = timers[id];
    info.startTime = std::chrono::steady_clock::now();
    info.tag = tag;

    spLogV(tag, "Monitor started (ID: %d). %s", id, msg);
}

/*
** ===================================================================
** stop()
**    Stops the monitor for a given timer ID and logs the elapsed time.
**
** Parameters:
**    id  - Unique identifier for the timer
**
** ===================================================================
*/
void Monitor::stop(int id)
{
    auto it = timers.find(id);
    if (it != timers.end())
    {
        // Extract tag and start time
        TimerInfo& info = it->second;
        const std::string& tag = info.tag;
        auto startTime = info.startTime;

        // Calculate the elapsed time
        auto endTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

        // Update statistics
        info.totalDuration += elapsed;
        info.stopCount++;
        if (elapsed < info.minDuration) info.minDuration = elapsed;
        if (elapsed > info.maxDuration) info.maxDuration = elapsed;

        // Calculate the average duration
        long long avgDuration = info.totalDuration / info.stopCount;

        // Log the results
        spLogV(tag.c_str(), "Monitor stopped (ID: %d). Elapsed: %lld ms. Avg: %lld ms. Min: %lld ms. Max: %lld ms.",
               id, elapsed, avgDuration, info.minDuration, info.maxDuration);
    }
    else
    {
        spLogW(LOGTAG_GENERAL, "Monitor stop called for an unknown ID: %d", id);
    }
}

/*
** ===================================================================
** dumpStats()
**    Logs a table of statistics for all currently tracked timers.
**
** Parameters:
**    tag - Logging tag for the summary log entry
**
** ===================================================================
*/
void Monitor::dumpStats(const char* tag)
{
    // Define column widths for alignment
    constexpr int idWidth = 6;         // ID column width
    constexpr int descWidth = 20;      // Description column width
    constexpr int countWidth = 12;     // Stop Count column width
    constexpr int avgWidth = 10;       // Avg(ms) column width
    constexpr int minWidth = 7;        // Min column width
    constexpr int maxWidth = 7;        // Max column width

    spLogI(tag, "Monitor Statistics Dump:");
    spLogI(tag, "---------------------------------------------------------------------------------");
    spLogI(tag, "| %*s | %-*s | %*s | %*s | %*s | %*s |",
           idWidth, "ID",
           descWidth, "Description",
           countWidth, "Stop Count",
           avgWidth, "Avg(ms)",
           minWidth, "Min",
           maxWidth, "Max");
    spLogI(tag, "---------------------------------------------------------------------------------");

    for (const auto& entry : timers)
    {
        const int id = entry.first;
        const TimerInfo& info = entry.second;

        // Get the description from idDescriptions, or use "Unknown ID" if not found
        const std::string& description = idDescriptions.count(id) ? idDescriptions.at(id) : "Unknown ID";

        if (info.stopCount > 0)
        {
            long long avgDuration = info.totalDuration / info.stopCount;

            spLogI(tag, "| %*d | %-*s | %*d | %*lld | %*lld | %*lld |",
                   idWidth, id,
                   descWidth, description.c_str(),
                   countWidth, info.stopCount,
                   avgWidth, avgDuration,
                   minWidth, info.minDuration,
                   maxWidth, info.maxDuration);
        }
        else
        {
            spLogI(tag, "| %*d | %-*s | %*d | %*s | %*s | %*s |",
                   idWidth, id,
                   descWidth, description.c_str(),
                   countWidth, info.stopCount,
                   avgWidth, "N/A",
                   minWidth, "N/A",
                   maxWidth, "N/A");
        }
    }

    spLogI(tag, "---------------------------------------------------------------------------------");
}

/*
** ===================================================================
** getStats()
**    Retrieves statistics for a specific monitor ID.
**
** Parameters:
**    monitorID - The unique ID of the monitor.
**
** Returns:
**    MonitorStats struct containing:
**    - ID
**    - Description (if registered, otherwise "Unknown ID")
**    - Stop count
**    - Average duration (ms)
**    - Min duration (ms)
**    - Max duration (ms)
**
** Notes:
**    - If the monitor ID is not found, it returns default values.
**
** ===================================================================
*/
MonitorStats Monitor::getStats(int monitorID)
{
    MonitorStats stats = {monitorID, "Unknown ID", 0, 0, LLONG_MAX, 0};

    auto it = timers.find(monitorID);
    if (it != timers.end())
    {
        const TimerInfo& info = it->second;

        stats.description = idDescriptions.count(monitorID) ? idDescriptions.at(monitorID) : "Unknown ID";
        stats.stopCount   = info.stopCount;
        stats.minMs       = info.minDuration;
        stats.maxMs       = info.maxDuration;

        // Calculate the average only if stopCount > 0 to avoid division by zero
        stats.averageMs = (info.stopCount > 0) ? (info.totalDuration / info.stopCount) : 0;
    }

    spLogV(LOGTAG_METRICS, "Returned ID: %s Avg: %lld", stats.description.c_str(),stats.averageMs);
    return stats;
}


/*
** ===================================================================
** watchHeap()
**    Logs the heap size and alerts if it falls below a threshold.
**
** Parameters:
**    threshold - The heap size threshold to warn about
**
** Returns:
**    true if over the heap threshold, false is under it
**
** ===================================================================
*/
bool Monitor::watchHeap(uint32_t threshold)
{
    // Get the current heap size
    uint32_t currentHeap = ESP.getFreeHeap();
    bool     bAnswer     = true;

    // Update stats
    if (currentHeap < heapStats.minHeap)
    {
        heapStats.minHeap = currentHeap;
    } 

    if (currentHeap > heapStats.maxHeap) 
    {
        heapStats.maxHeap = currentHeap;
    }

    heapStats.totalHeap += currentHeap;
    heapStats.sampleCount++;

    uint32_t avgHeap = heapStats.sampleCount > 0 ? heapStats.totalHeap / heapStats.sampleCount : 0;

    // Log the current heap size
    spLogV(LOGTAG_HEAP,"                                                                                ");
    spLogI(LOGTAG_HEAP, "Heap size: %u bytes. Avg: %u bytes. Min: %u bytes. Max: %u bytes.",
           currentHeap, avgHeap, heapStats.minHeap, heapStats.maxHeap);
    spLogV(LOGTAG_HEAP,"                                                                                ");

    // Warn if the heap size is below the threshold
    if (currentHeap < threshold)
    {
        spLogW(LOGTAG_HEAP, "Heap size below threshold! Current: %u bytes, Threshold: %u bytes.",
               currentHeap, threshold);
        bAnswer = false;
    }
    return bAnswer;
}

/*
** ===================================================================
** watchHeap()
**    Logs the heap size and alerts if it falls below a threshold.
**
** Parameters:
**    threshold - The heap size threshold to warn about
**
** Returns:
**    true if over the heap threshold, false is under it
**
** ===================================================================
*/
bool Monitor::watchQueue(QueueHandle_t queueHandle, uint32_t threshold)
{
    // Get the current queue depth
    UBaseType_t messagesInQueue = uxQueueMessagesWaiting(queueHandle);
    bool        bAnswer         = false;

    if (messagesInQueue > threshold)
    {
        spLogV(LOGTAG_GENERAL, "*** queue count over threshold of %u. count: %u", threshold, messagesInQueue);
        bAnswer = true;
    }

    if (messagesInQueue > maxQueueDepth)
    {
        maxQueueDepth = messagesInQueue;
    }

    return bAnswer;
}

uint32_t Monitor::getMaxQueueDepth()
{
    return maxQueueDepth;
}

/*
** -------------------------------------------------------------------
** getFreeHeap
**
** Returns the current heap size and remember the minimum heap.
**
** -------------------------------------------------------------------
*/
uint32_t Monitor::getFreeHeap()
{
   // Get the current heap size
    uint32_t currentHeap = ESP.getFreeHeap();

    // Update stats
    if (currentHeap < heapStats.minHeap)
    {
        heapStats.minHeap = currentHeap;
    } 

    return currentHeap;
}

/*
** -------------------------------------------------------------------
** getHeapStatus
**
** Returns a copy of the current heap statistics.
**
** Returns:
**    HeapStats - A struct containing heap information.
**
** -------------------------------------------------------------------
*/
Monitor::HeapStats Monitor::getHeapStats()
{
    return heapStats;
}

/*
/*
** -------------------------------------------------------------------
** getFormattedUptime
**
** Returns the system uptime as a formatted string based on the 
** provided format string.
**
** Parameters:
**    formatStr - A C-style format string that specifies the output 
**                format. Must contain four unsigned long format 
**                specifiers (e.g., "%lu") for days, hours, minutes, 
**                and seconds.
**
** Format Example:
**    "System Uptime: %lu days, %lu hours, %lu minutes, %lu seconds."
**    "Uptime: %luD %luH %luM %luS"
**
** Returns:
**    std::string - A human-readable string representation of uptime.
**
** Notes:
**    - The caller must ensure `formatStr` is a valid null-terminated 
**      format string with appropriate placeholders.
**    - If `formatStr` is null, a default format will be used.
**
** -------------------------------------------------------------------
*/
std::string Monitor::getFormattedUptime(const char *pszFormatStr)
{

    // Get the current uptime in milliseconds
    unsigned long uptimeMillis = millis();

    // Calculate days, hours, minutes, and seconds
    unsigned long seconds = (uptimeMillis / 1000) % 60;
    unsigned long minutes = (uptimeMillis / (1000 * 60)) % 60;
    unsigned long hours   = (uptimeMillis / (1000 * 60 * 60)) % 24;
    unsigned long days    = (uptimeMillis / (1000 * 60 * 60 * 24));

    char buffer[100] = {0};
    snprintf(buffer, sizeof(buffer), pszFormatStr,
             static_cast<unsigned long>(days),
             static_cast<unsigned long>(hours),
             static_cast<unsigned long>(minutes),
             static_cast<unsigned long>(seconds));    

    return std::string(buffer);

}

/*
** ===================================================================
** logUptime()
**    Logs the system uptime in a human-readable format (days, hours,
**    minutes, and seconds).
**
** Parameters:
**    tag - Logging tag for the uptime log entry
**
** Notes:
**    - Uses millis() to calculate the system uptime.
**    - Wraps around after approximately 49.7 days due to the size of
**      the unsigned long returned by millis().
**
** Example Output:
**    [General] System Uptime: 3 days, 4 hours, 23 minutes, 15 seconds.
**
** ===================================================================
*/
void Monitor::logUptime(const char* tag)
{
    // Log the uptime
    spLogI(tag, "%s", Monitor::getFormattedUptime("System Uptime: %lu days, %lu hours, %lu minutes, %lu seconds.").c_str());

}