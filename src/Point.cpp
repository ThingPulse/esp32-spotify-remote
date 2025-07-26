/*-------------------------------------------------------------------------------------------------
**
** Point.cpp
**
**    Defines a simple 2D Point class with integer coordinates.
**    Provides methods for displaying the point and computing the
**    Euclidean distance to another point.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2024-12-31 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include "Point.h"

// Constructor implementation
Point::Point(int x, int y) : x(x), y(y) {}

// Display the point
void Point::display() const {
    std::cout << "(" << x << ", " << y << ")" << std::endl;
}

// Calculate the distance to another point
double Point::distanceTo(const Point& other) const {
    return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
}