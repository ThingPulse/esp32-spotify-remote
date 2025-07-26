/*-------------------------------------------------------------------------------------------------
**
** PlayingMetadata.h
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

#include <Arduino.h>

// Forward declaration (so we don't include SpotifyArduino.h here)
struct CurrentlyPlaying; 

// These need to stay in sync with what is defined in SpotifyArduino.h
#define PLAYING_NAME_CHAR_LENGTH 100
#define PLAYING_URI_CHAR_LENGTH   40
#define PLAYING_URL_CHAR_LENGTH   70
#define PLAYING_MAX_NUM_ARTISTS    5
#define PLAYING_NUM_ALBUM_IMAGES   3


// Define PlayingMetaData struct with nested types
struct PlayingMetadata 
{
    struct SongArtist 
    {
        char artistName[PLAYING_NAME_CHAR_LENGTH];
        char artistUri[PLAYING_URI_CHAR_LENGTH];
    };

    struct AlbumImage 
    {
        int height;
        int width;
        char url[PLAYING_URL_CHAR_LENGTH];
    };

    enum PlayingType 
    {
        track,
        episode,
        other
    };

    SongArtist  artists[PLAYING_MAX_NUM_ARTISTS];
    int         numArtists;
    char        albumName[PLAYING_NAME_CHAR_LENGTH];
    char        albumUri[PLAYING_URI_CHAR_LENGTH];
    char        trackName[PLAYING_NAME_CHAR_LENGTH];
    char        trackUri[PLAYING_URI_CHAR_LENGTH];
    AlbumImage  albumImages[PLAYING_NUM_ALBUM_IMAGES];
    int         numImages;
    bool        isPlaying;
    long        progressMs;
    long        durationMs;
    long        lastRefreshMs;
    char        contextUri[PLAYING_URI_CHAR_LENGTH];
    PlayingType currentlyPlayingType;

    String getArtistsList(size_t maxLength); 

private:
    friend class SpotifyPlayer;
    void copyFrom(const CurrentlyPlaying &source);
    void copyFrom(const PlayingMetadata &source);

};
