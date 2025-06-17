/*-------------------------------------------------------------------------------------------------
**
** DisplayUI.h
**
**    Provides methods for drawing UI elements including album art, buttons,
**    progress indicators, and text, using the TFT_eSPI and OpenFontRender libraries.
**
**    Originally based on https://github.com/Bodmer/OpenWeather/blob/main/examples/TFT_eSPI_OpenWeather_LittleFS/GfxUi.h
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2024-12-26 - Electric Diversions - Renamed from GfxUI.h.h to tpGfxUi.h
**    2024-12-27 - Electric Diversions - Renamed to DisplayUI and promoted to base class
** ------------------------------------------------------------------------------------------------
*/

#pragma once

#include <FS.h>
#include <OpenFontRender.h>
#include <TFT_eSPI.h>

// JPEG decoder library
#include <TJpg_Decoder.h>

#include <string>

#include "Point.h"


// Maximum of 85 for BUFFPIXEL as 3 x this value is stored in an 8 bit variable!
// 32 is an efficient size for LittleFS due to SPI hardware pipeline buffer size
// A larger value of 80 is better for SD cards
#define BUFFPIXEL 32

// Size of cover art square side.
#define COVER_ART_SIZE 300

/*
** ===================================================================
** Predefined colors to use on the UI
** ===================================================================
*/
enum class TFTColor : uint16_t 
{
    Black          = 0x0000, //   0,   0,   0
    Blue           = 0x001F, //   0,   0, 255
    BlueThinkPulse = 0x0336, // The medium blue in the TP logo is 0x0067B0 which converts to 0x0336 in 16bit RGB565
    Brown          = 0x9A60, // 150,  75,   0
    Cyan           = 0x07FF, //   0, 255, 255
    DarkCyan       = 0x03EF, //   0, 128, 128
    DarkGreen      = 0x03E0, //   0, 128,   0
    DarkestGreen   = 0x0280,
    DarkGrey       = 0x7BEF, // 128, 128, 128
    Gold           = 0xFEA0, // 255, 215,   0
    Green          = 0x07E0, //   0, 255,   0
    GreenYellow    = 0xB7E0, // 180, 255,   0
    LightGrey      = 0xD69A, // 211, 211, 211
    LightPink      = 0xFC9F, //
    Magenta        = 0xF81F, // 255,   0, 255
    Maroon         = 0x7800, // 128,   0,   0
    Navy           = 0x000F, //   0,   0, 128
    Olive          = 0x7BE0, // 128, 128,   0
    Orange         = 0xFDA0, // 255, 180,   0
    Pink           = 0xFE19, // 255, 192, 203
    Purple         = 0x780F, // 128,   0, 128
    Red            = 0xF800, // 255,   0,   0
    Silver         = 0xC618, // 192, 192, 192
    SkyBlue        = 0x867D, // 135, 206, 235
    Violet         = 0x915C, // 180,  46, 226
    White          = 0xFFFF, // 255, 255, 255
    Yellow         = 0xFFE0, // 255, 255,   0

    SC_LowHeap           = 0xAB04, // 174,  98,  37  
    SC_DJX_BG            = 0x22D6, //  39,  88, 178
    SC_DJX_FG            = 0x6F11, // 108, 227, 148
    SC_PreviousTrack     = 0xFEA0, // 255, 215,   0
    SC_NextTrack         = 0xB7E0, // 180, 255,   0
    SC_PauseTrack        = 0x780F, // 128,   0, 128
    SC_CacheSave         = 0x867D, // 135, 206, 235
    SC_TouchDown         = 0x867D, // 135, 206, 235
    SC_AlertStatus       = 0xFFE0, // 255, 255,   0
    SC_NetworkInProgress = 0x7BEF, // 128, 128, 128
    SC_NetworkSuccess    = 0x03E0, //   0, 128,   0
    SC_NetworkFailure    = 0xF800  // 255,   0,   0
};

class DisplayUI {
public:
    DisplayUI(TFT_eSPI *tft, OpenFontRender *render, OpenFontRender *clockFont);
    void init(); 

    // Original Thing Pulse methods
    void drawBmp(String filename, uint16_t x, uint16_t y);
    void drawLogo();
    void drawAppInfo();
    void drawProgressBar(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                          uint8_t percentage, TFTColor frameColor,
                          TFTColor barColor);
    // Added 12/2024:
    void drawAlbumArt(int32_t x, int32_t y, String filename);
    void setJpgScaleToSmall(bool isSmall);
    void setJpgScaleToTiny();
    void drawStatusBox(TFTColor statusColor);
    void drawDownloadIndicator(bool bDownloadComplate); 
    void drawProgress(const char *text, int8_t percentage);
    void drawSeparator(uint16_t y);
    void drawButton(int32_t x, int32_t y, bool isPressed); 
    void drawBlankButton(int32_t x, int32_t y, int32_t width, int32_t height, uint8_t margin, TFTColor borderColor, bool isPressed); 
    void drawSkipTrackIcon(int32_t x, int32_t y, int32_t width, int32_t height, bool isReversed);
    void drawPlayTrackIcon(int32_t x, int32_t y, int32_t width, int32_t height);
    void drawPauseTrackIcon(int32_t x, int32_t y, int32_t width, int32_t height);
    void drawBackIcon(int32_t x, int32_t y, int32_t width, int32_t height);

    void        setBackground(TFTColor color, bool bRepaint=true);
    TFTColor    getBackground() const;
    bool        isSplitBackground() const;
    void        setSplitBackground(bool isSplitBackground = false);
    void        clearScreen();
    void        clearScreenKeepArt();
    void        clearScreenHome();
    std::string formatTime(long millis); 

    void drawTextToLCD(const char *text, int posY);
    void drawTextToLCD(const char *text, int posY, int fontSize, bool ignoreText); 
    void drawText(const char *text, int posX, int posY, int fontSize, TFTColor color); 

    void drawString(const char *str,
                    int32_t x,
                    int32_t y,
                    unsigned int fontSize,
                    TFTColor fg,
                    TFTColor bg,
                    Align    alignment,
                    const char  *clearMask);

    void cDrawString(const char *str,
                    int32_t x,
                    int32_t y,
                    unsigned int fontSize,
                    TFTColor fg,
                    TFTColor bg,
                    const char  *clearMask);

    void lDrawString(const char *str,
                    int32_t x,
                    int32_t y,
                    unsigned int fontSize,
                    TFTColor fg,
                    TFTColor bg,
                    const char  *clearMask);

    void rDrawString(const char *str,
                    int32_t x,
                    int32_t y,
                    unsigned int fontSize,
                    TFTColor fg,
                    TFTColor bg,
                    const char  *clearMask);

    void drawClockTime(bool isUSFormat, bool isForceRepaint);

    void showTouchDown(TFTColor color = TFTColor::SC_TouchDown);
    void showTouchUp(); 

    bool pushImageToTft(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);

    // display properties
    int16_t getCenterWidth();
    int16_t getCenterHeight();
    int16_t getWidth();
    int16_t getHeight();
    Point   getPreviousActionPoint();
    Point   getNextActionPoint();
     
    bool    isUIDirty();
    void    markUIDirty(bool isDirty);

    JRESULT  drawFsJpg(int32_t x, int32_t y, const char *pFilename, fs::FS &fs);
    TFTColor calculateAverageColor(const char* filename);

    // utility methods
    static void rgbToHsv(uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& v);
    static void hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b);
    

private:
    // Member variables
    TFT_eSPI       *_tft;
    OpenFontRender *_ofr;
    OpenFontRender *_clockFont;
    TFTColor       _bgColor             = TFTColor::BlueThinkPulse;
    bool           _isSplitBackground   = false;
    bool           _isUIDirty           = false;
    bool           _isPainting          = true; 
    SemaphoreHandle_t xSemaphoreDisplay = xSemaphoreCreateMutex();

    // Methods
    uint16_t       read16(fs::File &f);
    uint32_t       read32(fs::File &f);
    void           paintTouch(bool isPress, TFTColor color);
    static bool    jpgCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
    bool           jpgCalculateAverageColorCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap); 
    void           drawClockString(
                            const char *str, 
                            int32_t x, 
                            int32_t y, 
                            unsigned int    fontSize, 
                            TFTColor        fg, 
                            TFTColor        bg);   

};
