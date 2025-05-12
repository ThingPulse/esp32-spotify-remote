/*-------------------------------------------------------------------------------------------------
**
** DiagnosticsView.h
**
**    Declares the DiagnosticsView class, a UIView subclass that provides a
**    diagnostics interface for the Spotify controller. Displays metadata,
**    system status, and playback diagnostics, primarily for debugging purposes.
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
** Class: DiagnosticsView
**
** Purpose:
**    Provides a diagnostics screen for the Spotify controller UI.
**    Displays system status information such as network status,
**    current track data, playback state, and performance metrics.
**
**    This view is accessible via the HomeView and is intended
**    primarily for debugging or advanced user feedback. It inherits
**    from UIView and implements methods for initialization, UI
**    rendering, and message handling during idle time.
** ===================================================================
*/
class DiagnosticsView : public UIView
{
public:
    explicit DiagnosticsView(DisplayUI *pUI); // Constructor
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
    // Time management variables
    unsigned long _requestDueTime = 0;     // time when request due

    void clearAndPaintScreen(PlayingMetadata playing); 
    void refreshScreen(PlayingMetadata playing, bool isForcedRefresh);
    constexpr int  rowToY(int row);
};