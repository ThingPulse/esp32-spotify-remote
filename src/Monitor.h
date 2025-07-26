/*-------------------------------------------------------------------------------------------------
**
** Monitor.h
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

#pragma once

#include <map>
#include <string>
#include <chrono>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "SCLogger.h"

#define MONITOR_ID_CALCULATE_AVG_BACKGROUND_COLOR 100
#define MONITOR_ID_SPOTIFY_GET_CURRENTLY_PLAYING  200
#define MONITOR_ID_SPOTIFY_IMAGE_HTTP_GET         300
#define MONITOR_ID_SPOTIFY_IMAGE_FILE_SAVE        310
#define MONITOR_ID_SPOTIFY_IMAGE_FILE_LOAD        320
#define MONITOR_ID_SPOTIFY_IMAGE_CACHE_LOAD       390
#define MONITOR_ID_SPOTIFY_IMAGE_CACHE_SAVE       391
#define MONITOR_ID_FETCH_ALBUM_ART                399
#define MONITOR_ID_SCUI_QUEUE_DELAY               500

// Value was picked after after running multi-hour
// duration test and heap not going lower than  
// 64304.  - 1/19/2025
// 51048.  - 2/14/2025 (after swapping cores, etc.)
#define HEAP_LOW_THRESHOLD   50000
#define HEAP_WATCH_SIZE_BASE 60000

// TODO: clean this up...
#define LLONG_MAX 9223372036854775807LL

/*
** ===================================================================
** MonitorStats
**
** A struct to encapsulate statistics for a specific monitored event.
**
** Fields:
**    id             - The unique monitor ID
**    description    - A readable description of the monitor
**    stopCount      - The number of times the event has been stopped
**    averageMs      - The average duration in milliseconds
**    minMs          - The minimum recorded duration in milliseconds
**    maxMs          - The maximum recorded duration in milliseconds
**
** ===================================================================
*/
struct MonitorStats
{
    int         id;
    std::string description;
    int         stopCount;
    long long   averageMs;
    long long   minMs;
    long long   maxMs;
};

/*
** ===================================================================
** Monitor
**
** A utility class to measure time intervals with support for
** multiple active timers. Each timer is identified by a unique ID.
** Methods are static for utility-style usage.
**
** ===================================================================
*/
class Monitor
{
public:
    // Starts the monitor for a given timer ID
    static void start(int id, const char* tag, const char* msg);

    // Stops the monitor for a given timer ID and logs the elapsed time
    static void stop(int id);

    // Dumps the current stats for all timers to the log
    static void dumpStats(const char* tag);

    // Retrieves statistics for a specific monitor ID.
    static MonitorStats getStats(int monitorID);    

    // Monitors and logs heap size stats
    static bool watchHeap(uint32_t threshold);

    // Answer the current heap size and remember the lowest value
    static uint32_t getFreeHeap();    

    // Monitors and logs heap size stats
    static bool watchQueue(QueueHandle_t queueHandle, uint32_t threshold);    

    // Answer the max monitored queue depth
    static uint32_t getMaxQueueDepth();       

    // Register an ID description
    static void registerDescription(int id, const std::string& description);    

    // Answers the system uptime in a human-readable format
    static std::string getFormattedUptime(const char *pszFormatStr);

    // Logs the system uptime in a human-readable format
    static void logUptime(const char* tag);

    // Struct defining what is tracked for the heap
    struct HeapStats
    {
        uint32_t minHeap     = UINT32_MAX;
        uint32_t maxHeap     = 0;
        uint64_t totalHeap   = 0;
        uint32_t sampleCount = 0;
    };
  
    // Return the current Heap Stats
    static Monitor::HeapStats getHeapStats();    

private:
    struct TimerInfo
    {
        std::chrono::steady_clock::time_point startTime;
        std::string  tag;
        long long    totalDuration = 0;         // Total time in milliseconds
        int          stopCount     = 0;         // Number of times stop() was called
        long long    minDuration   = LLONG_MAX; // Minimum elapsed time
        long long    maxDuration   = 0;         // Maximum elapsed time
    };

    static std::map<int, TimerInfo>   timers;            // Active timers
    static std::map<int, std::string> idDescriptions;    // Readabile descriptions
    static HeapStats                  heapStats;         // Heap stats tracker
    static uint32_t                   maxQueueDepth;
};
