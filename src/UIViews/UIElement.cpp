/*-------------------------------------------------------------------------------------------------
**
** UIElement.cpp
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

#include "UIElement.h"

/*
** Constructor
**    Initializes the UI element with its position and dimensions.
*/
UIElement::UIElement(int16_t x, int16_t y, int16_t width, int16_t height, std::shared_ptr<IUIRenderer> renderer)
    : _x(x),
      _y(y), 
      _width(width), 
      _height(height),
      _renderer(std::move(renderer))
{
}

/*
** Constructor (touch zone only, no renderer)
**    Initializes the UI element as a non-rendering touch zone.
*/
UIElement::UIElement(int16_t x, int16_t y, int16_t width, int16_t height)
    : _x(x),
      _y(y), 
      _width(width), 
      _height(height), 
      _renderer(nullptr)
{
}

/*
** isPressed()
**    Determines if a touch point is within the bounds of this UI element.
**
** Returns:
**    True if the point lies within the rectangle, otherwise false.
*/
bool UIElement::isPressed(const TS_Point& point) const
{
    return (point.x >= _x 
         && point.x <= (_x + _width) 
         && point.y >= _y 
         && point.y <= (_y + _height));
}

/*
** render()
**    Calls the assigned renderer to render this UIElement.
**
** Parameters:
**    isPressed - Indicates whether the element is in a pressed state.
**
** Notes:
**    If a renderer is assigned, it will be invoked to draw the element.
**    Otherwise, this function does nothing.
*/
void UIElement::render(bool isPressed) const
{
    if (_renderer)
    {
        _renderer->render(*this, isPressed);
    }
}
