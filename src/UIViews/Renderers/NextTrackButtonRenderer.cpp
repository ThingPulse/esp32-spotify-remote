/*-------------------------------------------------------------------------------------------------
**
** NextTrackButtonRenderer.cpp
**
**    Renders the "Next Track" button in the Spotify controller UI. Draws a forward-skip
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

#include "NextTrackButtonRenderer.h"
#include "../UIElement.h"

/*
** ===================================================================
** render()
**    Renders the "Next Track" button.
**
** Parameters:
**    element   - Reference to the UI element being rendered.
**    isPressed - Indicates whether the button is pressed.
**
** Notes:
**    - Uses TFT_eSPI to draw a "Previous Track" icon.
**    - Changes appearance when pressed.
** ===================================================================
*/
void NextTrackButtonRenderer::render(const UIElement& element, bool isPressed)
{
    _pUI->drawBlankButton(element.getX(), element.getY(), element.getWidth(), element.getHeight(), 2, TFTColor::White, isPressed);
    _pUI->drawSkipTrackIcon(element.getX(), element.getY(), element.getWidth(), element.getHeight(), false);
}

/*
** ===================================================================
** drawIcon()
**    Abstract method for rendering a button's unique icon.
**
** Parameters:
**    x, y      - Top-left coordinates for the icon.
**    width, height - Dimensions for the icon.
**    color     - Color to use for the icon.
** ===================================================================
*/
void NextTrackButtonRenderer::drawIcon(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
    _pUI->drawSkipTrackIcon(x, y, width, height, false);
}
