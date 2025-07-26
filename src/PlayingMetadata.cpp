/*-------------------------------------------------------------------------------------------------
**
** PlayingMetadata.cpp
**
**    Defines the PlayingMetadata structure used to represent Spotify playback data.
**    Includes song, artist, album, and playback timing metadata. Supports copying
**    from Spotify API responses and formatting artist display strings.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-02-02 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include "PlayingMetadata.h"
#include <Arduino.h>
#include <cstring>
#include "SpotifyArduino.h"

/*
** ===================================================================
** copyFrom()
**    Copies data from a CurrentlyPlaying instance into a 
**    PlayingMetadata instance.  lastRefreshMs set to time
**    of operation.
** ===================================================================
*/
void PlayingMetadata::copyFrom(const CurrentlyPlaying &source) 
{
    numArtists           = source.numArtists;
    numImages            = source.numImages;
    isPlaying            = source.isPlaying;
    progressMs           = source.progressMs;
    durationMs           = source.durationMs;
    lastRefreshMs        = millis();  
    currentlyPlayingType = static_cast<PlayingType>(source.currentlyPlayingType);

    strncpy(albumName,  source.albumName  ? source.albumName  : "", PLAYING_NAME_CHAR_LENGTH);
    strncpy(albumUri,   source.albumUri   ? source.albumUri   : "", PLAYING_URI_CHAR_LENGTH);
    strncpy(trackName,  source.trackName  ? source.trackName  : "", PLAYING_NAME_CHAR_LENGTH);
    strncpy(trackUri,   source.trackUri   ? source.trackUri   : "", PLAYING_URI_CHAR_LENGTH);
    strncpy(contextUri, source.contextUri ? source.contextUri : "", PLAYING_URI_CHAR_LENGTH);

    for (int i = 0; i < numArtists; i++) 
    {
        strncpy(artists[i].artistName, source.artists[i].artistName ? source.artists[i].artistName : "", PLAYING_NAME_CHAR_LENGTH);
        strncpy(artists[i].artistUri,  source.artists[i].artistUri  ? source.artists[i].artistUri  : "", PLAYING_URI_CHAR_LENGTH);
    }

    for (int i = 0; i < numImages; i++) 
    {
        albumImages[i].height = source.albumImages[i].height;
        albumImages[i].width  = source.albumImages[i].width;
        strncpy(albumImages[i].url, source.albumImages[i].url ? source.albumImages[i].url : "", PLAYING_URL_CHAR_LENGTH);
    }
}

/*
** ===================================================================
** copyFrom()
**    Copies data from one PlayingMetadata instance to another.
** ===================================================================
*/
void PlayingMetadata::copyFrom(const PlayingMetadata &source) 
{
    numArtists           = source.numArtists;
    numImages            = source.numImages;
    isPlaying            = source.isPlaying;
    progressMs           = source.progressMs;
    durationMs           = source.durationMs;
    lastRefreshMs        = source.lastRefreshMs;
    currentlyPlayingType = source.currentlyPlayingType;

    strncpy(albumName,  source.albumName,  PLAYING_NAME_CHAR_LENGTH);
    strncpy(albumUri,   source.albumUri,   PLAYING_URI_CHAR_LENGTH);
    strncpy(trackName,  source.trackName,  PLAYING_NAME_CHAR_LENGTH);
    strncpy(trackUri,   source.trackUri,   PLAYING_URI_CHAR_LENGTH);
    strncpy(contextUri, source.contextUri, PLAYING_URI_CHAR_LENGTH);

    for (int i = 0; i < numArtists; i++) 
    {
        strncpy(artists[i].artistName, source.artists[i].artistName, PLAYING_NAME_CHAR_LENGTH);
        strncpy(artists[i].artistUri,  source.artists[i].artistUri,  PLAYING_URI_CHAR_LENGTH);
    }

    for (int i = 0; i < numImages; i++) 
    {
        albumImages[i].height = source.albumImages[i].height;
        albumImages[i].width  = source.albumImages[i].width;
        strncpy(albumImages[i].url, source.albumImages[i].url, PLAYING_URL_CHAR_LENGTH);
    }
}

/*
** ===================================================================
** getArtistsList()
**    Returns a String of all the artists for the track, adding an 
**    ellipsis ("...") if the length exceeds maxLength.
** ===================================================================
*/
String PlayingMetadata::getArtistsList(size_t maxLength) 
{
    String result = "";
    for (int i = 0; i < numArtists; i++) 
    {
        if (!result.isEmpty()) 
        {
            result += ", "; // Add a comma and space before each artist after the first
        }
        result += artists[i].artistName;

        // Check if the current result exceeds the max length
        if (result.length() > maxLength) 
        {
            result = result.substring(0, maxLength - 3) + "..."; // Add ellipsis
            break;
        }
    }
    return result;
}