/*-------------------------------------------------------------------------------------------------
**
** UIView.h
**
**    Abstract base class for UI views in the Spotify controller. Each view defines
**    its own rendering behavior and touch handling logic. Provides shared logic
**    for message handling, initialization, and screen transitions.
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

#include <string>
#include "FT6236TouchController/FT6236.h"
#include "SpotifyPlayer.h"
#include "UIElement.h"

/*
** ===================================================================
** Abstract Base Class: UIView
**
** Purpose:
**    Provides an interface for defining display modes in the UI. Each
**    display mode defines specific rendering behavior and may include
**    common setup logic shared across modes.
**
** Notes:
**    - Derived classes must implement the `drawUI` method.
**    - Includes a concrete utility method for displaying messages.
** ===================================================================
*/
class UIView
{
public:

    UIView(DisplayUI *pUI);
    virtual ~UIView() = default;

    // ===================================================================
    virtual void drawUI() = 0;
    virtual void handleMessage(SCUIMessage *pMessage); 

    // ===================================================================
    virtual void onTouchDown(const TS_Point& point);
    virtual void onTouchUp();

    // ===================================================================
    void initialize();

protected:

    // Player
    SpotifyPlayer& _spotifyPlayer = SpotifyPlayer::getInstance();    

    // UI Elements
    std::unique_ptr<UIElement>    _pPauseElement;
    std::unique_ptr<UIElement>    _pPreviousElement;
    std::unique_ptr<UIElement>    _pNextElement;
    std::unique_ptr<UIElement>    _pGotoCoverArtElement;
    std::unique_ptr<UIElement>    _pGotoDiagnosticElement;
    std::unique_ptr<UIElement>    _pGotoClockElement;
    std::unique_ptr<UIElement>    _pReturnViewElement;

    // UI
    DisplayUI                     *_pUI;
    bool                          _isWaitingMessageShowing = false;

    // Protected method to initialize UI elements
    virtual void initializeUIElements();

    // Transitions
    virtual void enteringView();
    virtual void leavingView();

    // UI Messages to support
    virtual void handle_UM_IDLE(SCUIMessage *pMessage);               // UM_IDLE
    virtual void handle_UM_STATUS_BOX(SCUIMessage *pMessage);         // UM_STATUS_BOX
    virtual void handle_UM_DOWNLOAD_BOX(SCUIMessage *pMessage);       // UM_DOWNLOAD_BOX
    virtual void handle_UM_MARK_DIRTY(SCUIMessage *pMessage);         // UM_MAKE_DIRTY    
    virtual void handle_UM_PLAYER_REFRESH(SCUIMessage *pMessage);     // UM_PLAYER_REFRESH

    // UI Refresh methods
    virtual bool checkAndHandleNoMusicAvailable();

    // Let the UIViewManager to call the transition methods.
    friend class UIViewManager;


};
