/*-------------------------------------------------------------------------------------------------
**
** DiagnosticsView.h
**
**    Declares the DiagnosticsView class, a UIView subclass that provides a
**    diagnostics interface for the Spotify controller. Displays metadata,
**    system status, and playback diagnostics, primarily for debugging purposes.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-01-21 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include "DiagnosticsView.h"
#include "SCLogger.h"
#include "logTags.h"
#include "ThingPulse/util.h"  
#include "Monitor.h"
#include "SpotifyArtMgr.h"
#include "settings.h"
#include "Renderers/BackButtonRenderer.h"
#include "esp_heap_caps.h"
#include "Vault.h"
#include "SpotifyArtMgr.h"

/*
** ===================================================================
** Constructor
**    Initializes the CoverView with the provided DisplayUI instance.
**
** Parameters:
**    pUI - Pointer to the DisplayUI instance.
** ===================================================================
*/
DiagnosticsView::DiagnosticsView(DisplayUI *pUI)
    : UIView(pUI) // Pass the DisplayUI pointer to the base class constructor
{
    // Initialization specific to CoverView, if any
}

void DiagnosticsView::initializeUIElements() 
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
void DiagnosticsView::drawUI()
{

    SpotifyPlayer  *pSP         = &SpotifyPlayer::getInstance();
    bool            isNewTrack  = pSP->isNewTrackReady(); // copy this before resetting it with getCurrentlyPlayingMetadata()
    PlayingMetadata playing     = pSP->getCurrentlyPlayingMetadata();

    if ((isNewTrack)
    || _pUI->isUIDirty())
    {
        clearAndPaintScreen(playing);
        refreshScreen(playing, true);
    }
    else if (millis() > _requestDueTime)
    {
        refreshScreen(playing, false);
    }

    _pUI->markUIDirty(false);
  
}

/*
** ===================================================================
** clearAndPaintScreen()
**    Clears the screen and repaints the UI with the provided track 
**    metadata. This method is responsible for drawing album details, 
**    compile time, and version information.
**
** Parameters:
**    playing - The metadata of the currently playing track.
**
** Returns:
**    None.
** ===================================================================
*/
void DiagnosticsView::clearAndPaintScreen(PlayingMetadata playing)
{
    const int     fontSize = 18;
    const int32_t xStart   = 240;
    char s[100];

    spLogV(LOGTAG_GUI, "==== ENTERING clearAndPaintScreen ===");

    // Clear screen
    _pUI->clearScreen();

    // **** Back Button *****
    _pReturnViewElement->render(false);

    // **** Album Information *****
    _pUI->cDrawString(truncateString(playing.trackName, 50).c_str(), xStart, rowToY(0), fontSize, TFTColor::Green, TFTColor::Black,".");  
    _pUI->cDrawString(truncateString(playing.albumName, 50).c_str(), xStart, rowToY(1), fontSize, TFTColor::Green, TFTColor::Black,".");  
    _pUI->cDrawString(truncateString(playing.getArtistsList(800).c_str(), 50).c_str(), xStart, rowToY(2), fontSize, TFTColor::Green, TFTColor::Black,".");  

    // **** Track Progress Header *****=
    _pUI->lDrawString("progressMs / durationMs :: lastRefreshMs (Pct)", 10, rowToY(4), fontSize, TFTColor::DarkGreen, TFTColor::Black,"");

    // **** Compile / Version / Config Information *****
    char buffer[40];
    snprintf(buffer, sizeof(buffer), "Config: %c %c %d",
             toString(Vault::getInstance().getCredentialSource())[0],
             toString(Vault::getInstance().getPrivacyLevel())[0],
             SpotifyArtMgr::getInstance()->getMaxCacheSize());    
    std::string configInfo = std::string(buffer);

    std::string versionStr = std::string("Version: ") + VERSION + " - " + configInfo;
    _pUI->lDrawString(versionStr.c_str(), 90, rowToY(12), fontSize, TFTColor::DarkGrey, TFTColor::Black,"#### ################## ####");
    versionStr = std::string("Compile: ") + COMPILE_TIME; 
    _pUI->lDrawString(versionStr.c_str(), 90, rowToY(13), fontSize, TFTColor::DarkGrey, TFTColor::Black,"#### ################## ####");
    
}

/*
** ===================================================================
** refreshScreen()
**    Updates the diagnostics screen with the latest track metadata 
**    and system statistics. This method ensures only changed values 
**    are redrawn to optimize performance and minimize flicker.
**
** Parameters:
**    playing         - The metadata of the currently playing track.
**    isForcedRefresh - If true, forces a full repaint regardless 
**                      of whether values have changed.
**
** Returns:
**    None.
** ===================================================================
*/
void DiagnosticsView::refreshScreen(PlayingMetadata playing, bool isForcedRefresh)
{
    const int       fontSize = 18;
    char            s[100];
    std::string     label;    
    std::string     label2;  
    MonitorStats    stats;
    TFTColor        newColor  = TFTColor::Purple;

    // **** Remaining Track Information *****

    static std::string trackInfoLine = "";
    if ((playing.isPlaying) || isForcedRefresh)
    {
        label    = "Y";
        newColor = TFTColor::DarkGreen;
    }
    else
    {
        label    = "N";
        newColor = TFTColor::Red;        
    }
    switch (playing.currentlyPlayingType)
    {
        case PlayingMetadata::PlayingType::track:
            label2 = "track";
            break;
        case PlayingMetadata::PlayingType::episode:
            label2 = "episode";
            break;
        default:
            label2 = "other";
            break;
    }  

    snprintf(s, sizeof(s), "# Artist: %u # Images: %u isPlaying: %s type: %s", playing.numArtists, playing.numImages, label.c_str(), label2.c_str());
    if ((trackInfoLine != s) || isForcedRefresh)
    {
        _pUI->lDrawString(s, 10, rowToY(3), fontSize, newColor, TFTColor::Black,""); 
        trackInfoLine = s;
    }

    static std::string playingTypeLine = ""; 
    uint8_t percentPlayed  = (playing.durationMs > 0) ? (playing.progressMs * 100) / playing.durationMs : 0;
    percentPlayed          = (percentPlayed > 100) ? 100 : percentPlayed;   
    snprintf(s, sizeof(s), "%u / %u :: %u (%u%%)", playing.progressMs, playing.durationMs, playing.lastRefreshMs, percentPlayed);   
    if ((playingTypeLine != s) || isForcedRefresh)
    {
        _pUI->lDrawString(s, 10, rowToY(5), fontSize, TFTColor::Cyan, TFTColor::Black,"lastRefreshMs: ##### playingType: episode");
        playingTypeLine = s;
    }

    // **** Metrics on Getting Song Information  *****    
    static std::string statsLine200 = "";
    stats = Monitor::getStats(MONITOR_ID_SPOTIFY_GET_CURRENTLY_PLAYING);
    if (stats.maxMs > 2500)
    {
        newColor = TFTColor::Yellow;
    }
    else
    {
        newColor = TFTColor::DarkGreen;
    }
    if (stats.averageMs > 2500)
    {
        newColor = TFTColor::Red;
    }            
    if (stats.minMs == LLONG_MAX)
    {
        snprintf(s, sizeof(s), "%u: A:%lld M:--- X:%lld C:%d", stats.id, stats.averageMs, stats.maxMs, stats.stopCount);
    }
    else
    {
        snprintf(s, sizeof(s), "%u: A:%lld M:%lld X:%lld C:%d", stats.id, stats.averageMs, stats.minMs, stats.maxMs, stats.stopCount);
    }
    if ((statsLine200 != s) || isForcedRefresh)
    {
        _pUI->lDrawString(s, 10, rowToY(6), fontSize, newColor, TFTColor::Black,"");
        statsLine200 = s;
    }

    // **** Metrics on Fetching Album Art *****    
    static std::string statsLine300 = "";
    stats = Monitor::getStats(MONITOR_ID_SPOTIFY_IMAGE_HTTP_GET);

    if (stats.minMs == LLONG_MAX)
    {
        snprintf(s, sizeof(s), "%u: A:%lld M:--- X:%lld C:%d", stats.id, stats.averageMs, stats.maxMs, stats.stopCount);
    }
    else
    {
        snprintf(s, sizeof(s), "%u: A:%lld M:%lld X:%lld C:%d", stats.id, stats.averageMs, stats.minMs, stats.maxMs, stats.stopCount);
    }
    if ((statsLine300 != s) || isForcedRefresh)
    {
        if (stats.maxMs > 2500)
        {
            newColor = TFTColor::Yellow;
        }
        else
        {
            newColor = TFTColor::DarkGreen;
        }
        if (stats.averageMs > 2500)
        {
            newColor = TFTColor::Red;
        }        
        _pUI->lDrawString(s, 10, rowToY(7), fontSize, newColor, TFTColor::Black,"");
        statsLine300 = s;
    } 
  
    // **** Art Cache Hits Metrics *****    
    static std::string artCacheLine = "";
    ArtMgrStats artMgrStats = SpotifyArtMgr::getInstance()->getStats();
    snprintf(s, sizeof(s), "AHR: %lld LHS: %lld CHS: %lld Hits: %lld TF: %lld", 
                            artMgrStats.averageHitRate,
                            artMgrStats.longestHitStreak,
                            artMgrStats.currentHitStreak,
                            artMgrStats.cacheHits,
                            artMgrStats.totalFetches);
    if ((artCacheLine != s) || isForcedRefresh)
    {
        _pUI->lDrawString(s, 10, rowToY(8), fontSize, TFTColor::DarkGreen, TFTColor::Black,"Heap: #######   Low: #######");
        artCacheLine = s;
    }

    // **** Heap Stats *****
    static std::string heapLine  = "";
    uint32_t           heapSize  = Monitor::getFreeHeap();
    Monitor::HeapStats heapStats = Monitor::getHeapStats();

    // Approximate percentage remaining based on max we've seen
    float remainingPercentage = (float)heapSize / (float)heapStats.maxHeap * 100.0f;

 
    snprintf(s, sizeof(s), "Heap: %u   Low: %u Hi: %u (%.0f%%)", heapSize, heapStats.minHeap, heapStats.maxHeap, remainingPercentage);    
    if ((heapLine != s) || isForcedRefresh)
    {
        if (heapSize < HEAP_LOW_THRESHOLD)
        {
            newColor = TFTColor::Red;
        }
        else if (heapSize < HEAP_WATCH_SIZE_BASE)
        {
            newColor = TFTColor::Yellow;
        }
        else
        {
            if (heapStats.minHeap < HEAP_LOW_THRESHOLD)
            {
                newColor = TFTColor::Pink;
            }
            else if ((heapStats.minHeap < HEAP_WATCH_SIZE_BASE))
            {
                newColor = TFTColor::SC_LowHeap;
            }
            else
            {
                newColor = TFTColor::Cyan;
            }
            
        }           
        _pUI->lDrawString(s, 10, rowToY(9), fontSize, newColor, TFTColor::Black,"Heap: #######   Low: #######   High: #######");
        heapLine = s;
    }

    // **** Message Queue Stats *****    
    static std::string  mqLine = "";
    uint32_t            depth = Monitor::getMaxQueueDepth();
    snprintf(s, sizeof(s), "MQD: %u%", depth);
    
    if ((mqLine != s) || isForcedRefresh)
    {
        if (depth > 5)
        {
            if (depth >= 7)
            {
                newColor = TFTColor::Red;
            }
            else
            {
                newColor = TFTColor::Yellow;
            }
        }
        else
        {
            newColor = TFTColor::DarkGreen;
        }           
        _pUI->lDrawString(s, 10, rowToY(10), fontSize, newColor, TFTColor::Black,"MQD: ##");   
        mqLine = s;
    }

    static std::string queueDelayLine = "";
    stats = Monitor::getStats(MONITOR_ID_SCUI_QUEUE_DELAY);
 
    snprintf(s, sizeof(s), "A: %lld M: %lld X: %lld", stats.averageMs, stats.minMs, stats.maxMs);
    if ((queueDelayLine != s) || isForcedRefresh)
    {
        if (stats.maxMs > 2000)
        {
            newColor = TFTColor::Yellow;
        }
        else
        {
            newColor = TFTColor::DarkGreen;
        }          
        _pUI->lDrawString(s, 100, rowToY(10), fontSize, newColor, TFTColor::Black,"Avg(ms): ##### Max(ms) #####");
        queueDelayLine = s;
    }

    // **** Uptime Stats *****
    static std::string uptimeLine = "";
    snprintf(s, sizeof(s), "%s", Monitor::getFormattedUptime("Up %lu days, %lu hrs, %lu mins, %lu secs").c_str());
    if ((uptimeLine != s) || isForcedRefresh)
    {
        _pUI->lDrawString(s, 90, rowToY(11), fontSize, TFTColor::DarkGreen, TFTColor::Black,"#### ################## ####");
        uptimeLine = s;
    }
}

/*
** ===================================================================
** rowToY()
**    Calculates the Y pixel position based on the given row index.
**    The calculation considers the default font size and spacing rules.
**
** Parameters:
**    row - The row index for which to calculate the Y position.
**
** Returns:
**    The Y pixel coordinate corresponding to the given row index.
** ===================================================================
*/
constexpr int DiagnosticsView::rowToY(int row)
{
    const int fontSize   = 18;
    const int rowSpacing = 22;
    const int margin     = 10;
    
    return margin + row * rowSpacing;

}

/*
** ===================================================================
** enteringView()
** ===================================================================
*/    
void DiagnosticsView::enteringView()
{
    // spLogI(LOGTAG_GENERAL, "*** ENTERING  DiagnosticsView::enteringView() ===");
    _pUI->setSplitBackground(false);

    _pUI->setBackground(TFTColor::Black, false);

    // force repaint to get latest album details and to get background updates
    _pUI->markUIDirty(true);

    drawUI();

    // spLogI(LOGTAG_GENERAL, "*** EXITING  DiagnosticsView::enteringView() ===");
}

/*
** ===================================================================
** handle_UUM_IDLE()
** ===================================================================
*/
void DiagnosticsView::handle_UM_IDLE(SCUIMessage *pMessage)
{
    const unsigned long delayBetweenRequests = 3000;

    // Every period refresh the display
    if (millis() > _requestDueTime)
    {
        drawUI();
        _requestDueTime = millis() + delayBetweenRequests;
    } 

}

