/*-------------------------------------------------------------------------------------------------
**
** UIElement.h
**
**    Represents a rectangular UI element on the display, providing basic
**    hit testing and rendering capabilities. Can be used as a pressable
**    touch zone or paired with a renderer to draw visual elements.
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
#include <memory>
#include "FT6236TouchController/FT6236.h"
#include "Renderers/IUIRenderer.h"

/*
** ===================================================================
** UIElement
**    Represents a rectangular UI element on the display.
** ===================================================================
*/
class UIElement
{
public:
    // Constructor to define the UI element's position and dimensions
    UIElement(int16_t x, int16_t y, int16_t width, int16_t height, std::shared_ptr<IUIRenderer> renderer);
    UIElement(int16_t x, int16_t y, int16_t width, int16_t height);

    /*
    ** ===================================================================    
    ** isPressed()
    **    Checks if a given point is within the bounds of this UI element.
    **
    ** Parameters:
    **    point - The touch point to check.
    **
    ** Returns:
    **    True if the point is within the bounds, otherwise false.
    ** ===================================================================    
    */
    bool isPressed(const TS_Point& point) const;

    /*
    ** ===================================================================    
    ** render()
    **    Calls the assigned renderer to render this UIElement.
    **
    ** Parameters:
    **    isPressed - Indicates whether the element is in a pressed state.
    ** ===================================================================
    */
   void render(bool isPressed) const;    

    /*
    ** ===================================================================      
    ** Accessors
    **    Provides read-only access to UIElement properties.
    ** ===================================================================      
    */
   int16_t getX()      const { return _x; }
   int16_t getY()      const { return _y; }
   int16_t getWidth()  const { return _width; }
   int16_t getHeight() const { return _height; }   

private:
    int16_t                      _x;        // X-coordinate of the top-left corner
    int16_t                      _y;        // Y-coordinate of the top-left corner
    int16_t                      _width;    // Width of the element
    int16_t                      _height;   // Height of the element
    std::shared_ptr<IUIRenderer> _renderer; // Renderer instance for drawing the element.
};