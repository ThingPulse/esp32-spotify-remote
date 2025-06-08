/*-------------------------------------------------------------------------------------------------
**
** PlayPauseButtonRenderer.cpp
**
**    Renders the play/pause toggle button in the Spotify controller UI.
**    Displays either a play or pause icon based on the playback state
**    retrieved from the SpotifyPlayer singleton.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-02-22 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include "PlayPauseButtonRenderer.h"
#include "SpotifyPlayer.h"
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
void PlayPauseButtonRenderer::render(const UIElement& element, bool isPressed)
{
    _pUI->drawBlankButton(element.getX(), element.getY(), element.getWidth(), element.getHeight(), 2, TFTColor::White, isPressed);

    if (SpotifyPlayer::getInstance().getCurrentlyPlayingMetadata().isPlaying)
    {
        _pUI->drawPauseTrackIcon(element.getX(), element.getY(), element.getWidth(), element.getHeight());
    }
    else
    {
        _pUI->drawPlayTrackIcon(element.getX(), element.getY(), element.getWidth(), element.getHeight());
    }
    
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
void PlayPauseButtonRenderer::drawIcon(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
}
