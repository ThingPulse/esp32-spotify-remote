/*-------------------------------------------------------------------------------------------------
**
** ClockView.cpp
**
**    Declares the ClockView class, a UIView subclass that displays the
**    current time and date alongside minimal Spotify playback information.
**    Includes progress bar handling, play status indication, and a return
**    button for navigation. Intended as an idle display mode.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com  
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-02-12 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include "ClockView.h"
#include "SCLogger.h"
#include "logTags.h"
#include "ThingPulse/util.h"  
#include "SpotifyArtMgr.h"
#include "Renderers/BackButtonRenderer.h"
#include "Vault.h"

/*
** ===================================================================
** Constructor
**    Initializes the CoverView with the provided DisplayUI instance.
**
** Parameters:
**    pUI - Pointer to the DisplayUI instance.
** ===================================================================
*/
ClockView::ClockView(DisplayUI *pUI)
    : UIView(pUI) // Pass the DisplayUI pointer to the base class constructor
{
    // Initialization specific to CoverView, if any
}

/*
** ===================================================================
** initializeUIElements()
**
**    Initializes the interactive UI elements used in the ClockView,
**    including the return button and album art tap zone. Enlarges the
**    touch area of the back button to improve tap reliability. Also calls
**    the base UIView initializer to set up any shared UI elements.
**
** Notes:
**    - `_pReturnViewElement` uses BackButtonRenderer and a padded touch zone.
**    - `_pGotoCoverArtElement` is a simple zone intended for tapping the album art.
**    - Be sure to call this during view setup before rendering.
** ===================================================================
*/
void ClockView::initializeUIElements() 
{

    std::shared_ptr<IUIRenderer> backButtonRenderer = std::make_shared<BackButtonRenderer>(_pUI);
    // Make the touch rectangle 10 pixels larger on all sides.  The renderer will take off the 10 pixels when rendering
    // Doing to make a bigger, more forgiving touch target.
    const int MARGIN = 10;
    _pReturnViewElement      = std::make_unique<UIElement>(  10 - MARGIN, 276 - MARGIN, 70 + (MARGIN * 2), 34 + (MARGIN * 2), backButtonRenderer);  
    _pGotoCoverArtElement    = std::make_unique<UIElement>(  0, 160, 40, 40); 

    // initialize other elements not set above
    UIView::initializeUIElements();

}

/*
** ===================================================================
** drawUI()
**    Renders the UI for the default mode.
** ===================================================================
*/
void ClockView::drawUI()
{

    SpotifyPlayer  *pSP         = &SpotifyPlayer::getInstance();
    bool            isNewTrack  = pSP->isNewTrackReady(); // copy this before resetting it with getCurrentlyPlayingMetadata()
    PlayingMetadata playing     = pSP->getCurrentlyPlayingMetadata();
    bool            isDisplayPercentageUpdated = false;
    static TFTColor progressBarColor = TFTColor::DarkGreen; 

    // Update clock
    if (millis() > _requestDueTime)
    {
        handleDate(false);
        _pUI->drawClockTime(Vault::getInstance().isUSDateTimeFormattingUsed(),false);
        char s[50];
        snprintf(s, sizeof(s), "%s", getCurrentTimestamp("%Y-%m-%d %H:%M:%S").c_str());        
    }

    isDisplayPercentageUpdated = refreshPlayingProgress();

    static bool isNoMusicMsgShown = false;
    if ((pSP->isMusicAvailable() == false)
    &&  !isNoMusicMsgShown)
    {
        _pUI->drawTextToLCD("No Music Playing...", 200, 24, false);
        _pUI->drawTextToLCD("", 230, 18, false);
        _pUI->drawTextToLCD("", 254, 18, false);   
        isNoMusicMsgShown = true;

        // Render back button
        TFTColor c = _pUI->getBackground();
        _pUI->setBackground(TFTColor::Black,false); 
        _pReturnViewElement->render(false);
        _pUI->setBackground(c,false);     
        handlePlayStatus(true);

        return;
    }

    isNoMusicMsgShown = false;

    if (isNewTrack
        || _pUI->isUIDirty())
    {

        handleDate(true);

        _pUI->drawTextToLCD(truncateString(playing.trackName, 36).c_str(), 200, 24, false);
        _pUI->drawTextToLCD(truncateString(playing.albumName, 50).c_str(),230, 18, false);
        _pUI->drawTextToLCD(truncateString(playing.getArtistsList(800).c_str(), 50).c_str(),254, 18, false);    

        // Render back button
        TFTColor c = _pUI->getBackground();
        _pUI->setBackground(TFTColor::Black,false); 
        _pReturnViewElement->render(false);
        _pUI->setBackground(c,false);  

        String filePath = SP_NO_COVER_JPG_FILENAME;

        const int TARGET_IMAGE_SIZE  = 300;    
    
        for (int i = 0; i < playing.numImages; i++)
        {
            if ((playing.albumImages[i].height == TARGET_IMAGE_SIZE)
            &&  (playing.albumImages[i].width  == TARGET_IMAGE_SIZE))
            {
                filePath = SpotifyArtMgr::getInstance()->getLocalFileName(playing.albumImages[i].url);
                break;
            }
        }   
         
        ///// Draw album art
        progressBarColor = _pUI->calculateAverageColor(filePath.c_str());

        _pUI->drawProgressBar(60, 170, 410, 20, 0, TFTColor::White, TFTColor::Black);
        _pUI->drawProgressBar(60, 170, 410, 20, _displayPercentage, TFTColor::White, progressBarColor);
           
        _pUI->setJpgScaleToTiny();     
        _pUI->drawAlbumArt(10, 160, filePath); 

        ///// Draw play duration
        _pUI->rDrawString("-:--", 330, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"XX:XX:XX"); 
        std::string duration = "/"+_pUI->formatTime(playing.durationMs);
        _pUI->lDrawString(duration.c_str(), 333, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"XX:XX:XX");       
        
        handlePlayStatus(true);

        _pUI->markUIDirty(false);
    }

    handleProgressBar(progressBarColor);   
    handlePlayStatus(false); 
    
}

/*
** ===================================================================
** handleProgressBar()
**
**    Updates the visual progress bar on the ClockView screen to reflect
**    the current playback position. Uses `_displayPercentage` to determine
**    how much of the bar to fill and updates the on-screen timestamp if it
**    has changed since the last frame.
**
** Parameters:
**    progressBarColor - The color to use for the filled portion of the bar.
**
** Notes:
**    - Uses a cached `lastProgress` string to avoid unnecessary redraws.
**    - The timestamp is displayed to the right of the progress bar.
** ===================================================================
*/
void ClockView::handleProgressBar(TFTColor progressBarColor)
{
    // spLogI(LOGTAG_GENERAL, "   ==== ENTERING  HomeView::handleProgressBar() ===");
    spLogV(LOGTAG_GUI, "Updating progress bar with percentage: %u", _displayPercentage);
    _pUI->drawProgressBar(60, 170, 410, 20, _displayPercentage, TFTColor::White, progressBarColor);

    std::string        progress = _pUI->formatTime(_forecastedProgressMS);

    static std::string lastProgress = "X:XX/X:XX";
    if (lastProgress != progress)
    {
        _pUI->rDrawString(progress.c_str(), 330, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"XX:XX:XX"); 
        lastProgress = progress;
    }

    // spLogI(LOGTAG_GENERAL, "   ==== EXITING  HomeView::handleProgressBar() ===");
}

/*
** ===================================================================
** handlePlayStatus()
**    Handles the logic for updating the UI based on the current
**    playback status of the Spotify player, including whether a track
**    is playing, paused, or stopped. It also updates the play/pause
**    button icon when the playback state changes.
**
** Parameters:
**    None
**
** Returns:
**    None
**
** Notes:
**    - This method updates the UI text to display the current playback
**      status (e.g., "Track Playing", "Paused", or "Stopped").
**    - It ensures that the play/pause button icon is refreshed whenever
**      the playback status changes.
** ===================================================================
*/
void ClockView::handlePlayStatus(bool isForcedUpdate)
{
    static bool     lastIsPlaying   = false;
    SpotifyPlayer  *pSP             = &SpotifyPlayer::getInstance();
    PlayingMetadata playing         = pSP->getCurrentlyPlayingMetadata();    
    const int32_t   posX            = 100;

    if ((lastIsPlaying != playing.isPlaying)
    ||  (isForcedUpdate))
    {
        if (playing.isPlaying)
        {
            std::string label;
            switch (playing.currentlyPlayingType)
            {
                case PlayingMetadata::PlayingType::track:
                    label = "Track Playing";
                    break;
                case PlayingMetadata::PlayingType::episode:
                    label = "Ep. Playing";
                    break;
                default:
                    label = "Other Playing";
                    break;
            }

            _pUI->lDrawString(label.c_str(), posX, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"Track Playing");  

        }
        else
        {

            if (playing.progressMs > 0)
            {
                _pUI->lDrawString("Paused", posX, 287, 20, TFTColor::Red, TFTColor::Black,"Track Playing");
            }
            else
            {
                _pUI->lDrawString("Stopped", posX, 287, 20, TFTColor::Red, TFTColor::Black,"Track Playing");
            }
        }   

    }

    if (lastIsPlaying != playing.isPlaying)
    {
        _pPauseElement->render(false); // refresh button icon
    }

    lastIsPlaying = playing.isPlaying;    
}

/*
** ===================================================================
** handleDate()
**    Draws the current date centered above the clock time. Only
**    updates the display if the date string has changed or if
**    forced refresh logic is added later.
**
** Notes:
**    - Uses the format: "Saturday March 30 2025"
** ===================================================================
*/
void ClockView::handleDate(bool isForceRefresh)
{
    std::string          format = UI_DATE_FORMAT;
    if (Vault::getInstance().isUSDateTimeFormattingUsed())
    {
        format = UI_DATE_FORMAT_US;
    }
    std::string        dateStr  = getCurrentTimestamp(format.c_str()).c_str();
    static std::string lastDate = "";

    if ((lastDate != dateStr)
    ||  isForceRefresh)
    {
        _pUI->cDrawString(dateStr.c_str(), 240, 8, 24, TFTColor::Yellow, _pUI->getBackground(), "");
        lastDate = dateStr;
    }
}

/*
** ===================================================================
** refreshPlayingProgress()
**    Updates the current progress of the playing media, calculating 
**    the forecasted progress and updating the display percentage.
**    The _forecastedProgressMS member variable will be updated 
**    during the execution of this method.
**
** Returns:
**    A boolean indicating whether the display percentage has been 
**    updated.
**
** Notes:
**    - The progress is only updated if the media is currently playing.
**    - This method skips progress updates if called too frequently 
**      (within a 100 ms threshold).
**    - The updated forecasted progress is used to calculate the 
**      display percentage based on the media's total duration.
** ===================================================================
*/
bool ClockView::refreshPlayingProgress()
{
    SpotifyPlayer  *pSP         = &SpotifyPlayer::getInstance();
    bool            isNewTrack  = pSP->isNewTrackReady();
    PlayingMetadata playing     = pSP->getCurrentlyPlayingMetadata();

  
    static long  lastInvokeMs               = 0;
    bool         isDisplayPercentageUpdated = false;
    long         currentMs                  = millis();

    if (isNewTrack)
    {
        // Reset back to zero
        lastInvokeMs          = 0; 
        _forecastedProgressMS = playing.progressMs;
    }

    if (currentMs < (lastInvokeMs + 25))
    {
        // Not ready to update yet
        isDisplayPercentageUpdated = false;
        spLogV(LOGTAG_GUI, "bSkipProgressBarRefresh = true.  currentMS: %u lastInvokeMs: %u", currentMs, lastInvokeMs);
    }
    else if (playing.isPlaying)  // 🔹 Only update if song is actually playing
    {
        // Song is playing, so update forecasted progress normally
        if (playing.lastRefreshMs < lastInvokeMs)
        {
            long difference = currentMs - lastInvokeMs;
            _forecastedProgressMS += difference;
            if (_forecastedProgressMS > playing.durationMs)
            {
                _forecastedProgressMS = playing.durationMs;
            }
            spLogV(LOGTAG_GUI, "Updating forecastedProgressMS. prev forecastedProgressMS: %u difference: %u", _forecastedProgressMS, difference);
        }
        else
        {
            // Song progress has changed naturally (not paused), so trust new progressMs
            _forecastedProgressMS = playing.progressMs;
        }
        _displayPercentage         = (playing.durationMs > 0) ? (_forecastedProgressMS * 100) / playing.durationMs : 0;
        _displayPercentage         = (_displayPercentage > 100) ? 100 : _displayPercentage;
        isDisplayPercentageUpdated = true;
    }

    // Update tracking variables

    lastInvokeMs          = millis();    

    return isDisplayPercentageUpdated;
}

/*
** ===================================================================
** enteringView()
** ===================================================================
*/    
void ClockView::enteringView()
{
    _pUI->setSplitBackground(true);
    // Make sure the UI is painted fresh
    _pUI->setBackground(TFTColor::DarkestGreen, false);
    _pUI->clearScreenHome();  
    _pUI->drawClockTime(Vault::getInstance().isUSDateTimeFormattingUsed(), true);
    _pUI->markUIDirty(true); // force refresh to get album details

}

/*
** ===================================================================
** handle_UUM_IDLE()
** ===================================================================
*/
void ClockView::handle_UM_IDLE(SCUIMessage *pMessage)
{
    const unsigned long delayBetweenRequests = 50;

    // Every period refresh the display
    if (millis() > _requestDueTime)
    {
        drawUI();
        _requestDueTime = millis() + delayBetweenRequests;
    } 

}