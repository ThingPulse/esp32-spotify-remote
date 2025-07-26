/*-------------------------------------------------------------------------------------------------
**
** CoverView.h
**
**    Declares the CoverView class, a UIView subclass that implements the
**    "Art Only" display mode for the Spotify controller. It shows only the
**    album cover art along with minimal track information and a return button.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-01-21 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#pragma once
#include "UIView.h"

/*
** ===================================================================
** Class: CoverView
**
** Purpose:
**    Implements the "Art Only" display mode, showing only the album
**    cover art.
** ===================================================================
*/
class CoverView : public UIView
{
public:
    explicit CoverView(DisplayUI *pUI); // Constructor
    virtual void initializeUIElements() override;

    /*
    ** ===================================================================
    ** drawUI()
    **    Renders the UI for the art-only mode.
    ** ===================================================================
    */
    void drawUI() override;
};