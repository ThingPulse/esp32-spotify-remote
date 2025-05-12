/*-------------------------------------------------------------------------------------------------
**
** PlayPauseButtonRenderer.h
**
**    Renders the play/pause toggle button in the Spotify controller UI.
**    Displays either a play or pause icon based on the playback state
**    retrieved from the SpotifyPlayer singleton.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-02-22 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#pragma once

#include "IUIRenderer.h"
#include "ButtonRenderer.h"

/*
** ===================================================================
** PlayPauseButtonRenderer
**
** Handles rendering of the "Previous Track" button in the UI.
**
** Responsibilities:
**  - Draws the button on the screen.
**  - Adjusts appearance based on its pressed state.
** ===================================================================
*/
class PlayPauseButtonRenderer : public ButtonRenderer
{
public:
    using ButtonRenderer::ButtonRenderer; 

    /*
    ** ===================================================================    
    ** render()
    **    Renders the "Previous Track" button.
    **
    ** Parameters:
    **    element   - Reference to the UI element being rendered.
    **    isPressed - Indicates whether the button is pressed.
    ** ===================================================================
    */
    void render(const UIElement& element, bool isPressed) override;

protected:
    void drawIcon(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color) override;
};