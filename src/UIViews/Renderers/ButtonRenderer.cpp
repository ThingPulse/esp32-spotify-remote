/*-------------------------------------------------------------------------------------------------
**
** ButtonRenderer.cpp
**
**    Base renderer for generic UI buttons. Handles button background and border
**    drawing based on press state, and delegates icon rendering to derived classes.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-02-22 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include "ButtonRenderer.h"
#include "../UIElement.h"
#include "TFT_eSPI.h"

extern TFT_eSPI tft; // Assume an external TFT instance is used

/*
** ===================================================================
** Constructor (default)
**    Initializes the ButtonRenderer without a UI reference.
** ===================================================================
*/
ButtonRenderer::ButtonRenderer() : _pUI(nullptr)
{
}

/*
** ===================================================================
** Constructor (with DisplayUI)
**    Initializes the ButtonRenderer with a reference to DisplayUI.
**
** Parameters:
**    pUI - Pointer to the DisplayUI instance.
** ===================================================================
*/
ButtonRenderer::ButtonRenderer(DisplayUI* pUI) : _pUI(pUI)
{
}

/*
** ===================================================================
** render()
**    Renders a button with a customizable icon.
**
** Parameters:
**    element   - Reference to the UI element being rendered.
**    isPressed - Indicates whether the button is pressed.
**
** Notes:
**    - Draws a button with a rounded rectangle.
**    - Calls `drawIcon()` to render the specific button symbol.
** ===================================================================
*/
void ButtonRenderer::render(const UIElement& element, bool isPressed)
{
    int16_t x = element.getX();
    int16_t y = element.getY();
    int16_t width = element.getWidth();
    int16_t height = element.getHeight();

    // Define colors based on button state
    uint16_t bgColor = isPressed ? TFT_DARKGREY : TFT_BLACK;
    uint16_t borderColor = isPressed ? TFT_WHITE : TFT_LIGHTGREY;

    // Draw button background
    tft.fillRoundRect(x, y, width, height, 5, bgColor);

    // Draw button border
    tft.drawRoundRect(x, y, width, height, 5, borderColor);

    // Draw the icon (specific to the derived class)
    drawIcon(x + width / 4, y + height / 4, width / 2, height / 2, borderColor);
}