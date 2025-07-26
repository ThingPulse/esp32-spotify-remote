/*-------------------------------------------------------------------------------------------------
**
** Point.h
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

#pragma once

#include <iostream>
#include <cmath> // For std::sqrt and std::pow

class Point {
public:
    int x; // X-coordinate
    int y; // Y-coordinate

    // Constructor
    Point(int x = 0, int y = 0);

    // Method to display the point
    void display() const;

    // Method to calculate the distance to another point
    double distanceTo(const Point& other) const;
};