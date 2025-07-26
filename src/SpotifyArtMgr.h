/*-------------------------------------------------------------------------------------------------
**
** SpotifyArtMgr.h
**
**    Handles downloading and caching of Spotify album art. Ensures reliable
**    display by managing secure certificate validation and efficient image
**    caching with LRU eviction. Designed as a Singleton for global access.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-01-11 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#pragma once

#include <map>
#include <list>
#include <queue>
#include <Arduino.h>
#include <HTTPClient.h>

#define SP_NO_COVER_JPG_FILENAME "/DefaultCoverArt.jpg"

/*
** ===================================================================
** ArtMgrStats
**
** A struct to encapsulate statistics related to album art fetching 
** and cache performance.
**
** Fields:
**    totalFetches     - The total number of album art fetch attempts
**    cacheHits        - The number of successful cache hits
**    currentHitStreak - The current streak of consecutive cache hits
**    longestHitStreak - The longest recorded streak of cache hits
**    averageHitRate   - The average cache hit rate over time
**
** Notes:
**    - This struct is used to track the effectiveness of caching
**      and provide insights into network requests vs. local cache use.
**    - `cacheHits` should always be ≤ `totalFetches`.
**    - The hit streak fields help identify caching efficiency trends.
**
** Example Usage:
**    ArtMgrStats stats = SpotifyArtMgr::getInstance().getStats();
**    spLogI(LOGTAG_GENERAL, "Cache Hit Rate: %lld%%", stats.averageHitRate);
**
** ===================================================================
*/
struct ArtMgrStats
{
    long long   totalFetches     = 0;
    long long   cacheHits        = 0;
    long long   currentHitStreak = 0;
    long long   longestHitStreak = 0;
    long long   averageHitRate   = 0;
};

/*
** ===================================================================
** SpotifyArtMgr
**
** Singleton class to manage downloading and caching of album art.
** Maintains a mapping of URLs to local file names. The cache has a 
** configurable maximum size and follows a FIFO replacement policy 
** when full.
** ===================================================================
*/
class SpotifyArtMgr
{
public:
    static SpotifyArtMgr* getInstance();

    void        setMaxCacheSize(size_t maxSize);
    size_t      getMaxCacheSize();
    void        saveCacheIndex();
    bool        isCacheDirty();

    ArtMgrStats getStats();

    String      acquireAlbumArt(const String& url);   
    String      getLocalFileName(const String& url);

private:
    // Constructor (private for Singleton pattern)
    SpotifyArtMgr();

    // Private methods
    void     determineCacheSize();
    void     loadCacheIndex();
    bool     downloadFile(String url, String filename);
    String   generateLocalFileName();
    void     evictOldestFile();
    void     saveFileFromHTTPResponse(String filename, HTTPClient *pHttp );
    void     printCacheIndex(const char* tag);
    uint32_t calculateChecksum(const char* data, size_t length);

    // Singleton instance
    static SpotifyArtMgr*    gInstance;

    // Cache configuration
    size_t                   _maxCacheSize = 10;    // Default maximum cache size

    SemaphoreHandle_t xSemaphoreArtFetchCompleted = xSemaphoreCreateMutex();;
    bool              _isDownloadPending          = false;

    // Data structures for caching
    std::list<String>        _cacheList;            // Maintain LRU order
    std::map<String, String> _urlToFileMap;         // Map URLs to list iterators
    size_t                   _nextFileId   = 1;       // Counter for generating file names
    bool                     _isDirty      = false;   // Indicate whether there is something new to save
    ArtMgrStats              _stats;
    bool                     _isHitStreakInProgress = false;
};