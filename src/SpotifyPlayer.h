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
#pragma once

#include <Arduino.h>
#include "DisplayUI.h"
#include "PlayingMetadata.h"
#include "scui.h"
class SpotifyPlayer {
public:
    // Public method to access the singleton instance
    static SpotifyPlayer& getInstance();

    // initialization related
    void   initialize(QueueHandle_t    *pScuiQueue);
    bool   isRefreshTokenAvailable();
    bool   requestRefreshToken();
    String getNodeName();
    void   login();
    void   startBackgroundRefreshes();


    // controls
    void   nextSong();
    void   previousSong();
    void   pauseSong();

    // status
    bool   isMusicAvailable();

    // Call this to get the stable DTO copy
    const PlayingMetadata &getCurrentlyPlayingMetadata();       

    const bool   isNewTrackReady();

private:
    // Member variables
    String              _spotifyRefreshToken   = "";
    QueueHandle_t       *_pScuiQueue; 
    String              _currentTrackUri       = "";        // Tracks the currently playing song
    bool                _isPlaying             = false;
    bool                _isCoverArtAvailable   = false;
    bool                _isMusicAvailable      = false;
    bool                _isNewTrackReady       = false;
    bool                _isNewTrack            = false;
    PlayingMetadata     _currentlyPlayingMetadata;          // Frequently updated instance
    PlayingMetadata     _currentlyPlayingMetadataDTO;       // Stable DTO instance
    SemaphoreHandle_t   _xSemaphoreNetwork     = xSemaphoreCreateMutex();
    SemaphoreHandle_t   _xSemaphoreDataCopy    = xSemaphoreCreateMutex();
    TaskHandle_t        _refreshTaskHandle;
    String              _spotifyClientId;
    String              _spotifyClientSecret;

    // Methods

    // Private constructor and destructor
    SpotifyPlayer();
    ~SpotifyPlayer();

    static void getCurrentlyPlayingCallback(CurrentlyPlaying currentlyPlaying);
    static void refreshCurrentSongTask(void *pvParameters);

    void   copyMetadataToDTO();
    void   printCurrentlyPlayingToSerial(CurrentlyPlaying currentlyPlaying);    
    void   refreshCurrentTrack();
    void   refreshCurrentSong(CurrentlyPlaying currentlyPlaying); // Call back
    void   refreshCoverArt();
    void   postScuiMessage(SCUIMessageType type, const String& str, int num);
    void   saveCache();

};
