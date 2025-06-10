/*-------------------------------------------------------------------------------------------------
**
** SpotifyArtMgr.cpp
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

#include "SpotifyArtMgr.h"
#include "SCLogger.h"
#include "logTags.h"
#include "Monitor.h"
#include "SCFileIO.h"
#include <LittleFS.h>

/*
** ===================================================================
** Certificates for Secure Spotify Image Downloads
**
** DigiCert Global Root G2 Certificate:
**      Source: https://www.digicert.com/kb/digicert-root-certificates.htm
**
** GlobalSign Root R2 Certificate:
**      Source: https://www.globalsign.com/en/repository
**      Command to convert the downloaded CRT file to PEM format:
**          zsh> openssl x509 -inform DER -in Root-R3.crt -out GlobalSignRoot.pem
**
** Context:
**      Spotify's image server (i.scdn.co) occasionally returns certificates 
**      signed by either the DigiCert Global Root G2 or the GlobalSign Root R2 
**      certificate authorities. Previously, the system was configured to trust 
**      only the DigiCert Global Root G2 certificate. When the server returned 
**      a GlobalSign certificate, the connection would fail due to an 
**      unrecognized CA.
**
** Purpose:
**      This implementation includes both the DigiCert Global Root G2 and 
**      GlobalSign Root R2 certificates to ensure compatibility with the 
**      Spotify image server, regardless of which CA is used for signing. The 
**      certificates are programmatically combined and passed to WiFiClientSecure 
**      for validation.
**
** Notes:
**      - The certificates were last pulled and verified on 1/12/2025.
**      - If future errors arise, verify both certificates for validity and 
**        ensure they are updated as needed.
**      - Use `WiFiClientSecure::setCACert()` to set the combined certificates 
**        for secure HTTPS connections.
**
** Maintenance:
**      1. Regularly check the above URLs for updated root certificates.
**      2. Use the OpenSSL command provided above to convert new certificates 
**         to PEM format.
**      3. Update this code with any new certificates and adjust their 
**         combination logic if necessary.
**
** SPDX-License-Identifier: MIT
** ===================================================================
*/
const char* gCombinedCerts = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
-----BEGIN CERTIFICATE-----
MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G
A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp
Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4
MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG
A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI
hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8
RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT
gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm
KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd
QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ
XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw
DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o
LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU
RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp
jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK
6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX
mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs
Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH
WD9f
-----END CERTIFICATE-----
)EOF";

// Initialize the singleton instance
SpotifyArtMgr* SpotifyArtMgr::gInstance = nullptr;

const char*   gCacheFileroot = "/cache_index_v5";
// const size_t  gJSONDocSize   = 16384;  //8192; 

/*
** ===================================================================
** Constructor
**    Initializes the SpotifyArtMgr object.
** ===================================================================
*/
SpotifyArtMgr::SpotifyArtMgr()
{
    // Initialize anything as needed (currently empty)
}

/*
** ===================================================================
** getInstance()
**    Returns the singleton instance of the SpotifyArtMgr class.
** ===================================================================
*/
SpotifyArtMgr* SpotifyArtMgr::getInstance()
{

    if (gInstance == nullptr)
    {
        gInstance = new SpotifyArtMgr();
        // figure out if configured with more than 4MB of file space
        gInstance->determineCacheSize();
        // populate the index to last run
        String filePath = String(gCacheFileroot) + ".txt";
        SCFileIO::getInstance().hexDump(LOGTAG_CACHE, filePath.c_str());
        Monitor::start(MONITOR_ID_SPOTIFY_IMAGE_CACHE_LOAD, LOGTAG_CACHE, "loadCacheIndex()");
        gInstance->loadCacheIndex();
        Monitor::stop(MONITOR_ID_SPOTIFY_IMAGE_CACHE_LOAD);
    }
    return gInstance;
}



/*
** ===================================================================
** getLocalFileName()
**    Retrieves the local file name for a given URL. Checks an internal
**    cache to see if the URL is already known. If found, returns the
**    cached file name; otherwise returns a fallback image name.
**
** Parameters:
**    url - The URL whose corresponding file name should be looked up.
**
** Returns:
**    String containing the cached file name if available, or a fallback
**    image file name if not found in the cache.
** ===================================================================
*/

String SpotifyArtMgr::getLocalFileName(const String& url)
{
    spLogD(LOGTAG_CACHE, "getLocalFileName() for URL: %s", url.c_str());

    // -------------------------------------------------------------------
    // If a download is pending, delay in small increments until either
    // it clears or 3 seconds have elapsed.
    // -------------------------------------------------------------------
    if (_isDownloadPending)
    {
        // const int totalWaitMs = 3000;
        // const int stepMs      = 100;
        // int       waitedMs    = 0;

        // while (_isDownloadPending && waitedMs < totalWaitMs)
        // {
        //     vTaskDelay(pdMS_TO_TICKS(stepMs));
        //     waitedMs += stepMs;
        // }
    }

    auto it = _urlToFileMap.find(url);
    if (it != _urlToFileMap.end())
    {
        // The iterator points to an entry in _cacheList; access the actual value
        String fileName = it->second;     // Dereference the iterator to get the cached URL
        spLogD(LOGTAG_CACHE, "Returning cached file name: %s", fileName.c_str());
        return fileName; // Return the cached file name
    }

    spLogD(LOGTAG_CACHE, "URL not found in cache. Returning fallback image: %s", SP_NO_COVER_JPG_FILENAME);
    return SP_NO_COVER_JPG_FILENAME; // Return fallback image
}

/*
** ===================================================================
** acquireAlbumArt()
**    Retrieves album art for the specified URL. Checks whether the URL
**    is already cached; if so, moves it to the front of the cache and
**    returns the existing file name. Otherwise, downloads the file,
**    adds it to the cache, and evicts the oldest entry if necessary.
**
** Parameters:
**    url - The URL of the album art to retrieve.
**
** Returns:
**    A String representing the local file name of the album art, or
**    a fallback file name (SP_NO_COVER_JPG_FILENAME) if the download
**    fails.
** ===================================================================
*/

String SpotifyArtMgr::acquireAlbumArt(const String& url)
{
    spLogD(LOGTAG_CACHE, "acquireAlbumArt() for URL: %s", url.c_str());

    _stats.totalFetches++;

    auto it = _urlToFileMap.find(url);

    // If the URL is already cached, move it to the front of the cache list
    bool isCacheHit = false;
    if (it != _urlToFileMap.end())
    {
        isCacheHit = true; 
        _cacheList.remove(url);     // Remove from current position
        _cacheList.push_front(url); // Move to the front
        _isDirty = true;            // Write the updated cache index
        _stats.cacheHits++;
        _stats.currentHitStreak++;      
        spLogD(LOGTAG_CACHE, "Cache hit. Moved URL to the front: %s", url.c_str());
    }
    _stats.averageHitRate = (_stats.totalFetches > 0) ? ((static_cast<double>(_stats.cacheHits) / _stats.totalFetches) * 100.0) : 0;   
    if (_stats.currentHitStreak > _stats.longestHitStreak)
    {
        _stats.longestHitStreak = _stats.currentHitStreak;
    }

    if (isCacheHit)
    {
        return it->second;          // Return the cached file name associated with the URL
    }

    _stats.currentHitStreak = 0;

    // Generate local file name and download
    String localFileName = generateLocalFileName();

    if (!downloadFile(url, localFileName))
    {
        spLogE(LOGTAG_GENERAL, "Failed to download album art for URL: %s", url.c_str());
        return SP_NO_COVER_JPG_FILENAME; // Return fallback image
    }

    // Add the URL and local file name to the cache
    if (_cacheList.size() >= _maxCacheSize)
    {
        evictOldestFile(); // Evict if necessary
    }

    _cacheList.push_front(url);
    _urlToFileMap[url] = localFileName;
    _isDirty = true;

    spLogD(LOGTAG_CACHE, "Returning downloaded file name: %s", localFileName.c_str());
    return localFileName;
}

/*
** ===================================================================
** setMaxCacheSize()
**    Sets the maximum size of the cache.
**
** Parameters:
**    maxSize - The maximum number of cached items
** ===================================================================
*/
void SpotifyArtMgr::setMaxCacheSize(size_t maxSize)
{
    _maxCacheSize = maxSize;
}

/*
** ===================================================================
** generateLocalFileName()
**    Generates a local file name for storing album art. If the cache
**    is full, it reuses the file name from the oldest cached entry.
**    Otherwise, it creates a new file name based on an internal counter.
**
** Parameters:
**    None.
**
** Returns:
**    A String containing the reused or newly generated local file name.
** ===================================================================
*/
String SpotifyArtMgr::generateLocalFileName()
{
    char buffer[32];

    // Check if the cache is full
    if (_cacheList.size() >= _maxCacheSize)
    {
        // Use the file name of the oldest (to be evicted) item in the list
        String oldestUrl = _cacheList.back(); // Back because LRU is the least recently used
        auto it = _urlToFileMap.find(oldestUrl);
        if (it != _urlToFileMap.end())
        {
            String fileName = it->second;
            spLogD(LOGTAG_CACHE, "Reusing file name of oldest cache entry: %s", fileName.c_str());
            return fileName; // Return the file name associated with the oldest URL
        }
    }

    // If the cache is not full, generate a new file name
    snprintf(buffer, sizeof(buffer), "/sc_album_art_%03zu.dat", _nextFileId++);
    spLogD(LOGTAG_CACHE, "Generated new file name: %s", buffer);
    return String(buffer);
}


/*
** ===================================================================
** evictOldestFile()
**    Removes the least recently used (LRU) entry from the cache. This
**    involves popping the oldest URL from the back of the cache list
**    and removing its corresponding file entry from the map.
**
** Parameters:
**    None.
**
** Returns:
**    None.
** ===================================================================
*/
void SpotifyArtMgr::evictOldestFile()
{
    if (_cacheList.empty())
    {
        spLogW(LOGTAG_CACHE, "Eviction attempted but cache list is empty.");
        return;
    }

    // Get the oldest URL (back of the list)
    String oldestUrl = _cacheList.back();
    _cacheList.pop_back(); // Remove from the back of the list

    // Find and remove the URL in the map
    auto it = _urlToFileMap.find(oldestUrl);
    if (it != _urlToFileMap.end())
    {
        spLogD(LOGTAG_CACHE, "Evicting URL: %s associated with file: %s",
               oldestUrl.c_str(), it->second.c_str());
        _urlToFileMap.erase(it); // Remove from the map
    }
    else
    {
        spLogW(LOGTAG_CACHE, "Attempted to evict a URL not found in _urlToFileMap.");
    }
}

/*
** ===================================================================
** downloadFile()
**    Downloads a file from the given URL and saves it locally.
**
** Parameters:
**    url      - The URL of the file to download
**    filename - The local file name to save the file as
**
** Returns:
**    True if the file was successfully downloaded, false otherwise.
** ===================================================================
*/
bool SpotifyArtMgr::downloadFile(String url, String filename)
{
    _isDownloadPending = true;

    // Placeholder for actual implementation
    spLogD(LOGTAG_GENERAL, "Downloading file: %s -> %s", url.c_str(), filename.c_str());

    // Was MAX_RETRIES = 2; however, retries seemed to fail
    // plus current code will try to refetch art anyway
    // during next refresh cycle
    constexpr int MAX_RETRIES    = 0;        
    constexpr int RETRY_DELAY_MS = 500;
    int retryCount = 0;

    spLogI(LOGTAG_SONG_DATA, "Downloading %s and saving as %s", url.c_str(), filename.c_str());

    while (retryCount <= MAX_RETRIES) {
        spLogD(LOGTAG_SONG_DATA, "Attempt %d of %d", retryCount + 1, MAX_RETRIES + 1);
        spLogD(LOGTAG_SONG_DATA, "Free heap before request: %d", Monitor::getFreeHeap());

        HTTPClient http;
        WiFiClientSecure wifiClient;

        spLogV(LOGTAG_SONG_DATA, "[HTTP] begin...");
        wifiClient.setCACert(gCombinedCerts);      //(spotify_image_server_cert);
        //http.setTimeout(10000);  // 5 seconds is the default
        http.begin(wifiClient, url);
        //http.setReuse(false);

        spLogV(LOGTAG_SONG_DATA, "[HTTP] GET...");
        Monitor::start(MONITOR_ID_SPOTIFY_IMAGE_HTTP_GET, LOGTAG_METRICS, "int httpCode = http.GET();");
        int httpCode = http.GET();
        Monitor::stop(MONITOR_ID_SPOTIFY_IMAGE_HTTP_GET);
        spLogD(LOGTAG_SONG_DATA, "Free heap before request: %d", Monitor::getFreeHeap());

        if (httpCode == HTTP_CODE_OK) {
            spLogV(LOGTAG_SONG_DATA, "[HTTP] GET succeeded with code: %d", httpCode);
            Monitor::start(MONITOR_ID_SPOTIFY_IMAGE_FILE_SAVE, LOGTAG_METRICS, "saveFileFromHTTPResponse(filename, &http);");

            // Wrap call that is going to use the file system
            spLogD(LOGTAG_MULTITASK,"Taking xSemaphoreFileIO.");
            if (xSemaphoreTake(SCFileIO::getInstance().xSemaphoreFileIO, portMAX_DELAY)) 
            {
                saveFileFromHTTPResponse(filename, &http);
                _isDirty = true;
                xSemaphoreGive(SCFileIO::getInstance().xSemaphoreFileIO);
            }
            else
            {
                spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreFileIO.");
            }
            Monitor::stop(MONITOR_ID_SPOTIFY_IMAGE_FILE_SAVE);
            http.end();
            wifiClient.stop();
            return true;  // Exit on success
        } else {
            spLogE(LOGTAG_SONG_DATA, "[HTTP] GET failed. Code: %d, Error: %s", httpCode, http.errorToString(httpCode).c_str());
        }

        http.end();
        wifiClient.stop();

        retryCount++;
        if (retryCount <= MAX_RETRIES) {
            spLogD(LOGTAG_SONG_DATA, "Retrying after %d ms...", RETRY_DELAY_MS);
            delay(RETRY_DELAY_MS);
        }
    }

    spLogE(LOGTAG_SONG_DATA, "Download failed after %d attempts. _isCoverArtAvailable = %", MAX_RETRIES + 1);

    _isDownloadPending = false;

    return false;
}

/*
** ===================================================================
** saveFileFromHTTPResponse()
**    Helper method that saves the file.  Put in own method to allow
** for retry logic.
**
**  NOTE: This method is not thread safe and needs to be called
**        called by something that wraps the call.
** ===================================================================
*/
void SpotifyArtMgr::saveFileFromHTTPResponse(String filename, HTTPClient *pHttp )
{
   fs::File f = LittleFS.open(filename, "w+");
    if (!f) 
    {
        spLogE(LOGTAG_SONG_DATA, "file open failed in SpotifyPlayer::downloadFile");
        return;
    }

    // get lenght of document (is -1 when Server sends no Content-Length
    // header)
    int total = pHttp->getSize();
    int len   = total;

    // create buffer for read
    uint8_t buff[128] = {0};

    // get tcp stream
    WiFiClient *stream = pHttp->getStreamPtr();

    // read all data from server
    while (pHttp->connected() && (len > 0 || len == -1)) 
    {
        // get available data size
        size_t size = stream->available();

        if (size) 
        {
            // read up to 128 byte
            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));

            // write it to Serial
            f.write(buff, c);

            if (len > 0) 
            {
                len -= c;
            }
        }
        delay(1);
    }
    spLogI(LOGTAG_SONG_DATA, "\n[HTTP] connection closed or file end.\n");

    f.close();

}

/*
** ===================================================================
** isCacheDirty()
**    Answers whether the cache index file has been changed
** since the last save.  Value can be used to assist with UI indicators.
** ===================================================================
*/
bool SpotifyArtMgr::isCacheDirty()
{
    return _isDirty;
}

/*
** ===================================================================
** getStats()
**    Retrieves a copy of the current SpotifyArtMgr statistics.
**
** Returns:
**    ArtMgrStats - A struct containing various statistics related to
**                  album art fetches, cache hits, and hit streaks.
**
** Notes:
**    - This method returns a copy of the statistics, ensuring the 
**      original data remains unchanged.
**    - The returned struct includes total fetches, cache hit count,
**      current and longest hit streaks, and the average hit rate.
**
** Example Usage:
**    ArtMgrStats stats = SpotifyArtMgr::getInstance().getStats();
**    spLogI(LOGTAG_GENERAL, "Cache Hits: %u", stats.cacheHits);
** ===================================================================
*/
ArtMgrStats SpotifyArtMgr::getStats()
{
    ArtMgrStats answerStats;

    answerStats.totalFetches      = _stats.totalFetches;   
    answerStats.cacheHits         = _stats.cacheHits;
    answerStats.currentHitStreak  = _stats.currentHitStreak;
    answerStats.longestHitStreak  = _stats.longestHitStreak;
    answerStats.averageHitRate    = _stats.averageHitRate;  

    return answerStats;
}

/*
** ===================================================================
** saveCacheIndex()
**    Saves the current cache state to a file in a simple text format.
**    This includes the mapping of URLs to file names and the order of
**    cached items for proper eviction handling.
**
** Format:
**    Each line of the file contains a URL and its associated local 
**    file name, separated by a '|'. The final line contains a checksum 
**    in the format "CHECKSUM:<value>".
**
** Notes:
**    - The cache state is saved to the file "/cache_index.txt".
**    - This method calculates a checksum of the serialized cache 
**      data to validate file integrity during loading.
**    - A temporary file "/cache_index.tmp" is used to ensure atomic 
**      updates, and the original file is replaced only if the 
**      operation succeeds.
**
** Integrity:
**    - The checksum is written as the last line of the file.
**    - Any changes to the file format should account for checksum 
**      validation logic.
**
** Preconditions:
**    - The `_urlToFileMap` and `_cacheList` structures must be in sync.
**    - The `SCFileIO` singleton handles thread-safe file operations.
**
** ===================================================================
*/
void SpotifyArtMgr::saveCacheIndex()
{
    spLogV(LOGTAG_CACHE, "Saving cache index to a simple text format with checksum.");

    if (!_isDirty)
    {
        spLogV(LOGTAG_CACHE, "No changes to save. Returning.");
        return;
    }

    Monitor::start(MONITOR_ID_SPOTIFY_IMAGE_CACHE_SAVE, LOGTAG_CACHE, "saveCacheIndex()");

    String tempFilePath  = String(gCacheFileroot) + ".tmp";
    String finalFilePath = String(gCacheFileroot) + ".txt";

    // Write to a temporary file
    File tempFile = SCFileIO::getInstance().open(tempFilePath.c_str(), "w+");
    if (!tempFile)
    {
        spLogE(LOGTAG_CACHE, "Failed to open temporary file for cache index.");
        Monitor::stop(MONITOR_ID_SPOTIFY_IMAGE_CACHE_SAVE);
        return;
    }

    String serializedData;

    // Write cache entries and construct serialized data for checksum calculation
    for (const auto& entry : _urlToFileMap)
    {
        serializedData += entry.first + "|" + entry.second + "\n";
        tempFile.printf("%s|%s\n", entry.first.c_str(), entry.second.c_str());
    }

    // Calculate checksum
    uint32_t checksum = calculateChecksum(serializedData.c_str(), serializedData.length());

    // Write checksum as the last line
    tempFile.printf("CHECKSUM:%u\n", checksum);
    tempFile.close();
    spLogI(LOGTAG_CACHE, "Temporary cache index saved successfully.");

    // Rename the temporary file to the final file
    if (SCFileIO::getInstance().exists(finalFilePath.c_str()))
    {
        SCFileIO::getInstance().remove(finalFilePath.c_str());
    }

    if (LittleFS.rename(tempFilePath, finalFilePath))
    {
        _isDirty = false;
        spLogI(LOGTAG_CACHE, "Cache index successfully updated.");
    }
    else
    {
        spLogE(LOGTAG_CACHE, "Failed to rename temporary file to final file.");
    }

    Monitor::stop(MONITOR_ID_SPOTIFY_IMAGE_CACHE_SAVE);

    printCacheIndex(LOGTAG_CACHE);
    //SCFileIO::getInstance().listFiles();
}

/*
** ===================================================================
** determineCacheSize()
**    Determines the maximum number of album art files to cache based
**    on the size of the LittleFS partition. If the partition is at
**    least 3,000,000 bytes, this is assumed to be a custom partition
**    layout and the cache size is increased to 60. Otherwise, the
**    default cache size (typically 10) is used.
**
**    This method should be called during initialization to adjust the
**    cache size dynamically based on the hardware configuration.
**
** Notes:
**    - SCFileIO::getPartitionSize() is used to obtain the partition size
**      in a thread-safe manner.
**    - A partition size >= 3,000,000 bytes indicates use of the custom
**      no_ota.csv layout with a larger cache capacity.
**    - The default cache size should be initialized before this is called.
**
** ===================================================================
*/

void SpotifyArtMgr::determineCacheSize()
{
    size_t partitionSize = SCFileIO::getInstance().getPartitionSize();

    if (partitionSize >= 3000000)
    {
        _maxCacheSize = 60;
        spLogI(LOGTAG_CACHE, "Custom partition detected (size: %zu). Setting max cache size to 60.", partitionSize);
    }
    else
    {
        spLogI(LOGTAG_CACHE, "Standard partition detected (size: %zu). Using default cache size: %zu.", partitionSize, _maxCacheSize);
    }
}

/*
** ===================================================================
** getMaxCacheSize()
**    Returns the current maximum cache size used for album art.
**
** Returns:
**    size_t - The maximum number of cached album art entries.
** ===================================================================
*/
size_t SpotifyArtMgr::getMaxCacheSize()
{
    return _maxCacheSize;
}

/*
** ===================================================================
** loadCacheIndex()
**    Loads the cache state from a previously saved file in a simple 
**    text format. This method restores the mapping of URLs to file 
**    names and the order of cached items.
**
** Format:
**    Each line of the file contains a URL and its associated local 
**    file name, separated by a '|'. The final line contains a checksum 
**    in the format "CHECKSUM:<value>".
**
** Validation:
**    - The checksum stored in the file is compared with a calculated 
**      checksum of the serialized data to ensure integrity.
**    - If the checksum validation fails, the file is removed, and the 
**      cache state is reset.
**
** Notes:
**    - The cache state is read from the file "/cache_index.txt".
**    - If the file is missing, empty, or invalid, the cache is started fresh.
**    - This method clears and repopulates `_urlToFileMap` and `_cacheList`.
**
** Preconditions:
**    - The `SCFileIO` singleton handles thread-safe file operations.
**
** Error Handling:
**    - Logs warnings for invalid or missing entries in the file.
**    - Removes the cache file if checksum validation fails or 
**      deserialization encounters an error.
**
** ===================================================================
*/
void SpotifyArtMgr::loadCacheIndex()
{
    spLogI(LOGTAG_CACHE, "Loading cache index from a simple text file with checksum.");

    String finalFilePath = String(gCacheFileroot) + ".txt";

    if (!SCFileIO::getInstance().exists(finalFilePath.c_str()))
    {
        spLogW(LOGTAG_CACHE, "Cache index file not found. Starting fresh.");
        return;
    }

    File file = SCFileIO::getInstance().open(finalFilePath.c_str(), "r");
    if (!file || file.size() == 0)
    {
        spLogE(LOGTAG_CACHE, "Cache index file is empty or inaccessible. Removing it and starting fresh.");
        SCFileIO::getInstance().remove(finalFilePath.c_str());
        return;
    }

    String serializedData;
    uint32_t storedChecksum = 0;
    uint32_t maxFileId = 0; // Track the highest file ID

    _cacheList.clear();
    _urlToFileMap.clear();

    // Read file line by line
    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();

        // Check for checksum line
        if (line.startsWith("CHECKSUM:"))
        {
            storedChecksum = line.substring(9).toInt(); // Extract checksum value
            continue;
        }

        // Parse URL and file name
        int separatorIndex = line.indexOf('|');
        if (separatorIndex != -1)
        {
            String url         = line.substring(0, separatorIndex);
            String localFile   = line.substring(separatorIndex + 1);
            _urlToFileMap[url] = localFile;
            _cacheList.push_back(url);

            // Extract numeric part of file name (e.g., "003" from "/sc_album_art_003")
            int startIdx = localFile.lastIndexOf('_') + 1;
            int endIdx = localFile.lastIndexOf('.');

            // Log the start and end indexes before any logic
            spLogD(LOGTAG_CACHE, "Parsing file name: %s", localFile.c_str());
            spLogD(LOGTAG_CACHE, "Initial startIdx: %d, endIdx: %d (before adjustments)", startIdx, endIdx);

            // Check if there's a period after the underscore
            if (localFile.lastIndexOf('.') > startIdx) {
                endIdx = localFile.lastIndexOf('.');
                spLogD(LOGTAG_CACHE, "Adjusted endIdx to: %d (period found after underscore)", endIdx);
            }

            // Log indexes before the substring operation
            spLogD(LOGTAG_CACHE, "Final startIdx: %d, endIdx: %d", startIdx, endIdx);            
            
            if (startIdx > 0 && endIdx > startIdx)
            {
                String   fileIdStr = localFile.substring(startIdx, endIdx);
                uint32_t fileId    = fileIdStr.toInt();

                // Update maxFileId
                if (fileId > maxFileId)
                {
                    maxFileId = fileId;
                }

                spLogD(LOGTAG_CACHE, "Parsed file ID: %u from localFile: %s", fileId, localFile.c_str());
            }

            serializedData += line + "\n"; // Accumulate serialized data
        }
    }
    file.close();

    // Validate checksum
    uint32_t calculatedChecksum = calculateChecksum(serializedData.c_str(), serializedData.length());
    if (storedChecksum != calculatedChecksum)
    {
        spLogE(LOGTAG_CACHE, "Checksum mismatch! Stored: %u, Calculated: %u", storedChecksum, calculatedChecksum);
        SCFileIO::getInstance().remove(finalFilePath.c_str());
        return;
    }

    // Update _nextFileId
    _nextFileId = maxFileId + 1;
    spLogI(LOGTAG_CACHE, "Next file ID set to: %u", _nextFileId);

    spLogI(LOGTAG_CACHE, "Checksum validation passed. Cache index loaded successfully.");

    printCacheIndex(LOGTAG_CACHE);
}

/*
** ===================================================================
** printCacheIndex()
**    Logs the current cache state in a formatted and readable format.
**    Check code for log levels used.
**
**    The log includes:
**      - List of URLs and their associated file names in the LRU order.
**      - The calculated checksum for the cache state.
**
** Parameters:
**    tag - The logging tag to identify the context of the log.
**
** Format:
**    Cache:
**    "URL1": "FileName1"
**    "URL2": "FileName2"
**    ...
**    Checksum: <calculated checksum>
**
** Example Output:
**    Cache:
**    "https://i.scdn.co/image/ab67616d00001e02203e4c6a048df02a21cdd813": "/sc_album_art_002"
**    "https://i.scdn.co/image/ab67616d00001e02ccdddd46119a4ff53eaf1f5d": "/sc_album_art_003"
**    "https://i.scdn.co/image/ab67616d00001e02e85259a1cae29a8d91f2093d": "/sc_album_art_001"
**    Checksum: 61
**
** Notes:
**    - The checksum is recalculated for the current cache state to
**      ensure data integrity and consistency.
**
** ===================================================================
*/
void SpotifyArtMgr::printCacheIndex(const char* tag)
{
    spLogD(tag, "------------------------------------------------------------------------------------------");
    spLogD(tag, "Cache:");

    // If the cache is empty, indicate that
    if (_cacheList.empty())
    {
        spLogI(tag, "    [Cache is empty]");
        return;
    }

    // Prepare a string to calculate the checksum
    String serializedData;

    // Print and serialize the cache data
    for (const auto& url : _cacheList)
    {
        auto it = _urlToFileMap.find(url);
        if (it != _urlToFileMap.end())
        {
            spLogD(tag, "    \"%s\": \"%s\"", url.c_str(), it->second.c_str());
            serializedData += url + ":" + it->second + "\n";
        }
        else
        {
            spLogW(tag, "    \"%s\": [No associated file found]", url.c_str());
        }
    }

    // Calculate and print the checksum
    // uint32_t checksum = calculateChecksum(serializedData.c_str(), serializedData.length());
    // spLogI(tag, "Checksum: %u", checksum);
    spLogD(tag, "------------------------------------------------------------------------------------------");

    String filePath = String(gCacheFileroot) + ".txt";
    SCFileIO::getInstance().hexDump(LOGTAG_CACHE, filePath.c_str());
}

/*
** ===================================================================
** calculateChecksum()
**    Calculates a simple XOR-based checksum for a given data buffer.
**
** Parameters:
**    data   - Pointer to the data buffer to calculate the checksum for
**    length - The length of the data buffer
**
** Returns:
**    The calculated checksum as a 32-bit unsigned integer.
**
** Notes:
**    - This checksum is based on a simple XOR operation, where each
**      byte in the data buffer is XORed with the checksum value.
**    - This method is lightweight and suitable for basic integrity
**      checks, but it is not cryptographically secure.
**    - Ensure that the checksum field itself is excluded from the data
**      buffer being validated to avoid circular dependency issues.
**
** Example Usage:
**    const char* data = "example data";
**    size_t length = strlen(data);
**    uint32_t checksum = calculateChecksum(data, length);
**
** ===================================================================
*/
uint32_t SpotifyArtMgr::calculateChecksum(const char* data, size_t length)
{
    uint32_t checksum = 0;
    for (size_t i = 0; i < length; ++i)
    {
        checksum ^= data[i];
    }
    return checksum;
}