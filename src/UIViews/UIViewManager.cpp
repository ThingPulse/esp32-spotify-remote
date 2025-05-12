/*-------------------------------------------------------------------------------------------------
**
** UIViewManager.cpp
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


#include "UIViewManager.h"
#include "CoverView.h"
#include "HomeView.h"
#include "DiagnosticsView.h"
#include "ClockView.h"

/*
** ===================================================================
** UIViewManager::UIViewManager()
**    Constructor initializes the display manager.
** ===================================================================
*/
UIViewManager::UIViewManager()
{
}

/*
** ===================================================================
** getInstance()
**    Returns the singleton instance of UIViewManager.
** ===================================================================
*/
UIViewManager& UIViewManager::getInstance()
{
    static UIViewManager instance;
    return instance;
}

/*
** ===================================================================
** setDisplayUI()
**    Sets the DisplayUI instance to be used by the display modes and
**    initializes the display modes if they haven't been initialized.
**
** Parameters:
**    pUI - Pointer to the DisplayUI instance.
** ===================================================================
*/
void UIViewManager::setDisplayUI(DisplayUI *pUI)
{
    if (!_isInitialized 
    &&  pUI != nullptr)
    {
        initializeViews(pUI);
        _isInitialized = true;
    }
}

/*
** ===================================================================
** getActiveView()
**    Returns the current display mode.
**
** Returns:
**    The currently active display mode as a UIView pointer.
** ===================================================================
*/
UIView* UIViewManager::getActiveView() const
{
    return _pCurrentView;
}

/*
** ===================================================================
** setViewID()
**    Changes the active display mode.
**
** Parameters:
**    id - ViewID representing the new display mode.
** ===================================================================
*/
void UIViewManager::setViewID(ViewID id)
{
    if (_pCurrentView)
    {
        _pCurrentView->leavingView();
    }

    _pCurrentView  = _views[static_cast<size_t>(id)].get();
    _currentViewID = id;

    if (_pCurrentView)
    {
        _pCurrentView->enteringView();
    }
}

/*
** ===================================================================
** advanceView()
**    Advances to the next display mode in sequence.
** ===================================================================
*/
void UIViewManager::advanceView()
{
    if (!_isInitialized)
    {
        return; // Do nothing if not initialized
    }

    // Determine the next view in the sequence
    ViewID nextView = static_cast<ViewID>((static_cast<uint8_t>(_currentViewID) + 1) % NUM_DISPLAY_MODES);

    // Switch to the next view
    setViewID(nextView);
}

/*
** ===================================================================
** gotoView()
**    Sets the active view to the given view ID, pushing the current
**    view onto the stack before switching.
**
** Parameters:
**    id - ViewID representing the new view to switch to.
** ===================================================================
*/
void UIViewManager::gotoView(ViewID id)
{
    if (_currentViewID == id)
    {
        return; // Already in the requested view
    }

    if (static_cast<uint8_t>(id) < NUM_DISPLAY_MODES)
    {
        // Push current view onto stack
        _viewStack.push(_currentViewID);

        // Set new active view
        setViewID(id);
    }
}

/*
** ===================================================================
** exitView()
**    Returns to the previous view if possible. If the stack is empty,
**    no action is taken and false is returned.
**
** Returns:
**    true  - If successfully switched to a previous view.
**    false - If already at the root view.
** ===================================================================
*/
bool UIViewManager::exitView()
{
    if (!_viewStack.empty())
    {
        // Get the last view from the stack
        ViewID previousView = _viewStack.top();
        _viewStack.pop();

        // Set the active view to the previous view
        setViewID(previousView);
        return true;
    }

    return false; // No previous view to return to
}

/*
** ===================================================================
** initializeViews()
**    Initializes the display modes using the provided DisplayUI
**    instance.
**
** Parameters:
**    pUI - Pointer to the DisplayUI instance.
** ===================================================================
*/
void UIViewManager::initializeViews(DisplayUI *pUI)
{
    _views = {
        std::make_unique<HomeView>(pUI),
        std::make_unique<CoverView>(pUI),
        std::make_unique<DiagnosticsView>(pUI),
        std::make_unique<ClockView>(pUI)
    };

    // Call initialize() on each view to ensure UI elements are properly set up
    for (auto& view : _views) {
        view->initialize();
    }    

    // Set the initial mode to Home
    _currentViewID = ViewID::Home;
    _pCurrentView  = _views[static_cast<size_t>(_currentViewID)].get();
    _pCurrentView->enteringView();
}