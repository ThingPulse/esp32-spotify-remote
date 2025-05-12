/*-------------------------------------------------------------------------------------------------
**
** HomeView.h
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

#pragma once
#include "UIView.h"

/*
** ===================================================================
** Class: HomeView
**
** Purpose:
**    Implements the default display mode, showing song, album, and
**    artist details over cover art.
** ===================================================================
*/
class HomeView : public UIView
{
public:
    explicit     HomeView(DisplayUI *pUI); // Constructor
    virtual void initializeUIElements() override;
    void         drawUI() override;
    virtual void handle_UM_IDLE(SCUIMessage *pMessage) override;

protected:
    void enteringView() override;

private:
    // Time management variables
    unsigned long _requestDueTime       = 0;      // time when request due
    bool          _forceUpdate          = false;  // should an UI refresh be forced?
    uint8_t       _displayPercentage    = 0;      // what is being displayed as the percentage
    long          _forecastedProgressMS = 0;      // forcasted progress into the song  

    bool refreshPlayingProgress();
    void handleProgressBar();
    void handlePlayStatus();  
    void handleClock(bool isPaintForced);  
    void clearAndPaintScreen(); 




};