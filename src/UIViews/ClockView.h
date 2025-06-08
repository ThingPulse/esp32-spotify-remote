/*-------------------------------------------------------------------------------------------------
**
** ClockView.h
**
**    Declares the ClockView class, a UIView subclass that displays the
**    current time and date alongside minimal Spotify playback information.
**    Includes progress bar handling, play status indication, and a return
**    button for navigation.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com  
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-02-12 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#pragma once
#include "UIView.h"

/*
** ===================================================================
** Class: ClockView
**
** Purpose:
**    Implements the "Clock" display mode for the Spotify controller UI.
**    Displays the current time and date alongside minimal Spotify playback
**    information such as progress and playback status. 
** ===================================================================
*/

class ClockView : public UIView
{
public:
    explicit ClockView(DisplayUI *pUI); // Constructor
    virtual void initializeUIElements() override;

    /*
    ** ===================================================================
    ** drawUI()
    **    Renders the UI for the art-only mode.
    ** ===================================================================
    */
    void drawUI() override;

protected:
    // Transitions
    virtual void enteringView() override;
    virtual void handle_UM_IDLE(SCUIMessage *pMessage) override;

private:
    void handleProgressBar(TFTColor progressBarColor);
    void handlePlayStatus(bool isForcedUpdate);  
    void handleDate(bool isForceRefresh);
    bool refreshPlayingProgress();
    // Time management variables
    unsigned long _requestDueTime = 0;            // time when request due
    uint8_t       _displayPercentage    = 0;      // what is being displayed as the percentage
    long          _forecastedProgressMS = 0;      // forcasted progress into the song  
};