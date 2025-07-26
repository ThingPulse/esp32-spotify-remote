/*-------------------------------------------------------------------------------------------------
**
** IUIRenderer.h
**
**    Interface for rendering UI elements on the display. Classes implementing
**    this interface must define a `render` method to draw a UIElement, either
**    in its normal or pressed state.
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

/*
** ===================================================================
** Interface: IUIRenderer
**
** Purpose:
**    Provides an interface for rendering UI elements.
** ===================================================================
*/
class UIElement; // Forward declaration

class IUIRenderer
{
public:
    /*
    ** ===================================================================
    ** Destructor
    **    Ensures proper cleanup of derived classes.
    ** ===================================================================
    */
    virtual ~IUIRenderer() = default;

    /*
    ** ===================================================================
    ** Method: render
    **    Renders a UIElement.
    **
    ** Parameters:
    **    element   - Reference to the UIElement to render.
    **    isPressed - Indicates whether the element is in a pressed state.
    ** ===================================================================
    */
    virtual void render(const UIElement& element, bool isPressed) = 0;
};

