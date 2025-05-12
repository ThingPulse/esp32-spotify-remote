/*-------------------------------------------------------------------------------------------------
**
** PreviousTrackButtonRenderer.h
**
**    Renders the "Previous Track" button in the Spotify controller UI. Draws a back-skip
**    icon and adjusts visual state when pressed, using the DisplayUI drawing helpers.
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
** PreviousTrackButtonRenderer
**
** Handles rendering of the "Previous Track" button in the UI.
**
** Responsibilities:
**  - Draws the button on the screen.
**  - Adjusts appearance based on its pressed state.
** ===================================================================
*/
class PreviousTrackButtonRenderer : public ButtonRenderer
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