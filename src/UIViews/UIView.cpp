/*-------------------------------------------------------------------------------------------------
**
** UIView.cpp
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

#include "UIView.h"
#include "DisplayUI.h"
#include "SCLogger.h"
#include "logTags.h"
#include "UIViewManager.h"

#include <Arduino.h> // Assuming Arduino framework for Serial.print
#include <memory>    // Required for std::make_unique and std::unique_ptr

/*
** ===================================================================
** initialize()
**    Prepares the display mode by displaying a setup message.
** ===================================================================
*/
void UIView::initialize()
{
    initializeUIElements();
}

/*
** ===================================================================
** UIView Constructor
**    Initializes the UIView object with the provided DisplayUI
**    object and sets up default UI elements.
**
** Parameters:
**    pUI - Pointer to the DisplayUI object, which provides rendering
**          capabilities and access to the display context.
**
** Purpose:
**    - Store the reference to the DisplayUI object for use in UI
**      rendering and interaction.
**    - Call the initializeUIElements() method to set up default
**      positions and sizes for the UI elements.
**
** Notes:
**    - This constructor assumes that the DisplayUI object passed in
**      has been properly initialized by the caller.
**    - The initializeUIElements() method can be overridden in
**      subclasses to customize the UI layout for specific display modes.
**
** ===================================================================
*/
UIView::UIView(DisplayUI *pUI) 
{
    _pUI = pUI;
}

/*
** ===================================================================
** initializeUIElements
**    Initializes the UI elements with default positions and sizes.
**
** Purpose:
**    - Assign default values to the UI elements (e.g., pause, previous,
**      next, and toggle art buttons).
**    - This method can be overridden or extended by subclasses to
**      provide a custom layout specific to the display mode.
**
** Notes:
**    - Default values are provided as placeholders; real positions
**      and dimensions should be adjusted as needed.
**    - Uses std::unique_ptr to manage the UIElement objects, ensuring
**      proper memory management and avoiding manual cleanup.
**
** ===================================================================
*/
void UIView::initializeUIElements() 
{

    // spLogI(LOGTAG_GENERAL,"UIView::initializeUIElements() ");

    // Assign default positions and sizes for UI elements
    // 480x320

    if (!_pPauseElement)
    {
        _pPauseElement           = std::make_unique<UIElement>(101,   0, 259, 120); 
    }

    if (!_pPreviousElement)
    {
        _pPreviousElement        = std::make_unique<UIElement>(  0,   0, 100, 120); 
    }
    
    if (!_pNextElement)
    {
        _pNextElement            = std::make_unique<UIElement>(360,   0, 100, 120); 
    }
    
    if (!_pGotoCoverArtElement)
    {
        _pGotoCoverArtElement    = std::make_unique<UIElement>(120, 121, 160, 200);  
    }
    
    if (!_pGotoClockElement)
    {
        _pGotoClockElement       = std::make_unique<UIElement>(280, 121, 200, 100); 
    }    

    if (!_pGotoDiagnosticElement)
    {
        _pGotoDiagnosticElement  = std::make_unique<UIElement>(280, 221, 200, 100); 
    }  
    
    if (!_pReturnViewElement)
    {
        _pReturnViewElement      = std::make_unique<UIElement>(  0, 121, 119, 200); 
    }      

}

/*
** ===================================================================
** enteringView()
** ===================================================================
*/    
void UIView::enteringView()
{

    _pUI->setSplitBackground(false);

    // Make sure the UI is painted fresh
    _pUI->markUIDirty(true);
    
    drawUI();
}

/*
** ===================================================================
** leavingView()
** ===================================================================
*/    
void UIView::leavingView()
{
    // spLogI(LOGTAG_GENERAL, "*** ENTERING  UIView::leavingView() ===");
    _pUI->clearScreen();
    // spLogI(LOGTAG_GENERAL, "*** EXITING  UIView::leavingView() ===");

}

/*
** ===================================================================
** onTouchDown()
**    Handles the event when the screen is touched (press event).
**
** Parameters:
**    point - The touch point representing the coordinates of the press
**            event on the screen.
**
** Notes:
**    - Default implementation logs the touch-down event. Derived classes
**      can override this to provide specific behavior.
** ===================================================================
*/
void UIView::onTouchDown(const TS_Point& point)
{
    spLogD(LOGTAG_DISPLAY_MODE, "Default onTouchDown at (%d, %d)", point.x, point.y);

    // Spotify Player
    SpotifyPlayer& spotifyPlayer = SpotifyPlayer::getInstance();   

    if (_pPauseElement 
    &&  _pPauseElement->isPressed(point)) 
    {
        _pUI->showTouchDown(TFTColor::SC_PauseTrack);
        _pPauseElement->render(true);
        spotifyPlayer.pauseSong(); 
    }
    else if (_pPreviousElement 
         &&  _pPreviousElement->isPressed(point)) 
    {
        _pUI->showTouchDown(TFTColor::SC_PreviousTrack);
        _pPreviousElement->render(true);
        spotifyPlayer.previousSong();  
    }
    else if (_pNextElement 
         &&  _pNextElement->isPressed(point)) 
    {
        _pUI->showTouchDown(TFTColor::SC_NextTrack);
        _pNextElement->render(true);
        spotifyPlayer.nextSong();      
    }
    else if (_pGotoCoverArtElement 
         &&  _pGotoCoverArtElement->isPressed(point)) 
    {
        // Handle art button press
        UIViewManager::getInstance().gotoView(UIViewManager::ViewID::Cover);
    }
    else if (_pGotoDiagnosticElement
         &&  _pGotoDiagnosticElement->isPressed(point))
    {
        // Handle goto Diagnostic view
        UIViewManager::getInstance().gotoView(UIViewManager::ViewID::Diagnostics);
    }
    else if (_pReturnViewElement
         &&  _pReturnViewElement->isPressed(point))
    {
        // Handle return view
        _pReturnViewElement->render(true);
        if (!UIViewManager::getInstance().exitView())
        {
            // if top view, simply force a refresh
            UIViewManager::getInstance().getActiveView()->drawUI();
        }
    }
    else if (_pGotoClockElement
         &&  _pGotoClockElement->isPressed(point))
    {
        // Handle got Clock view
        UIViewManager::getInstance().gotoView(UIViewManager::ViewID::Clock);
    }
}

/*
** ===================================================================
** onTouchUp()
**    Handles the event when the screen touch is lifted (release event).
**
** Parameters:
**    point - The touch point representing the coordinates of the release
**            event on the screen.
**
** Notes:
**    - Default implementation logs the touch-up event. Derived classes
**      can override this to provide specific behavior.
** ===================================================================
*/
void UIView::onTouchUp()
{
    spLogD(LOGTAG_DISPLAY_MODE, "Default onTouchUp()");
    _pUI->showTouchUp();
    _pPreviousElement->render(false);
    _pPauseElement->render(false);
    _pNextElement->render(false);
    _pReturnViewElement->render(false);
}

/*
** ===================================================================
** handleMessage()
** ===================================================================
*/    
void UIView::handleMessage(SCUIMessage *pMessage)
{
    switch (pMessage->type)
    {
        case SCUIMessageType::UM_IDLE:
            handle_UM_IDLE(pMessage);
            break;
        case SCUIMessageType::UM_STATUS_BOX:
            handle_UM_STATUS_BOX(pMessage);
            break;
        case SCUIMessageType::UM_DOWNLOAD_BOX:
            handle_UM_DOWNLOAD_BOX(pMessage);
            break;
        case SCUIMessageType::UM_MARK_DIRTY:
            handle_UM_MARK_DIRTY(pMessage);
            break;
        case SCUIMessageType::UM_PLAYER_REFRESH:
            handle_UM_PLAYER_REFRESH(pMessage);
            break;
        default:
            spLogW(LOGTAG_GUI, "Unknown message type received.");
            break;
    }
}

/*
** ===================================================================
** handle_UM_STATUS_BOX()
**     is the provided color is negative, use the existing background
**     color.
** ===================================================================
*/
void UIView::handle_UM_STATUS_BOX(SCUIMessage *pMessage)
{
    TFTColor c = TFTColor::Black;

    if (!_pUI->isSplitBackground())
    {
        c = _pUI->getBackground();
    }

    if (pMessage->num > 0)
    {
        c = static_cast<TFTColor>(pMessage->num);
    }
    _pUI->drawStatusBox(c);
}

/*
** ===================================================================
** handle_UM_DOWNLOAD_BOX()
** ===================================================================
*/
void UIView::handle_UM_DOWNLOAD_BOX(SCUIMessage *pMessage)
{
    bool isDownloadComplete = pMessage->num;  // Get the status
    _pUI->drawDownloadIndicator(isDownloadComplete);
}

/*
** ===================================================================
** handle_UM_MARK_DIRTY()
** ===================================================================
*/
void UIView::handle_UM_MARK_DIRTY(SCUIMessage *pMessage)
{
    bool isDirty = pMessage->num;  // Get the status
    _pUI->markUIDirty(isDirty);
}

/*
** ===================================================================
** handle_UM_PLAYER_REFRESH()
** ===================================================================
*/
void UIView::handle_UM_PLAYER_REFRESH(SCUIMessage *pMessage)
{
    bool isNewTrackReady = pMessage->num;  // Get whether this is a new track

    drawUI();

}

/*
** ===================================================================
** checkAndHandleNoMusicAvailable()
**    Checks if music is available and handles the "Waiting for song" 
**    message if necessary.
**
** Description:
**    This method checks if music is currently available through the 
**    SpotifyPlayer instance. If no music is available, it updates the 
**    UI to display a "Waiting for song..." message, sets the background, 
**    and logs the event.
**
** Returns:
**    true  - If the "Waiting for song" message was shown.
**    false - If music is available, or no action was taken.
** ===================================================================
*/
bool UIView::checkAndHandleNoMusicAvailable()
{
    SpotifyPlayer  *pSP         = &SpotifyPlayer::getInstance();

    if (pSP->isMusicAvailable() == false)
    {
        if (!_isWaitingMessageShowing)
        {
            spLogI(LOGTAG_GUI, "drawCurrentlyPlayingToLCD() - Empty track. Draw Waiting for song..."); 
            _pUI->setBackground(TFTColor::SC_DJX_BG, true);
            _pUI->drawTextToLCD("Music is unavailable",110);
            _pUI->drawTextToLCD("Waiting for song...",150);
            _isWaitingMessageShowing = true;
        }
        return _isWaitingMessageShowing;
    }
    _isWaitingMessageShowing = false;    

    return _isWaitingMessageShowing;
}

/*
** ===================================================================
** handle_UUM_IDLE()
** ===================================================================
*/
void UIView::handle_UM_IDLE(SCUIMessage *pMessage)
{

}