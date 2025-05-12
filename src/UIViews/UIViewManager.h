/*-------------------------------------------------------------------------------------------------
**
** UIViewManager.h
**
**    Manages display modes for the Spotify controller UI. Implements a singleton
**    that lazily initializes and switches between different UIView screens using
**    a stack to track navigation history and enable return functionality.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-01-26 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#pragma once

#include "UIView.h"
#include <array>
#include <memory>
#include <stack>

/*
** ===================================================================
** UIViewManager
**
** Manages the different display modes for the UI. This class is
** implemented as a singleton and lazily initializes its display
** modes when `setDisplayUI()` is called with a valid DisplayUI
** instance.
**
** Responsibilities:
**  - Manages and switches between different display modes.
**  - Lazily initializes modes based on the provided DisplayUI.
**  - Supports navigation history with a stack for returning to
**    the previous view.
** ===================================================================
*/
class UIViewManager
{
public:
    // Enum representing supported display modes
    enum class ViewID : uint8_t
    {
        Home,        // Show Song, Album, and Artist(s) over cover art
        Cover,       // Show only the cover art
        Diagnostics, // System Stats
        Clock        // Clock View
    };

    // Returns the singleton instance of UIViewManager
    static UIViewManager &getInstance();

    // Sets the DisplayUI instance to be used by the display modes and
    // initializes the display modes if they haven’t been initialized.
    void setDisplayUI(DisplayUI *pUI);

    // Retrieves the current display mode
    UIView *getActiveView() const;

    // Sets the current display mode
    void setViewID(ViewID id);

    // Advances to the next display mode
    void advanceView();

    // Sets the active view to the given view ID, pushing the current
    // view onto the stack before switching.
    void gotoView(ViewID id);

    // Returns to the previous view if possible. Returns false if
    // already at the root view.
    bool exitView();

private:
    // Private constructor to enforce singleton pattern
    UIViewManager();

    // Initializes all display modes
    void initializeViews(DisplayUI *pUI);

    // Number of display modes (static constexpr to avoid compile errors)
    static constexpr uint8_t NUM_DISPLAY_MODES = static_cast<uint8_t>(ViewID::Clock) + 1;

    std::array<std::unique_ptr<UIView>, NUM_DISPLAY_MODES> _views; // List of all display modes
    UIView *             _pCurrentView   = nullptr;                // Active display mode
    ViewID               _currentViewID  = ViewID::Home;           // Tracks current ViewID
    bool                 _isInitialized  = false;                  // Initialization flag
    std::stack<ViewID>   _viewStack;                               // Stack for tracking navigation history
};