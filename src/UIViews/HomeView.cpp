/*-------------------------------------------------------------------------------------------------
**
** HomeView.cpp
**
**    Implements the default HomeView for the Spotify controller UI, displaying
**    track metadata, album art, control buttons, and current playback progress.
**    Inherits from UIView and handles rendering and touch interactions for the
**    main screen. Includes methods for managing UI refresh cycles and state.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-01-21 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include "HomeView.h"
#include "SCLogger.h"
#include "logTags.h"
#include "ThingPulse/util.h"  
#include "SpotifyArtMgr.h"
#include "Renderers/PreviousTrackButtonRenderer.h"
#include "Renderers/PlayPauseButtonRenderer.h"
#include "Renderers/NextTrackButtonRenderer.h"
#include "Vault.h"

/*
** ===================================================================
** Constructor
**    Initializes the HomeView class.
**
** Parameters:
**    pUI - Pointer to the DisplayUI instance.
** ===================================================================
*/
HomeView::HomeView(DisplayUI *pUI) 
    : UIView(pUI) // Call the base class constructor
{
    // Additional initialization (if needed)
    _pUI->setBackground(TFTColor::Navy, true);
}

/*
** ===================================================================
** initializeUIElements()
**
**    Sets up and positions all interactive UIElements used in the HomeView,
**    including media control buttons, navigation targets, and cover art tap zones.
**
** Notes:
**    - Creates shared_ptr instances of IUIRenderer for each control.
**    - Defines specific coordinates and dimensions for button layout.
**    - This method is called during the initial view setup.
** ===================================================================
*/
void HomeView::initializeUIElements() 
{

    // spLogI(LOGTAG_GENERAL,"HomeView::initializeUIElements() ");

    const int32_t buttonSize   = 90;
    const int32_t spacer       = 11;
    const int32_t buttonStartX = 170;
    const int32_t buttonStartY = 40;
    const int32_t coverArtSize = COVER_ART_SIZE / 2;

    std::shared_ptr<IUIRenderer> previousButtonRenderer = std::make_shared<PreviousTrackButtonRenderer>(_pUI);
    _pPreviousElement        = std::make_unique<UIElement>(buttonStartX, buttonStartY, buttonSize, buttonSize, previousButtonRenderer); 

    std::shared_ptr<IUIRenderer> playPauseButtonRenderer = std::make_shared<PlayPauseButtonRenderer>(_pUI); 
    _pPauseElement           = std::make_unique<UIElement>(buttonStartX + spacer + buttonSize, buttonStartY, buttonSize, buttonSize, playPauseButtonRenderer); 

    std::shared_ptr<IUIRenderer> nextButtonRenderer = std::make_shared<NextTrackButtonRenderer>(_pUI);    
    _pNextElement            = std::make_unique<UIElement>(buttonStartX + (spacer + buttonSize) * 2,   buttonStartY, buttonSize, buttonSize, nextButtonRenderer); 

    _pGotoCoverArtElement    = std::make_unique<UIElement>(10, 10, coverArtSize, coverArtSize); 


    _pReturnViewElement      = std::make_unique<UIElement>(  0, 121, 119, 200);  
    _pGotoClockElement       = std::make_unique<UIElement>(300, 221, 100, 200); 
    _pGotoDiagnosticElement  = std::make_unique<UIElement>(400, 221,  80, 200); 


}


/*
** ===================================================================
** enteringView()
** ===================================================================
*/    
void HomeView::enteringView()
{

    // spLogI(LOGTAG_GENERAL, "*** ENTERING  HomeView::enteringView() ===");

    _pUI->setSplitBackground(true);
    
    // Make sure the UI is painted fresh
    _pUI->markUIDirty(true);
    
    drawUI();

    // spLogI(LOGTAG_GENERAL, "*** EXITING  HomeView::enteringView() ===");
}

/*
** ===================================================================
** drawUI()
**    Renders the UI for the default mode.
** ===================================================================
*/
void HomeView::drawUI()
{
    // spLogI(LOGTAG_GENERAL, "==== ENTERING drawUI for HomeView ===");

    SpotifyPlayer  *pSP                        = &SpotifyPlayer::getInstance();
    bool            isNewTrack                 = pSP->isNewTrackReady();
    bool            isDisplayPercentageUpdated = false;

    // Check if music is playing and if not
    // display message and exit
    if (checkAndHandleNoMusicAvailable())
    {
        return;
    }
    
    isDisplayPercentageUpdated = refreshPlayingProgress();

    // If this isn't a new song and the UI
    // isn't dirty, nothing to do so leave.
    if (isNewTrack
    || _pUI->isUIDirty())
    {
        clearAndPaintScreen();
        _pUI->markUIDirty(false);
    }
    else
    {
        if (isDisplayPercentageUpdated)
        {
            // handleProgressBar();
        }
        handleProgressBar();
        handleClock(false);     
        handlePlayStatus();
    }

    // spLogI(LOGTAG_GENERAL, "==== EXITING drawUI for HomeView ===");

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
bool HomeView::refreshPlayingProgress()
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

    if (currentMs < (lastInvokeMs + 100))
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
** handleProgressBar()
**    Updates the progress bar on the screen based on the current
**    display percentage. Additionally, updates and displays the
**    current progress time if it has changed.
**
** Notes:
**    - The progress bar is updated with the current display percentage.
**    - If the progress time has changed, the new progress time is 
**      drawn on the screen.
** ===================================================================
*/
void HomeView::handleProgressBar()
{
    // spLogI(LOGTAG_GENERAL, "   ==== ENTERING  HomeView::handleProgressBar() ===");
    spLogV(LOGTAG_GUI, "Updating progress bar with percentage: %u", _displayPercentage);
    _pUI->drawProgressBar(10, 170, 460, 20, _displayPercentage, TFTColor::White, TFTColor::DarkGreen);

    std::string        progress = _pUI->formatTime(_forecastedProgressMS);

    static std::string lastProgress = "X:XX/X:XX";
    if (lastProgress != progress)
    {
        _pUI->rDrawString(progress.c_str(), 220, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"XX:XX:XX"); 
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
void HomeView::handlePlayStatus()
{
    static bool lastIsPlaying   = false;
    SpotifyPlayer  *pSP         = &SpotifyPlayer::getInstance();
    PlayingMetadata playing     = pSP->getCurrentlyPlayingMetadata();    

    if ((lastIsPlaying != playing.isPlaying)
    ||  (_forceUpdate))
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

            //_pUI->drawText(label.c_str(), 100, 290, 22, TFTColor::DarkGreen);
            _pUI->lDrawString(label.c_str(), 10, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"Track Playing");  

        }
        else
        {
            //_pUI->drawText("Not Playing", 100, 290, 22, TFTColor::Red);
            if (playing.progressMs > 0)
            {
                _pUI->lDrawString("Paused", 10, 287, 20, TFTColor::Red, TFTColor::Black,"Track Playing");
            }
            else
            {
                _pUI->lDrawString("Stopped", 10, 287, 20, TFTColor::Red, TFTColor::Black,"Track Playing");
            }
        }   
        _forceUpdate = false;
    }

    if (lastIsPlaying != playing.isPlaying)
    {
        _pPauseElement->render(false); // refresh button icon
    }

    lastIsPlaying = playing.isPlaying;    
}

/*
** ===================================================================
** handleClock()
**    Updates and displays the current time on the screen. If the 
**    time has changed or a repaint is explicitly requested, the 
**    time is redrawn on the display.
**
** Parameters:
**    isPaintForced - A boolean flag indicating whether a forced repaint
**                    should occur, regardless of whether the time has 
**                    changed.
**
** Notes:
**    - The time is displayed in the format "XX:XX PM" if US formatting is used.
**    - If the current time differs from the last displayed time or 
**      if a forced repaint is requested, the time is redrawn.
** ===================================================================
*/
void HomeView::handleClock(bool isPaintForced)
{
    std::string       format = UI_TIME_FORMAT;
    int               offset = 10;
    if (Vault::getInstance().isUSDateTimeFormattingUsed())
    {
        format = UI_TIME_FORMAT_US;
        offset = 0;
    }
    std::string        timeStr  = getCurrentTimestamp(format.c_str()).c_str();         
    static std::string lastTime = "X:XX/X:XX";
    if ((lastTime != timeStr)
    ||  (isPaintForced))
    {            
        _pUI->lDrawString(timeStr.c_str(), 310 + offset, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"XX:XX PM");    
        lastTime = timeStr;
    }    
}

/*
** ===================================================================
** clearAndPaintScreen()
**    Clears the screen and paints the current playback information.
**
** Description:
**    This method clears the display and updates it with the currently 
**    playing track's metadata, including album art, control buttons, 
**    progress bar, track details, duration, and the current clock time.
**    It retrieves the album art file path, calculates the average 
**    background color, and paints the UI elements to display a fresh 
**    view of the current playback state.
**
** Notes:
**    - This method forces a UI update even if the content has not changed.
**    - Progress bar is drawn twice to ensure proper clearing of the previous state.
**    - The background color is dynamically calculated based on the album art.
** 
** Parameters:
**    None
**
** Returns:
**    void
** ===================================================================
*/
void HomeView::clearAndPaintScreen()
{
    SpotifyPlayer  *pSP         = &SpotifyPlayer::getInstance();
    PlayingMetadata playing     = pSP->getCurrentlyPlayingMetadata();

    spLogV(LOGTAG_GUI, "Song has changed or the UI is dirty. Painting.");

    _forceUpdate = true;
    
    ///// Get cover art file path

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

    ///// Calculate background color and clear screen

    TFTColor c = _pUI->calculateAverageColor(filePath.c_str());
    _pUI->setBackground(c, false); //true);
    _pUI->clearScreenHome();    

 
    ///// Draw album art
       
    _pUI->setJpgScaleToSmall(true);
    _pUI->drawAlbumArt(10, 10, filePath);

    ///// Draw control buttons
       
    _pPreviousElement->render(false);
    _pPauseElement->render(false);
    _pNextElement->render(false);
   
    ///// Draw progress bar. Draw 0 percent one to force bar to be fully cleared
    ///// otherwise the black will be erased by the screen clear
    spLogV(LOGTAG_GUI, "Updating progress bar with percentage: %u", _displayPercentage);    
    _pUI->drawProgressBar(10, 170, 460, 20, 0, TFTColor::White, TFTColor::DarkGreen);        

    ///// Draw album details    
    _pUI->drawTextToLCD(truncateString(playing.trackName, 36).c_str(), 200, 24, false);
    _pUI->drawTextToLCD(truncateString(playing.albumName, 50).c_str(),230, 18, false);
    _pUI->drawTextToLCD(truncateString(playing.getArtistsList(800).c_str(), 50).c_str(),254, 18, false);      

    ///// Draw play duration
    _pUI->rDrawString("-:--", 220, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"XX:XX:XX"); 
    std::string duration = "/"+_pUI->formatTime(playing.durationMs);
    _pUI->lDrawString(duration.c_str(), 223, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"XX:XX:XX");     
    handleProgressBar();

    ///// Draw play status
    _pUI->lDrawString("Refreshing", 10, 287, 20, TFTColor::DarkGreen, TFTColor::Black,"Track Playing");  

    ///// Draw clock
    handleClock(true);

}

/*
** ===================================================================
** handle_UUM_IDLE()
** ===================================================================
*/
void HomeView::handle_UM_IDLE(SCUIMessage *pMessage)
{
    const unsigned long delayBetweenRequests = 500;

    // Every period refresh the display
    if (millis() > _requestDueTime)
    {
        drawUI();
        _requestDueTime = millis() + delayBetweenRequests;
    } 

}