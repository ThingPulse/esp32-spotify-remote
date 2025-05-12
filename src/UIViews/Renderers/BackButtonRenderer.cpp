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


#include "BackButtonRenderer.h"
#include "../UIElement.h"

/*
** ===================================================================
** render()
**    Renders the "Previous Track" button.
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
void BackButtonRenderer::render(const UIElement& element, bool isPressed)
{
    // MARGIN is used here and in the including view to provide a larger touch target
    const int MARGIN = 10;
    _pUI->drawBlankButton(element.getX() + MARGIN, element.getY() + MARGIN, element.getWidth() - (MARGIN * 2), element.getHeight() - (MARGIN * 2), 2, TFTColor::SC_NetworkSuccess, isPressed);
    _pUI->drawBackIcon(element.getX() + MARGIN, element.getY() + MARGIN, element.getWidth() - (MARGIN * 2), element.getHeight() - (MARGIN * 2));
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
void BackButtonRenderer::drawIcon(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
    _pUI->drawSkipTrackIcon(x, y, width, height, false);
}
