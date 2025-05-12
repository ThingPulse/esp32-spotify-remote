/*-------------------------------------------------------------------------------------------------
**
** ButtonRenderer.h
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

#pragma once

#include "IUIRenderer.h"
#include "../../DisplayUI.h"

/*
** ===================================================================
** ButtonRenderer
**    Base class for all button renderers.
**
** Responsibilities:
**    - Provides common rendering logic for UI buttons.
**    - Handles default button drawing, including borders and backgrounds.
**    - Allows derived classes to customize icons.
** ===================================================================
*/
class ButtonRenderer : public IUIRenderer
{
public:
    /*
    ** ===================================================================
    ** Constructor (default)
    **    Initializes the ButtonRenderer without a UI reference.
    ** ===================================================================
    */
    ButtonRenderer();

    /*
    ** ===================================================================
    ** Constructor (with DisplayUI)
    **    Initializes the ButtonRenderer with a reference to DisplayUI.
    **
    ** Parameters:
    **    pUI - Pointer to the DisplayUI instance.
    ** ===================================================================
    */
    explicit ButtonRenderer(DisplayUI* pUI);

    /*
    ** ===================================================================
    ** Destructor
    **    Cleans up any allocated resources.
    ** ===================================================================
    */
    virtual ~ButtonRenderer() = default;

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
    void render(const UIElement& element, bool isPressed) override;

protected:
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
    virtual void drawIcon(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color) = 0;

    /*
    ** ===================================================================
    ** Protected Members
    **    _pUI - Pointer to the DisplayUI instance.
    ** ===================================================================
    */
    DisplayUI* _pUI;
};