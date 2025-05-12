/*-------------------------------------------------------------------------------------------------
**
** BackButtonRenderer.cpp
**
**    Renders a "Back" or "Previous Track" UI button for the Spotify controller interface.
**    Changes its appearance when pressed and draws a corresponding back arrow icon.
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
** BackButtonRenderer.h
**
** Handles rendering of the "Previous Track" button in the UI.
**
** Responsibilities:
**  - Draws the button on the screen.
**  - Adjusts appearance based on its pressed state.
** ===================================================================
*/
class BackButtonRenderer : public ButtonRenderer
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