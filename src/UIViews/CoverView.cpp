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

#include "CoverView.h"
#include "SCLogger.h"
#include "logTags.h"
#include "ThingPulse/util.h"  
#include "SpotifyArtMgr.h"
#include "Renderers/BackButtonRenderer.h"

/*
** ===================================================================
** Constructor
**    Initializes the CoverView with the provided DisplayUI instance.
**
** Parameters:
**    pUI - Pointer to the DisplayUI instance.
** ===================================================================
*/
CoverView::CoverView(DisplayUI *pUI)
    : UIView(pUI) // Pass the DisplayUI pointer to the base class constructor
{
    // Initialization specific to CoverView, if any
}

void CoverView::initializeUIElements() 
{

    std::shared_ptr<IUIRenderer> backButtonRenderer = std::make_shared<BackButtonRenderer>(_pUI);
    // Make the touch rectangle 10 pixels larger on all sides.  The renderer will take off the 10 pixels when rendering
    // Doing to make a bigger, more forgiving touch target.
    const int MARGIN = 10;
    _pReturnViewElement      = std::make_unique<UIElement>(  10 - MARGIN, 270 - MARGIN, 70 + (MARGIN * 2), 40 + (MARGIN * 2), backButtonRenderer);  

    // initialize other elements not set above
    UIView::initializeUIElements();

}

/*
** ===================================================================
** drawUI()
**    Renders the UI for the default mode.
** ===================================================================
*/
void CoverView::drawUI()
{
    SpotifyPlayer  *pSP         = &SpotifyPlayer::getInstance();
    bool            isNewTrack  = pSP->isNewTrackReady();
    PlayingMetadata playing     = pSP->getCurrentlyPlayingMetadata();

    // Render back button
    _pReturnViewElement->render(false);

    const int trackY  = 110; 
    const int albumY  = 150; 
    const int artistY = 190; 

    static bool isWaitingShowing = false;
    if (pSP->isMusicAvailable() == false)
    {
        if (!isWaitingShowing)
        {
            spLogI(LOGTAG_GUI, "drawCurrentlyPlayingToLCD() - Empty track. Draw Waiting for song..."); 
            _pUI->setBackground(TFTColor::SC_DJX_BG, true);
            _pUI->drawTextToLCD("Music is unavailable",trackY);
            _pUI->drawTextToLCD("Waiting for song...",albumY);
            isWaitingShowing = true;
        }
        return;
    }
    isWaitingShowing = false;

    // If this isn't a new song and the UI
    // isn't dirty, nothing to do so leave.
    if ((isNewTrack        == false)
    &&  (_pUI->isUIDirty() == false))
    {
        spLogI(LOGTAG_GUI, "Song has not changed and the UI is not dirty. Leaving drawUI() method.");
        return;
    }

    //start repaintCoverArt();
    String filePath = SP_NO_COVER_JPG_FILENAME;

    const int TARGET_IMAGE_SIZE  = 300;    

    for (int i = 0; i < playing.numImages; i++)
    {
        if ((playing.albumImages[i].height == TARGET_IMAGE_SIZE)
        &&  (playing.albumImages[i].width  == TARGET_IMAGE_SIZE))
        {
            filePath = SpotifyArtMgr::getInstance()->getLocalFileName(playing.albumImages[i].url);
            break;
        }
    }   
    TFTColor c = _pUI->calculateAverageColor(filePath.c_str());
    _pUI->setBackground(c, false);
    _pUI->clearScreenKeepArt();      // Clear any left over text    
    _pUI->drawTextToLCD("Uno",  200, 20, true);
    _pUI->drawTextToLCD("Dos",  230, 14, true);
    _pUI->drawTextToLCD("Tres", 254, 14, true);          

    _pUI->setJpgScaleToSmall(false);
    _pUI->drawAlbumArt(90, 10, filePath);
  
    _pUI->markUIDirty(false);


}