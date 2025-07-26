/*-------------------------------------------------------------------------------------------------
**
** SpotifyPlayer.cpp
**
**    Instances of this class control Spotify once it has started.
**    This includes UI interactions. Due to dependencies used by
**    this class it should be treated as a Singleton.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2024-12-28 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include "esp_task_wdt.h"
#include "SpotifyPlayer.h"
#include "logTags.h"
#include "SCLogger.h"
#include "ThingPulse/spotify.h"
#include "Monitor.h"
#include "SpotifyArtMgr.h"
#include "SCFileIO.h"

#include <TJpg_Decoder.h> // Ensure you include the required decoder library

// Spotify related
#define SP_SPOTIFY_MARKET         "IE"

/*
** ===================================================================
** Callback routine for spotify.getCurrentlyPlaying() method.
** ===================================================================
*/
void SpotifyPlayer::getCurrentlyPlayingCallback(CurrentlyPlaying currentlyPlaying)
{
    spLogD(LOGTAG_PLAYER, "getCurrentlyPlayingCallback - callback routine for spotify.getCurrentlyPlaying() method.");
    SpotifyPlayer::getInstance().refreshCurrentSong(currentlyPlaying);
}

/*
** ===================================================================
** Constructor and Destructor
** ===================================================================
*/
SpotifyPlayer::SpotifyPlayer()
{
    // Initialization code (if needed)
}

SpotifyPlayer::~SpotifyPlayer() 
{
}

/*
** ===================================================================
** getInstance()
** ===================================================================
*/
SpotifyPlayer& SpotifyPlayer::getInstance() 
{
    static SpotifyPlayer instance; // Guaranteed to be thread-safe in C++11 and later
    return instance;
}

/*
** ===================================================================
** initialize()
** ===================================================================
*/

void SpotifyPlayer::initialize(QueueHandle_t    *pScuiQueue)
{
    _pScuiQueue = pScuiQueue;

    // keep reference since SpotifyArduino is dependent on values
    // remaining in memory
    _spotifyClientId     = Vault::getInstance().getSpotifyClientID();
    _spotifyClientSecret = Vault::getInstance().getSpotifyClientSecret();

    spotify.lateInit(_spotifyClientId.c_str(), _spotifyClientSecret.c_str(), _spotifyRefreshToken.c_str());

    // client is defined in ThingPulse/spotify.h
    client.setCACert(spotify_server_cert);  
    
}

/*
** ===================================================================
** startBackgroundRefreshes()
** ===================================================================
*/

void SpotifyPlayer::startBackgroundRefreshes()
{

    // The ESP32 is a dual-core processor with two Xtensa LX6 cores:
    // 
    // - **Core 0 (PRO CPU)** → Handles system tasks (Wi-Fi, Bluetooth, FreeRTOS)
    // - **Core 1 (APP CPU)** → Runs the Arduino framework (`setup()` and `loop()`)
    //
    // By default, all Arduino code executes on **Core 1**.

    // Create task to refresh song information
    spLogI(LOGTAG_MULTITASK, "creating background task - pinned to core 1");

    // Note: Core 0 had stability issues despite best attempts
    // to address.  Moved UI loop which doesn't block the way
    // that the song refresh logic does to run on Core 0.
    xTaskCreatePinnedToCore(
        SpotifyPlayer::refreshCurrentSongTask, // Task function
        "SongRefresh",          // Task name
        8192,                   // Stack size
        NULL,                   // Task parameter
        1,                      // Task priority
        &_refreshTaskHandle,    // Task handle
        1                       // Core ID
    );    
    
    spLogI(LOGTAG_MULTITASK, "background task created");

}

/*
** ===================================================================
** isRefreshTokenAvailable()
**
** Returns whether the refresh token is available.  This will also
** internally initialize the token if it exists.
** ===================================================================
*/
bool SpotifyPlayer::isRefreshTokenAvailable()
{
   _spotifyRefreshToken = SCFileIO::getInstance().readFsString(SPOTIFY_REFRESH_TOKEN_FILE_NAME);
   
   if (_spotifyRefreshToken == "")
   {
        spLogI(LOGTAG_PLAYER, "Spotify refresh token not found.");
        return false;
   }
   else
   {
        spLogI(LOGTAG_PLAYER, "Spotify refresh token found.");
        return true;
   }
}

/*
** ===================================================================
** requestRefreshToken()
**
** Requests a fresh refresh token.  A refresh token is a security 
** credential that allows client applications to obtain new access
** tokens without requiring users to reauthorize the application.
** ===================================================================
*/
bool SpotifyPlayer::requestRefreshToken()
{

    spLogI(LOGTAG_PLAYER, "Requesting Spotify refresh token through the browser via auth code.");

    String spotifyAuthCode = fetchSpotifyAuthCode();
    _spotifyRefreshToken = spotify.requestAccessTokens(spotifyAuthCode.c_str(), SPOTIFY_REDIRECT_URI);
    SCFileIO::getInstance().saveFsString(SPOTIFY_REFRESH_TOKEN_FILE_NAME, _spotifyRefreshToken);

    return true;

}

/*
** ===================================================================
** getNodeName()
** ===================================================================
*/
String SpotifyPlayer::getNodeName()
{
    return SPOTIFY_ESPOTIFIER_NODE_NAME;
}

/*
** ===================================================================
** login()
** ===================================================================
*/
void SpotifyPlayer::login()
{
    // The Spotify library
    // - keeps track of the refresh token and its TTL internally
    // - automatically renews the actual access token using the refresh token
    // -> see SpotifyArduino.h#autoTokenRefresh and SpotifyArduino::checkAndRefreshAccessToken() (called before every API function)
    spotify.setRefreshToken(_spotifyRefreshToken.c_str());
    spotify.refreshAccessToken();
    spLogI(LOGTAG_PLAYER, "Authentication against Spotify done. Refresh token: %s", _spotifyRefreshToken.c_str());    
}

/*
** ===================================================================
** postScuiMessage()
**
**     Sends a message to the SCUI queue with a specified type,
**     string data, and an integer value. The integer is appended
**     to the message data.
**
** Parameters:
**     type - The type of SCUI message.
**     str  - The string data to send.
**     num  - The integer value to append to the message.
**
** Returns:
**     None
** ===================================================================
*/
void SpotifyPlayer::postScuiMessage(SCUIMessageType type, const String& str, int num)
{
    if (_pScuiQueue != NULL)
    {
        SCUIMessage msg;
        msg.type = type;
        msg.str  = str;
        msg.num  = num;

        Monitor::start(MONITOR_ID_SCUI_QUEUE_DELAY, LOGTAG_MULTITASK, "postScuiMessage()");
        if (xQueueSend(*_pScuiQueue, &msg, pdMS_TO_TICKS(10)) != pdPASS)
        {
            spLogE(LOGTAG_PLAYER, "Failed to send SCUI message to queue");
        }
    }
}

/*
** ===================================================================
** nextSong()
** ===================================================================
*/
void SpotifyPlayer::nextSong()
{
    // Code to skip to the next song using Spotify's API
    // Example: Sending an API request or invoking a method in the client
    spLogI(LOGTAG_PLAYER, "Skipping to the next song.");

    postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                    "Making Call",
                    static_cast<int>(TFTColor::SC_NetworkInProgress));
    if (xSemaphoreTake(_xSemaphoreNetwork, portMAX_DELAY)) 
    {
        if (spotify.nextTrack())
        {
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "",
                            static_cast<int>(TFTColor::SC_NetworkSuccess));
            spLogD(LOGTAG_PLAYER, "next track call succesful");
        }
        else
        {
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "",
                            static_cast<int>(TFTColor::SC_NetworkFailure));            
            spLogD(LOGTAG_PLAYER, "next track call failed"); // 
        }
        xSemaphoreGive(_xSemaphoreNetwork);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take _xSemaphoreNetwork.");
    }    

}

/*
** ===================================================================
** previousSong()
** ===================================================================
*/
void SpotifyPlayer::previousSong()
{
    // Code to go back to the previous song using Spotify's API
    // Example: Sending an API request or invoking a method in the client
    spLogI(LOGTAG_PLAYER, "Going back to the previous song.");

    postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                    "Making Call",
                    static_cast<int>(TFTColor::SC_NetworkInProgress));    

    if (xSemaphoreTake(_xSemaphoreNetwork, portMAX_DELAY)) 
    {
        if (spotify.previousTrack())
        {
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "",
                            static_cast<int>(TFTColor::SC_NetworkSuccess));
            spLogD(LOGTAG_PLAYER, "previous track call succesful");
        }
        else
        {
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "",
                            static_cast<int>(TFTColor::SC_NetworkFailure));    
            spLogD(LOGTAG_PLAYER, "previous track call failed");
        }
        xSemaphoreGive(_xSemaphoreNetwork);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take _xSemaphoreNetwork.");
    }


}

/*
** ===================================================================
** pauseSong()
** ===================================================================
*/
void SpotifyPlayer::pauseSong()
{
    // Code to pause playback using Spotify's API
    // Example: Sending an API request or invoking a method in the client
    spLogI(LOGTAG_PLAYER, "Pausing the current song.");

    if (xSemaphoreTake(_xSemaphoreNetwork, portMAX_DELAY)) 
    {
        if (_isPlaying)
        {
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "Making Call",
                            static_cast<int>(TFTColor::SC_NetworkInProgress));    
            if (spotify.pause())
            {
                postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                                "",
                                static_cast<int>(TFTColor::SC_NetworkSuccess));
                spLogD(LOGTAG_PLAYER, "pause track call succesful");
            }
            else
            {
                postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                                "",
                                static_cast<int>(TFTColor::SC_NetworkFailure));   
                spLogD(LOGTAG_PLAYER, "pause track call failed"); // 
            }                    
        }
        else
        {
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "Making Call",
                            static_cast<int>(TFTColor::SC_NetworkInProgress));    
            if (spotify.play())
            {
                postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                                "",
                                static_cast<int>(TFTColor::SC_NetworkSuccess));
                spLogD(LOGTAG_PLAYER, "pause track call succesful");
            }
            else
            {
                postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                                "",
                                static_cast<int>(TFTColor::SC_NetworkFailure));  
                spLogD(LOGTAG_PLAYER, "pause track call failed"); // 
            }                      
        }     
        xSemaphoreGive(_xSemaphoreNetwork);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take _xSemaphoreNetwork.");
    }
}

/*
** ===================================================================
** refreshCurrentTrack()
** ===================================================================
*/

void SpotifyPlayer::refreshCurrentTrack()
{
        spLogD(LOGTAG_MULTITASK, "Free Heap: ");
        spLogD(LOGTAG_MULTITASK, "%d", ESP.getFreeHeap());
       
        postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                        "Making Call",
                        static_cast<int>(TFTColor::SC_NetworkInProgress));    

        spLogI(LOGTAG_MULTITASK, "getting currently playing song:");
        // Market can be excluded if you want e.g. spotify.getCurrentlyPlaying()
        int status = -777;
        
        if (xSemaphoreTake(_xSemaphoreNetwork, portMAX_DELAY)) 
        {
            spLogI(LOGTAG_MULTITASK, "Invoking spotify.getCurrentlyPlaying(...)");
            Monitor::start(MONITOR_ID_SPOTIFY_GET_CURRENTLY_PLAYING, LOGTAG_METRICS, "spotify.getCurrentlyPlaying(...)");
            status = spotify.getCurrentlyPlaying(SpotifyPlayer::getCurrentlyPlayingCallback, SP_SPOTIFY_MARKET);
            Monitor::stop(MONITOR_ID_SPOTIFY_GET_CURRENTLY_PLAYING);
            xSemaphoreGive(_xSemaphoreNetwork);
        }
        else
        {
            spLogI(LOGTAG_MULTITASK,"Unable to take _xSemaphoreNetwork.");
        }


        if (status == 200)
        {
            spLogI(LOGTAG_MULTITASK, "Successfully refreshed current song.");
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "Successful",
                            static_cast<int>(TFTColor::SC_NetworkSuccess));
        }
        else if (status == 204)
        {
            spLogI(LOGTAG_MULTITASK, "Doesn't seem to be anything playing");
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "Nothing playing",
                            static_cast<int>(TFTColor::SC_AlertStatus));

        }
        else if (status == -777)
        {
            spLogI(LOGTAG_MULTITASK, "No, really, unable to get semaphore.  status still -777");
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "Unable to get semaphore",
                            static_cast<int>(TFTColor::SC_AlertStatus));
        }
        else
        {
            spLogE(LOGTAG_MULTITASK, "Error: ");
            spLogE(LOGTAG_MULTITASK, "Status: %d", status);
            postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                            "Failure",
                            static_cast<int>(TFTColor::SC_NetworkFailure));
        }

        refreshCoverArt();

        if (_isNewTrack)
        {
                // Track is loaded and copied.
                // At least one pass made at downloading the art.
                _isNewTrackReady = true;
        }

        postScuiMessage(SCUIMessageType::UM_PLAYER_REFRESH,
                        "Refresh",
                        _isNewTrackReady);
}

/*
** ===================================================================
** refreshCoverArt()
** 
**    This method handles refreshing the cover art for the currently
** playing track. It checks if a new track is detected or if cover
** art is unavailable and downloads the appropriate album art. If
** the download is successful, it updates the background color to
** match the average color of the album art. It also ensures the 
** UI is marked dirty to trigger a repaint.
**
** Parameters:
**    None
**
** Returns:
**    None
**
** Notes:
**    - The method determines if the currently playing track has 
**      changed by comparing track URIs.
**    - If cover art is downloaded successfully, the background 
**      color is updated based on the average color of the image.
**    - UI is marked dirty whenever the cover art is refreshed.
**
** ===================================================================
*/

void SpotifyPlayer::refreshCoverArt()
{
    bool bNewTrack = false;

    if (_currentTrackUri != _currentlyPlayingMetadata.trackUri)
    {
        spLogI(LOGTAG_GENERAL, "New song detected: %s", _currentlyPlayingMetadata.trackName);
        bNewTrack                = true;
        _isCoverArtAvailable     = false;
        _currentTrackUri         = _currentlyPlayingMetadata.trackUri; 
    }

    // if new track, download and draw it
    spLogD(LOGTAG_GENERAL, "if (bNewTrack || !_isCoverArtAvailable)... bNewTrack=%d, _isCoverArtAvailable=%d", (int)bNewTrack, (int)_isCoverArtAvailable);
    if (bNewTrack 
    || !_isCoverArtAvailable)
    {
        const int TARGET_IMAGE_SIZE  = 300;
        String    filePath; 
        
        for (int i = 0; i < _currentlyPlayingMetadata.numImages; i++)
        {
            if ((_currentlyPlayingMetadata.albumImages[i].height == TARGET_IMAGE_SIZE)
            &&  (_currentlyPlayingMetadata.albumImages[i].width  == TARGET_IMAGE_SIZE))
            {
                spLogV(LOGTAG_GENERAL, "*** Attempting image download ***");

                postScuiMessage(SCUIMessageType::UM_DOWNLOAD_BOX,"",false);
                Monitor::start(MONITOR_ID_FETCH_ALBUM_ART, LOGTAG_METRICS, "Download Art If Needed.");
                filePath = SpotifyArtMgr::getInstance()->acquireAlbumArt(_currentlyPlayingMetadata.albumImages[i].url);
                Monitor::stop(MONITOR_ID_FETCH_ALBUM_ART);
                postScuiMessage(SCUIMessageType::UM_DOWNLOAD_BOX,"",true);

                // Check if the acquired file is valid
                if (filePath != SP_NO_COVER_JPG_FILENAME 
                && !filePath.isEmpty())
                {
                    _isCoverArtAvailable = true;
                    spLogV(LOGTAG_GENERAL, "*** Cover art is available: %s ***", filePath.c_str());
                }
                else
                {
                    _isCoverArtAvailable = false;
                    spLogW(LOGTAG_GENERAL, "*** Cover art is unavailable ***");
                }                

                spLogV(LOGTAG_GENERAL, "*** downloadFile() finished executing ***");
                break;
            }
        }
        if (_isCoverArtAvailable)
        {
            // TFTColor c = _pUI->calculateAverageColor(filePath.c_str());
            // _pUI->setBackground(c, false);
        }
        else
        {
            spLogI(LOGTAG_MULTITASK, "*** %dx%d image not found to download. ***",TARGET_IMAGE_SIZE,TARGET_IMAGE_SIZE); 
        }

        // request updated UI
        postScuiMessage(SCUIMessageType::UM_MARK_DIRTY,
                        "",
                        true);        

    }
    
}

/*
** ===================================================================
** refreshCurrentSong()
**
**     This is the callback for what is playing from the Spotify API.
** ===================================================================
*/
void SpotifyPlayer::refreshCurrentSong(CurrentlyPlaying currentlyPlaying)
{

    spLogI(LOGTAG_MULTITASK, "Refreshing current song.  SpotifyPlayer::refreshCurrentSong(CurrentlyPlaying currentlyPlaying)");

    // If not a track or episode, update the UI accordingly and indicate music isn't available
    if ((!currentlyPlaying.currentlyPlayingType == SpotifyPlayingType::track)
    &&  (!currentlyPlaying.currentlyPlayingType == SpotifyPlayingType::episode))
    {
        spLogI(LOGTAG_GENERAL, " _isMusicAvailable set to false. currentlyPlayingType is not a supported type." );
        _isMusicAvailable = false; 
        // force a refresh to get the waiting message
        postScuiMessage(SCUIMessageType::UM_MARK_DIRTY,
                        "",
                        true);        
        return;
    }

    if (xSemaphoreTake(_xSemaphoreDataCopy, portMAX_DELAY)) 
    {
        // copy currentlyPlaying over for use
        if (currentlyPlaying.trackUri == nullptr)
        {
            spLogV(LOGTAG_GENERAL, "currentlyPlaying.trackUri == nullptr. skipping new track ready logic.");
        }
        else
        {
            if (strcmp(currentlyPlaying.trackUri, _currentlyPlayingMetadata.trackUri) != 0)
            {
                // New track has just been loaded
                _isNewTrack = true;
                spLogV(LOGTAG_GENERAL, "New Track!!!  _isNewTrack = true");
            }
            else
            {
                _isNewTrack = false;
            }
        }
        _currentlyPlayingMetadata.copyFrom(currentlyPlaying);
        xSemaphoreGive(_xSemaphoreDataCopy);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK, "Unable to take _xSemaphoreDataCopy.");
    }

    _isPlaying = currentlyPlaying.isPlaying;
    
    if (currentlyPlaying.trackName == nullptr)
    {
        spLogI(LOGTAG_MULTITASK, "refreshCurrentSong() - Empty track detected");
        _isMusicAvailable = false; 
        // force a refresh to get the waiting message
        postScuiMessage(SCUIMessageType::UM_MARK_DIRTY,
                        "",
                        true);
        return;
    }
    else
    {
        // spLogI(LOGTAG_GENERAL, " _isMusicAvailable = true; currentlyPlaying.trackName ='%s' with length = %u", currentlyPlaying.trackName, strlen(currentlyPlaying.trackName) );
        _isMusicAvailable = true; 
        printCurrentlyPlayingToSerial(currentlyPlaying);
    }

}

/*
** ===================================================================
** isMusicAvailable()
**    Answer whether music is currently available to display.
** ===================================================================
*/
bool SpotifyPlayer::isMusicAvailable()
{
    return _isMusicAvailable;
}

/*
** ===================================================================
** printCurrentlyPlayingToSerial()
**    Prints the currently playing track details to serial output.
** ===================================================================
*/
void SpotifyPlayer::printCurrentlyPlayingToSerial(CurrentlyPlaying currentlyPlaying)
{

    spLogI(LOGTAG_SONG_DATA, "--------- Currently Playing ---------");

    spLogI(LOGTAG_SONG_DATA, "Track: %s", currentlyPlaying.trackName);
    spLogV(LOGTAG_SONG_DATA, "");

    spLogV(LOGTAG_SONG_DATA, "Artists: ");
    for (int i = 0; i < currentlyPlaying.numArtists; i++)
    {
        spLogV(LOGTAG_SONG_DATA, "  Name: %s", currentlyPlaying.artists[i].artistName);
        spLogV(LOGTAG_SONG_DATA, "");
    }

    spLogV(LOGTAG_SONG_DATA, "Album: %s", currentlyPlaying.albumName);
    spLogV(LOGTAG_SONG_DATA, "");

    spLogI(LOGTAG_SONG_DATA, "------------------------");    

}

/*
** ===================================================================
** getCurrentlyPlayingMetadata()
**    Returns a stable copy of the currently playing metadata.
**    This method ensures that the returned data does not change
**    unexpectedly while in use.  It will have the side effect
**    of reseting isNewTrackReady() to false.
**
** Returns:
**    A reference to the stable DTO copy of PlayingMetadata.
** ===================================================================
*/
const PlayingMetadata &SpotifyPlayer::getCurrentlyPlayingMetadata() 
{
    copyMetadataToDTO();
    return _currentlyPlayingMetadataDTO;
}  

/*
** ===================================================================
** copyMetadataToDTO()
**    Copies the currently playing metadata into a stable DTO.
**    This method ensures thread safety by using a semaphore
**    to prevent data races.
**
** Locks:
**    - _xSemaphoreDataCopy: Ensures safe access to shared data.
**
** Behavior:
**    - If the semaphore is available, it performs a deep copy.
**    - If the semaphore is not available, logs an informational message.
** ===================================================================
*/
void SpotifyPlayer::copyMetadataToDTO() 
{
    if (xSemaphoreTake(_xSemaphoreDataCopy, portMAX_DELAY)) 
    {
        // copy currentlyPlaying over for use
        _currentlyPlayingMetadataDTO.copyFrom(_currentlyPlayingMetadata);
        _isNewTrackReady = false;
        xSemaphoreGive(_xSemaphoreDataCopy);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK, "Unable to take _xSemaphoreDataCopy.");
    }        
    
}    

/*
** ===================================================================
** isNewTrackReady()
**     Answers whether a new track has become ready and 
**   getCurrentlyPlayingMetadata() should be called to populdate
**   the DTO.
**  
** ===================================================================
*/
const bool   SpotifyPlayer::isNewTrackReady()
{
    return _isNewTrackReady;
}

void SpotifyPlayer::saveCache()
{

    SpotifyArtMgr *pSAM = SpotifyArtMgr::getInstance();

    if (pSAM->isCacheDirty())
    {
        postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                        "",
                        static_cast<int>(TFTColor::SC_CacheSave));  
        pSAM->saveCacheIndex(); // persist the updated file index
    }

    // clear status.  this assumes that this is the last
    // update which slightly breaks encapsulation and
    // is also brittle; however, not sure there is a cleaner
    // approach that isn't worse.
    postScuiMessage(SCUIMessageType::UM_STATUS_BOX,
                    "",
                    -1);  // -1 will use background color         

}

/*
** ===================================================================
** refreshCurrentSongTask()
**    Background task responsible for periodically refreshing the
**    currently playing song information from Spotify. The task 
**    includes a blocking HTTP call to fetch data and resets the 
**    Task Watchdog Timer to prevent timeout.
**
** Parameters:
**    pvParameters - Pointer to task parameters (not used in this task).
**
** Notes:
**    - Executes in an infinite loop with a specified delay between
**      iterations to avoid overloading the system.
**    - Calls the refreshCurrentTrack() method of SpotifyPlayer to 
**      perform the actual data retrieval.
**    - Resets the Task Watchdog Timer on each iteration.
** ===================================================================
*/

void SpotifyPlayer::refreshCurrentSongTask(void *pvParameters) 
{
    spLogI(LOGTAG_MULTITASK, "background task executing.  about to enter loop.");
    // TODO: work this out better so it just simply starts when ready
    vTaskDelay(pdMS_TO_TICKS(2000)); // 2000 ms delay to get started and wait for everything to process
    while (true) {
        spLogI(LOGTAG_MULTITASK, "refreshCurrentSongTask is running");

        // Feed the Task Watchdog to avoid timeout
        esp_task_wdt_reset();

        // Perform the task (blocking HTTP call)
        SpotifyPlayer::getInstance().refreshCurrentTrack();

        // Give the UI time to refresh since might be marked dirty
        // Also give time for other tasks to do work.
        vTaskDelay(pdMS_TO_TICKS(1500)); 

        SpotifyPlayer::getInstance().saveCache();

        // Allow other tasks to execute
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}