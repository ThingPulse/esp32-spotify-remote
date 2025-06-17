/*-------------------------------------------------------------------
**
** DisplayUI.cpp
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
**
*/

/* 
** ------------------------------------------------------------------------------------------------
**
**    NOTE: DMA (Direct Memory Access) is *not* enabled for ILI9488.
**
**    The ILI9488 display controller requires 24-bit (RGB888) color 
**    over SPI, while the ESP32's SPI DMA engine only supports 8-bit 
**    and 16-bit transfers. Because of this, the TFT_eSPI library 
**    disables DMA support for ILI9488 by default.
**
**    Enabling DMA would require additional processing to convert 
**    16-bit (RGB565) color data into the 24-bit format, eliminating 
**    any potential performance benefits. It could also introduce 
**    graphical artifacts or instability.
**
**    For this project, standard SPI transfers are used instead.
**
** 02/15/2025 - Documented DMA decision
**
** ------------------------------------------------------------------------------------------------
*/

#include <cmath>       // for fmod, fabs
#include <algorithm>   // for std::min, std::max
#include <unordered_map>

#include "DisplayUI.h"
#include "settings.h"
#include "logTags.h"
#include "SCLogger.h"
#include "Monitor.h"
#include "ThingPulse/util.h"  

#define FS_TP_LOGO "/ThingPulse-logo-260.jpeg"

/*
** ===================================================================
** toValue()
**    Helper function to convert TFTColor to standard color value
** ===================================================================
*/
constexpr uint16_t toValue(TFTColor color) 
{
    return static_cast<uint16_t>(color);
}

/*
** ===================================================================
** toRGB565()
**    Converts a color from RGB565 (16-bit) or RGB888 (24-bit) to
**    16-bit RGB565 format.
**
** Parameters:
**    color - Can be either:
**            - A 16-bit RGB565 color (TFTColor enum)
**            - A 24-bit RGB888 color (0xRRGGBB)
**
** Returns:
**    A 16-bit RGB565 color.
**
** Notes:
**    - If the input is already RGB565 (16-bit), it is returned as-is.
**    - If the input is RGB888 (24-bit), it is converted to RGB565.
**    - This function is useful for converting colors for TFT displays.
**
** Example Usage:
**    uint16_t red565 = toRGB565(TFTColor::Red);
**    uint16_t customBlue = toRGB565(0x0067B0); // Custom RGB888 color
**
**    _ofr->cdrawString("RED", posX, posY, red565, toRGB565(TFTColor::Black));
** ===================================================================
*/
uint16_t toRGB565(TFTColor tftColor) 
{

    // Convert to unsigned int
    uint32_t color = toValue(tftColor);

    // If color is 16-bit (RGB565), return it as-is
    if (color <= 0xFFFF) {
        return static_cast<uint16_t>(color);
    }

    // If color is 24-bit (RGB888), convert it to RGB565
    uint8_t r = (color >> 16) & 0xFF; // Extract Red (8-bit)
    uint8_t g = (color >> 8) & 0xFF;  // Extract Green (8-bit)
    uint8_t b = color & 0xFF;         // Extract Blue (8-bit)

    // Convert 8-bit RGB888 to 5-6-5 RGB565 format
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// TODO: Standardize singleton instantiation across the project.
static DisplayUI *pInstance = nullptr; 

/*
** ===================================================================
** DisplayUI() Constructor
** ===================================================================
*/
DisplayUI::DisplayUI(TFT_eSPI *tft, OpenFontRender *ofr, OpenFontRender *clockFont) 
{
    _tft        = tft;
    _ofr        = ofr;
    _clockFont  = clockFont;

    TJpgDec.setJpgScale(1);
    TJpgDec.setCallback(DisplayUI::jpgCallback);    
}

/*
** ===================================================================
** init()
**    Initialize the singleton.
** ===================================================================
*/
void DisplayUI::init()
{
   pInstance = this; 
}

/*
** ===================================================================
** isUIDirty()
**    Determines if the UI needs to be updated. Returns true if the
** UI is marked as dirty, false otherwise.
** ===================================================================
*/
bool DisplayUI::isUIDirty()
{
    bool answer = false;

    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        answer = _isUIDirty;
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"DisplayUI::isUIDirty - Unable to take xSemaphoreDisplay.");
    }    
    
    return answer;
}

/*
** ===================================================================
** markUIDirty()
**    Marks the UI as dirty or not dirty, based on the value of
** the isDirty parameter.
** ===================================================================
*/
void DisplayUI::markUIDirty(bool isDirty)
{
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        _isUIDirty = isDirty;
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"DisplayUI::markUIDirty - Unable to take xSemaphoreDisplay.");
    }    
}

/*
** ===================================================================
** getCenterWidth()
** ===================================================================
*/
int16_t DisplayUI::getCenterWidth()
{
    return _tft->width() / 2;
}

/*
** ===================================================================
** getCenterHeight()
** ===================================================================
*/
int16_t DisplayUI::getCenterHeight()
{
    return _tft->height() / 2;
}

/*
** ===================================================================
** getWidth()
** ===================================================================
*/
int16_t DisplayUI::getWidth()
{
    return _tft->width();
}

/*
** ===================================================================
** getHeight()
** ===================================================================
*/
int16_t DisplayUI::getHeight()
{
    return _tft->height();
}

/*
** ===================================================================
** drawBmp()
**    Renders a 24-bit BMP image from the LittleFS filesystem onto
**    the display at the specified (x, y) position. Optimized for
**    performance using a no-seek buffer and direct line decoding.
**
** Parameters:
**    filename - Path to the BMP file in the LittleFS filesystem.
**    x        - X-coordinate for the top-left corner of the image.
**    y        - Y-coordinate for the top-left corner of the image.
**
** Notes:
**    - Only uncompressed 24-bit BMP files are supported.
**    - Image is drawn bottom-up as per BMP specification.
**    - Invalid formats or missing files will generate log errors.
**
** Example Usage:
**    drawBmp("/splash.bmp", 10, 20);
** ===================================================================
*/

// Bodmer's streamlined x2 faster "no seek" version
void DisplayUI::drawBmp(String filename, uint16_t x, uint16_t y) {

  if ((x >= _tft->width()) || (y >= _tft->height()))
    return;

  fs::File bmpFS;

  // Note: ESP32 passes "open" test even if file does not exist, whereas ESP8266
  // returns NULL
  if (!LittleFS.exists(filename)) {
    spLogE(LOGTAG_GUI, " File not found");
    return;
  }

  // Open requested file
  bmpFS = LittleFS.open(filename, "r");

  uint32_t seekOffset;
  uint16_t w, h, row;
  uint8_t r, g, b;
  bool oldSwap = false;

  if (read16(bmpFS) == 0x4D42) {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0)) {
      y += h - 1;

      oldSwap = _tft->getSwapBytes();
      _tft->setSwapBytes(true);
      bmpFS.seek(seekOffset);

      // Calculate padding to avoid seek
      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {

        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t *bptr = lineBuffer;
        uint16_t *tptr = (uint16_t *)lineBuffer;
        // Convert 24 to 16 bit colours using the same line buffer for results
        for (uint16_t col = 0; col < w; col++) {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        _tft->pushImage(x, y--, w, 1, (uint16_t *)lineBuffer);
      }
    } else
      spLogE(LOGTAG_GUI, "BMP format not recognized.");
  }
  _tft->setSwapBytes(oldSwap);
  bmpFS.close();
}

/*
** ===================================================================
** drawLogo()
** ===================================================================
*/
void DisplayUI::drawLogo() {
  if (LittleFS.exists(FS_TP_LOGO)) {
    uint16_t w = 0, h = 0;
    TJpgDec.getFsJpgSize(&w, &h, FS_TP_LOGO, LittleFS);
    TJpgDec.drawFsJpg((_tft->width() - w) / 2, 30, FS_TP_LOGO, LittleFS);
  }
}

/*
** ===================================================================
** drawAppInfo()
** ===================================================================
*/
void DisplayUI::drawAppInfo() {
  _ofr->setFontSize(16);
  _ofr->cdrawString(APP_NAME, getCenterWidth(), _tft->height() - 50);
  _ofr->cdrawString(VERSION,  getCenterWidth(), _tft->height() - 30);
}

/*
** ===================================================================
** drawAlbumArt()
** ===================================================================
*/
void DisplayUI::drawAlbumArt(int32_t x, int32_t y, String filename) 
{
  spLogI(LOGTAG_GUI, "Entering drawAlbumArt. ttf width is %d. filename is %s.", _tft->width(), filename.c_str());

  if (LittleFS.exists(filename)) 
  {
    uint16_t w = 0, h = 0;
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        TJpgDec.getFsJpgSize(&w, &h, filename, LittleFS);
        spLogI(LOGTAG_GUI, "After getFsJpgSize(). ttf width is %d. w is %d. filename is %s.", _tft->width(), w, filename.c_str());
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }

    Monitor::start(MONITOR_ID_SPOTIFY_IMAGE_FILE_LOAD, LOGTAG_METRICS, "drawFsJpg(...)");
    drawFsJpg(x, y, filename.c_str(), LittleFS);

    Monitor::stop(MONITOR_ID_SPOTIFY_IMAGE_FILE_LOAD);

  }

}

/*
** ===================================================================
** drawProgressBar()
** ===================================================================
*/
void DisplayUI::drawProgressBar(uint16_t x0, uint16_t y0, uint16_t w, uint16_t h,
                            uint8_t percentage, TFTColor frameColor,
                            TFTColor barColor) 
{
    if (percentage == 0) 
    {
        _tft->fillRoundRect(x0, y0, w, h, 3, TFT_BLACK);
    }
    uint8_t margin = 2;
    uint16_t barHeight = h - 2 * margin;
    uint16_t barWidth = w - 2 * margin;

    _tft->drawRoundRect(x0, y0, w, h, 3, toValue(frameColor));

    _tft->fillRect(x0 + margin, y0 + margin, barWidth * percentage / 100.0,
                    barHeight, toValue(barColor));
}

/*
** ===================================================================
** drawStatusBox()
**       This call should be thread safe.
** ===================================================================
*/
void DisplayUI::drawStatusBox(TFTColor statusColor) 
{
    uint8_t margin = 2;
    int32_t boxX   = 450; //460; //10;
    int32_t boxY   = 290; //300;
    
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        _tft->drawRoundRect(boxX, boxY, 20, 20, 3, toValue(TFTColor::DarkGrey));
        _tft->fillRect(boxX + margin, boxY + margin, 16, 16, toValue(statusColor));
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay in drawStatusBox().");
    }
  }

/*
** ===================================================================
** drawDownloadIndicator()
**       This call should be thread safe.
** ===================================================================
*/
void DisplayUI::drawDownloadIndicator(bool bDownloadComplete) 
{
    uint8_t margin = 2;
    int32_t boxX   = 425; //460; //10;
    int32_t boxY   = 290; //300;

    TFTColor lineColor = TFTColor::Black; 
    TFTColor fillColor = TFTColor::Black;

    if (!isSplitBackground())
    {
        lineColor = getBackground(); 
        fillColor = getBackground();
    }    

    if (!bDownloadComplete)
    {
        lineColor = TFTColor::White;
        fillColor = TFTColor::SkyBlue;
    }
    
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        _tft->drawRoundRect(boxX, boxY, 20, 20, 3, toValue(lineColor));
        _tft->fillRect(boxX + margin, boxY + margin, 16, 16, toValue(fillColor));
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay in drawStatusBox().");
    }
  }

/*
** ===================================================================
** drawButton()
**       This call should be thread safe.
** ===================================================================
*/
void DisplayUI::drawButton(int32_t x, int32_t y, bool isPressed) 
{
    const uint8_t margin = 2;
    const int32_t width  = 90;
    const int32_t height = 90;
    
    drawBlankButton(x,y,width,height,margin,TFTColor::White,isPressed);

    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        // Draw rewind symbol (|<)
        const int32_t centerX = x + width / 2;
        const int32_t centerY = y + height / 2;
        const int32_t triangleSize = 20;
        const int32_t lineThickness = 5; // Adjust thickness of vertical line
        const int32_t lineX = centerX - 20; // Shifted left for proper placement
        const int32_t triangleX = lineX + lineThickness + 20;//2; // Right next to the line
        const int32_t triangleY = centerY - 15;
        const TFTColor symbolColor = TFTColor::White;

        // Draw vertical line '|' using a filled rectangle for thickness
        _tft->fillRect(lineX, centerY - 15, lineThickness, 30, toValue(symbolColor));

        // Draw left-facing triangle '<'
        _tft->fillTriangle(
            triangleX + 15, triangleY,               // Top point
            triangleX + 15, triangleY + 30,          // Bottom point
            triangleX - triangleSize, centerY,  // Leftmost point
            toValue(symbolColor)
        );

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay in drawStatusBox().");
    }
  }


/*
** ===================================================================
** drawBlankButton()
**    Draws a blank button with optional pressed effect and specified border color.
**    Thread-safe via xSemaphoreDisplay.
** ===================================================================
*/
void DisplayUI::drawBlankButton(int32_t x, int32_t y, int32_t width, int32_t height, uint8_t margin, TFTColor borderColor, bool isPressed)
{
    
    TFTColor bodyColor   = getBackground();
    // Invert the color
    TFTColor pressColor  = static_cast<TFTColor>(~static_cast<uint16_t>(bodyColor));

    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        _tft->drawRoundRect(x, y, width, height, 3, toValue(borderColor));

        if (isPressed)
        {
            _tft->fillRect(x + margin, y + margin, width - margin * 2, height - margin * 2, toValue(pressColor));
        }
        else
        {
            _tft->fillRect(x + margin, y + margin, width - margin * 2, height - margin * 2, toValue(bodyColor));
        }

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay in drawStatusBox().");
    }    
}

/*
** ===================================================================
** drawSkipTrackIcon()
**    Draws a skip track icon. Direction is determined by isReversed flag.
**    Thread-safe via xSemaphoreDisplay.
** ===================================================================
*/
void DisplayUI::drawSkipTrackIcon(int32_t x, int32_t y, int32_t width, int32_t height, bool isReversed)
{
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        // Define basic geometry
        const int32_t centerX       = x + width / 2;
        const int32_t centerY       = y + height / 2;
        const int32_t triangleSize  = 20;
        const int32_t lineThickness = 5;  // Thickness of the vertical line
        const int32_t offsetX       = 20; // Offset to adjust positioning

        // Determine positions based on isReversed
        const int32_t lineX = isReversed ? centerX - offsetX : centerX + offsetX - lineThickness;
        const int32_t triangleX = isReversed ? (lineX + lineThickness + 20) : (lineX - 20);
        const int32_t triangleY = centerY - 15;
        const TFTColor symbolColor = TFTColor::White;

        // Draw vertical line '|'
        _tft->fillRect(lineX, centerY - 15, lineThickness, 30, toValue(symbolColor));

        // Draw triangle '<' or '>'
        if (isReversed) 
        {
            // Left-facing triangle '<'
            _tft->fillTriangle(
                triangleX + 15, triangleY,          // Top point
                triangleX + 15, triangleY + 30,     // Bottom point
                triangleX - triangleSize, centerY,  // Leftmost point
                toValue(symbolColor)
            );
        } 
        else 
        {
            // Right-facing triangle '>'
            _tft->fillTriangle(
                triangleX - 15, triangleY,          // Top point
                triangleX - 15, triangleY + 30,     // Bottom point
                triangleX + triangleSize, centerY,  // Rightmost point
                toValue(symbolColor)
            );
        }

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay in drawSkipTrackIcon().");
    }
}

/*
** ===================================================================
** drawPlayTrackIcon()
**    Draws a right-facing triangle icon for the "play" action.
** ===================================================================
*/
void DisplayUI::drawPlayTrackIcon(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        // Define basic geometry
        const int32_t centerX       = x + width / 2;
        const int32_t centerY       = y + height / 2;
        const int32_t triangleSize  = 20;
        const int32_t lineThickness = 0;  // Thickness of the vertical line
        const int32_t offsetX       = 20; // Offset to adjust positioning

        // Determine positions based on isReversed
        const int32_t lineX        = centerX + offsetX - lineThickness;
        const int32_t triangleX    = lineX - 20;
        const int32_t triangleY    = centerY - 15;
        const TFTColor symbolColor = TFTColor::White;

        // Right-facing triangle '>'
        _tft->fillTriangle(
            triangleX - 15, triangleY,          // Top point
            triangleX - 15, triangleY + 30,     // Bottom point
            triangleX + triangleSize, centerY,  // Rightmost point
            toValue(symbolColor)
        );

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay in drawSkipTrackIcon().");
    }
}

/*
** ===================================================================
** drawPauseTrackIcon()
**    Draws two vertical bars to represent the "pause" icon.
** ===================================================================
*/
void DisplayUI::drawPauseTrackIcon(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        // Define basic geometry
        const int32_t centerX       = x + width / 2;
        const int32_t centerY       = y + height / 2;
        const int32_t lineThickness = 5; // Thickness of the vertical line
        const TFTColor symbolColor  = TFTColor::White;

        // Draw vertical line '|'
        _tft->fillRect(centerX - lineThickness * 2, centerY - 15, lineThickness, 30, toValue(symbolColor));
        _tft->fillRect(centerX + lineThickness * 2, centerY - 15, lineThickness, 30, toValue(symbolColor));

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay in drawSkipTrackIcon().");
    }
}

/*
** ===================================================================
** drawBackIcon()
**    Draws a back arrow icon pointing to the left.
** ===================================================================
*/
void DisplayUI::drawBackIcon(int32_t x, int32_t y, int32_t width, int32_t height)
{
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        // Define basic geometry
        const int32_t centerX       = x + width / 2;
        const int32_t centerY       = y + height / 2;
        const int32_t arrowWidth    = 20;
        const int32_t arrowHeight   = 10;
        const int32_t lineThickness = 5;
        const TFTColor symbolColor  = TFTColor::SC_NetworkSuccess; //::White;
        
        // Draw left-facing arrow '<-'
        // Draw the line part of the arrow
        _tft->fillRect(centerX - lineThickness, centerY - lineThickness / 2, arrowWidth, lineThickness, toValue(symbolColor));

        // Draw the triangular tip of the arrow
        _tft->fillTriangle(
            centerX - arrowWidth, centerY,                  // Tip of the arrow
            centerX, centerY - arrowHeight,                 // Top point of triangle
            centerX, centerY + arrowHeight,                 // Bottom point of triangle
            toValue(symbolColor)
        );

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay in drawBackTrackIcon().");
    }
}

/*
** ===================================================================
** read16()
** ===================================================================
*/

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t DisplayUI::read16(fs::File &f) 
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

/*
** ===================================================================
** read32()
** ===================================================================
*/
uint32_t DisplayUI::read32(fs::File &f) 
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

/*
** ===================================================================
** drawProgress()
** ===================================================================
*/
void DisplayUI::drawProgress(const char *text, int8_t percentage) {
  _ofr->setFontSize(24);
  int pbWidth = _tft->width() - 100;
  int pbX = (_tft->width() - pbWidth)/2;
  int pbY = 210; //260;
  int progressTextY = 160; //210;

  _tft->fillRect(0, progressTextY, _tft->width(), 40, TFT_BLACK);
  _ofr->cdrawString(text, getCenterWidth(), progressTextY);
  drawProgressBar(pbX, pbY, pbWidth, 15, percentage, TFTColor::White, TFTColor::BlueThinkPulse);
}

/*
** ===================================================================
** drawSeparator()
** ===================================================================
*/
void DisplayUI::drawSeparator(uint16_t y) 
{
  _tft->drawFastHLine(10, y, _tft->width() - 2 * 15, 0x4228);
}

/*
** ===================================================================
** clearScreen()
** ===================================================================
*/
void DisplayUI::clearScreen()
{
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {

         _tft->fillScreen(toValue(getBackground()));

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }

}

/*
** ===================================================================
** clearScreenKeepArt() - clear area around art
** ===================================================================
*/
void DisplayUI::clearScreenKeepArt()
{
    const int32_t lineSize   = 10;
    const int32_t borderSize = (_tft->width() - 300) / 2; // 300 is cover art size
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        // Top
        _tft->fillRect(0, 0, _tft->width() , lineSize, toValue(getBackground()));

        // Left 
        _tft->fillRect(0, 0, borderSize, _tft->height(), toValue(getBackground()));

        // Right
        _tft->fillRect(_tft->width() - borderSize, 0, borderSize, _tft->height(), toValue(getBackground()));

        // Bottom
        _tft->fillRect(0, _tft->height() - lineSize, _tft->width() , lineSize, toValue(getBackground()));

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }

}

/*
** ===================================================================
** clearScreenKeepArt() - clear area around art
** ===================================================================
*/
void DisplayUI::clearScreenHome()
{

    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        // Top
        _tft->fillRect(0, 0, _tft->width() , 200, toValue(getBackground()));

        // Bottom
        _tft->fillRect(0,200, _tft->width() , 320, toValue(TFTColor::Black));

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }

}

/*
** ===================================================================
** getBackground()
**    Returns the current background color of the UI.
** ===================================================================
*/
TFTColor DisplayUI::getBackground() const 
{
    return _bgColor;
}


/*
** ===================================================================
** isSplitBackground()
** ===================================================================
*/
bool DisplayUI::isSplitBackground() const
{
    // spLogI(LOGTAG_GENERAL,"isSplitBackground() returning %u", _isSplitBackground);
    return _isSplitBackground;
}

/*
** ===================================================================
** setSplitBackground()
** ===================================================================
*/
void DisplayUI::setSplitBackground(bool isSplitBackground) 
{
    // spLogI(LOGTAG_GENERAL,"setting _isSplitBackground to %u", isSplitBackground);
    _isSplitBackground = isSplitBackground;
}

/*
** ===================================================================
** setBackground()
** ===================================================================
*/
void DisplayUI::setBackground(TFTColor color, bool bRepaint)
{
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        _bgColor = color;
        if (bRepaint)
        {
            _tft->fillScreen(toValue(color));
        }
        
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }  

}

/*
** ===================================================================
** drawTextToLCD()
**    Displays text on the LCD at the specified vertical position,
**    using a default font size.
** ===================================================================
*/
void DisplayUI::drawTextToLCD(const char *text, int posY) 
{

    int pbWidth = _tft->width() - 100;
    int pbX     = (_tft->width() - pbWidth)/2;
    int pbY     = 260;
    int textY   = posY;

    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        _ofr->setFontSize(24);
        _tft->fillRect(0, textY, _tft->width(), 40, toValue(TFTColor::Black));
        _ofr->cdrawString(text, getCenterWidth(), textY);
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }

}

/*
** ===================================================================
** drawTextToLCD()
**    Overloaded version of drawTextToLCD that allows specifying
**    font size and whether to ignore rendering the text.
** ===================================================================
*/
void DisplayUI::drawTextToLCD(const char *text, int posY, int fontSize,  bool ignoreText) 
{

    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        _ofr->setFontSize(fontSize);

        if (ignoreText)
        {
            _tft->fillRect(0, posY, _tft->width(), fontSize + 10, toValue(getBackground()));
        }
        else
        {
            _tft->fillRect(0, posY, _tft->width(), fontSize + 10, toValue(TFTColor::Black));
            _ofr->cdrawString(text, getCenterWidth(), posY);   
        }
   

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }

}

/*
** ===================================================================
** rgb565()
**    Converts 24-bit RGB888 color to 16-bit RGB565 format.
** ===================================================================
*/
uint16_t rgb565(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF; // Extract Red
    uint8_t g = (color >> 8) & 0xFF;  // Extract Green
    uint8_t b = color & 0xFF;         // Extract Blue
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/*
** ===================================================================
** bgr565()
**    Converts 24-bit RGB888 color to 16-bit RGB565 with red and blue swapped.
** ===================================================================
*/
uint16_t bgr565(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF; // Extract Red
    uint8_t g = (color >> 8) & 0xFF;  // Extract Green
    uint8_t b = color & 0xFF;         // Extract Blue
    return ((b & 0xF8) << 8) | ((g & 0xFC) << 3) | (r >> 3); // Swap Red and Blue
}

/*
** ===================================================================
** grb565()
**    Converts 24-bit RGB888 color to 16-bit RGB565 with red and green swapped.
** ===================================================================
*/
uint16_t grb565(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF; // Extract Red
    uint8_t g = (color >> 8) & 0xFF;  // Extract Green
    uint8_t b = color & 0xFF;         // Extract Blue
    return ((g & 0xF8) << 8) | ((r & 0xFC) << 3) | (b >> 3); // Swap Red and Green
}

/*
** ===================================================================
** brg565()
**    Converts 24-bit RGB888 color to 16-bit RGB565 with blue and green swapped.
** ===================================================================
*/
uint16_t brg565(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF; // Extract Red
    uint8_t g = (color >> 8) & 0xFF;  // Extract Green
    uint8_t b = color & 0xFF;         // Extract Blue
    return ((b & 0xF8) << 8) | ((r & 0xFC) << 3) | (g >> 3); // Swap Blue and Green
}

/*
** ===================================================================
** rgb888_to_rgb565()
**    Converts a 24-bit RGB888 color value to 16-bit RGB565 format.
** ===================================================================
*/
uint16_t rgb888_to_rgb565(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF; // Extract Red (8-bit)
    uint8_t g = (color >> 8) & 0xFF;  // Extract Green (8-bit)
    uint8_t b = color & 0xFF;         // Extract Blue (8-bit)
    
    // Convert 8-bit RGB888 to 5-6-5 RGB565 format
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/*
** ===================================================================
** convertColorForILI9488()
**    Converts a color from RGB565 (16-bit) or RGB888 (24-bit) to
**    the correct format for the ILI9488 display. Ensures that colors
**    defined in the TFTColor enum render correctly.
**
** Parameters:
**    color - Can be either:
**            - A 16-bit RGB565 color (from TFTColor enum)
**            - A 24-bit RGB888 color (0xRRGGBB)
**
** Returns:
**    16-bit RGB565 color, correctly formatted for the ILI9488.
**
** Notes:
**    - If the input is already RGB565 (16-bit), it is returned as-is.
**    - If the input is RGB888 (24-bit), it is converted to RGB565.
**    - This ensures that colors from the TFTColor enum render correctly.
**
** Example Usage:
**    uint16_t red565 = convertColorForILI9488(TFTColor::Red);
**    uint16_t customBlue = convertColorForILI9488(0x0067B0); // Custom RGB888 color
**
**    _ofr->cdrawString("RED", posX, posY, red565, convertColorForILI9488(TFTColor::Black));
** ===================================================================
*/
uint16_t convertColorForILI9488(uint32_t color) {
    // If color is 16-bit (RGB565), return it as-is
    if (color <= 0xFFFF) {
        return static_cast<uint16_t>(color);
    }

    // If color is 24-bit (RGB888), convert it to RGB565
    uint8_t r = (color >> 16) & 0xFF; // Extract Red (8-bit)
    uint8_t g = (color >> 8) & 0xFF;  // Extract Green (8-bit)
    uint8_t b = color & 0xFF;         // Extract Blue (8-bit)

    // Convert 8-bit RGB888 to 5-6-5 RGB565 format
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

/*
** ===================================================================
** drawText()
**    Draws text on the display at a given position and font size.
** ===================================================================
*/
void DisplayUI::drawText(const char *text, int posX, int posY, int fontSize, TFTColor color) 
{

    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        _ofr->setFontSize(fontSize);
        _ofr->setAlignment(Align::BottomLeft);
        _tft->fillRect(posX - 70, posY, 140, fontSize + 10, toValue(TFTColor::Black));
        _ofr->cdrawString(text, posX, posY,  toRGB565(color), toRGB565(TFTColor::Black));  

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }

}

/*
** ===================================================================
** drawString()
**    Draws a string at the specified location with the given font,
**    foreground and background colors, alignment, and optional clear mask.
** ===================================================================
*/
void DisplayUI::drawString(const char *str, 
                            int32_t x, 
                            int32_t y, 
                            unsigned int    fontSize, 
                            TFTColor        fg, 
                            TFTColor        bg, 
                            Align           alignment, 
                            const char      *clearMask)
{
    _ofr->setAlignment(alignment);
    _ofr->setFontSize(fontSize);
    uint16_t fgColor = toRGB565(fg);
    uint16_t bgColor = toRGB565(bg);

    // figure out what to clear
    FT_BBox bbox;
    if (strlen(clearMask) > 0)
    {
        // clear just the mask
        bbox           = _ofr->calculateBoundingBox(x, y, _ofr->getFontSize(), Align::Left, Layout::Horizontal, clearMask);
        int     width  = bbox.xMax - bbox.xMin;
        int     height = bbox.yMax - bbox.yMin;
        _tft->fillRect(x, bbox.yMin, width, height, toValue(bg));
    }
    else
    {
        // clear entire line
        bbox           = _ofr->calculateBoundingBox(x, y, _ofr->getFontSize(), Align::Left, Layout::Horizontal, "AG123jt");
        int     width  = bbox.xMax - bbox.xMin;
        int     height = bbox.yMax - bbox.yMin;        
        _tft->fillRect(0, y, _tft->width(), height, toValue(bg));        
    }

    _ofr->drawString(str, x, y, fgColor, bgColor, Layout::Horizontal);    
}

/*
** ===================================================================
** cDrawString()
**    Draws a centered string with foreground and background color.
** ===================================================================
*/
void DisplayUI::cDrawString( const char  *str,
    int32_t      x,
    int32_t      y,
    unsigned int fontSize,
    TFTColor     fg,
    TFTColor     bg,
    const char  *clearMask)
{
    drawString(str, x, y, fontSize, fg, bg, Align::Center, clearMask);
}

/*
** ===================================================================
** lDrawString()
**    Draws a left-aligned string with foreground and background color.
** ===================================================================
*/
void DisplayUI::lDrawString( const char  *str,
                            int32_t      x,
                            int32_t      y,
                            unsigned int fontSize,
                            TFTColor     fg,
                            TFTColor     bg,
                            const char  *clearMask)
{
    drawString(str, x, y, fontSize, fg, bg, Align::Left, clearMask);
}

/*
** ===================================================================
** rDrawString()
**    Draws a right-aligned string with foreground and background color.
** ===================================================================
*/
void DisplayUI::rDrawString( const char  *str,
                            int32_t      x,
                            int32_t      y,
                            unsigned int fontSize,
                            TFTColor     fg,
                            TFTColor     bg,
                            const char  *clearMask)
{
    _ofr->setAlignment(Align::Right);
    _ofr->setFontSize(fontSize);
    uint16_t fgColor = toRGB565(fg);
    uint16_t bgColor = toRGB565(bg);

    // figure out what to clear
    FT_BBox bbox   = _ofr->calculateBoundingBox(x, y, _ofr->getFontSize(), Align::Right, Layout::Horizontal, clearMask);
    int     width  = bbox.xMax - bbox.xMin;
    int     height = bbox.yMax - bbox.yMin;
    _tft->fillRect(x - width, bbox.yMin, width, height, toValue(bg));

    _ofr->drawString(str, x, y, fgColor, bgColor, Layout::Horizontal);
}

/*
** ===================================================================
** drawClockTime()
**    Draws the current time using clock font and updates only when values change.
** ===================================================================
*/
void DisplayUI::drawClockTime(bool isUSFormat, bool isForceRepaint)
{
    const int fontSize  = 128;
    const int fontWidth = 76;

    int offset          = 0;
    std::string hours   = "";
    if (isUSFormat)
    {
        hours  = getCurrentTimestamp("%l").c_str();  
    }
    else
    {
        hours  = getCurrentTimestamp("%H").c_str();
        offset = 50;
    }
    std::string minutes = getCurrentTimestamp("%M").c_str();  
    std::string ampm    = getCurrentTimestamp("%p").c_str();  
    std::string seconds = getCurrentTimestamp("%S").c_str();

    static std::string lastHours   = "";
    static std::string lastMinutes = "";
    static std::string lastAmpm    = "";

    // Hours
    if ((lastHours != hours)
    ||  isForceRepaint)
    {
        _tft->fillRect(20 + offset, 35, fontWidth * 2, 100, toValue(getBackground())); 
        drawClockString(hours.c_str(), 20 + offset, 25, fontSize, TFTColor::White, getBackground());   
        lastHours = hours;     
    }

    // Seconds indicator :
    static bool isLastColonVisible      = true;
    int         iSeconds              = std::stoi(seconds);
    bool        isCurrentColonVisible = (iSeconds % 2 == 0);
    if (isCurrentColonVisible != isLastColonVisible 
    ||  isForceRepaint)
    {
        _tft->fillRect(180 + offset, 53, 35, 84, toValue(getBackground())); 
        const char* colonChar = isCurrentColonVisible ? ":" : " ";
        drawClockString(colonChar, 160 + offset, 20, fontSize, TFTColor::Yellow, getBackground());
        isLastColonVisible = isCurrentColonVisible;
    }

    // Minutes
    if ((lastMinutes != minutes)
    ||  isForceRepaint)
    {  
        _tft->fillRect(225 + offset, 35, fontWidth * 2, 100, toValue(getBackground())); 
        drawClockString(minutes.c_str(), 220 + offset, 25, fontSize, TFTColor::White, getBackground());
        lastMinutes = minutes;     
    }

    // AM/PM if US Format
    if (isUSFormat)
    {
        if ((lastAmpm != ampm)
        ||  isForceRepaint)
        { 
            _tft->fillRect(380, 40, fontWidth, 100, toValue(getBackground())); //toValue(getBackground())); 
            if (ampm == "AM")
            {
                drawClockString(ampm.c_str(), 380, 35, fontSize / 2, TFTColor::Yellow, getBackground());
            }  
            else
            {
                drawClockString(ampm.c_str(), 380, 80, fontSize / 2, TFTColor::Yellow, getBackground());           
            } 

            lastAmpm = ampm;     
        }
    }
}

/*
** ===================================================================
** drawClockString()
**    Draws a string using the clock font renderer with a specified
**    foreground and background color at a given position.
**
** Parameters:
**    str      - The string to draw.
**    x        - X-coordinate on the screen.
**    y        - Y-coordinate on the screen.
**    fontSize - Font size to use for rendering.
**    fg       - Foreground color (text color).
**    bg       - Background color.
**
** Notes:
**    - This method does not clear the area before drawing. The caller
**      must clear the area manually if needed.
**    - Intended for use in clock display rendering.
**
** Example Usage:
**    drawClockString("12", 30, 30, 96, TFTColor::Yellow, TFTColor::Black);
** ===================================================================
*/
void DisplayUI::drawClockString(
    const char *str, 
    int32_t x, 
    int32_t y, 
    unsigned int    fontSize, 
    TFTColor        fg, 
    TFTColor        bg)
{
    _clockFont->setAlignment(Align::Left);
    _clockFont->setFontSize(fontSize);
    _clockFont->drawString(str, x, y, toRGB565(fg), toRGB565(bg), Layout::Horizontal);    
}

/*
** ===================================================================
** formatTime()
**    Converts a time duration from milliseconds to a string in the
**    format HH:MM:SS or MM:SS if hours are zero.
**
** Parameters:
**    millis - Time duration in milliseconds.
**
** Returns:
**    A string representing the time:
**      - "H:MM:SS" if hours > 0
**      - "M:SS" if hours == 0
**
** Notes:
**    - Removes leading zeros (e.g., "1:05" instead of "01:05").
**    - Skips hours if they are zero (e.g., "5:30" instead of "00:05:30").
**
** Example Usage:
**    Serial.println(formatTime(3661000)); // Output: "1:01:01"
**    Serial.println(formatTime(59000));   // Output: "59"
**    Serial.println(formatTime(3600000)); // Output: "1:00:00"
**    Serial.println(formatTime(300000));  // Output: "5:00"
** ===================================================================
*/
std::string DisplayUI::formatTime(long millis) {
    long totalSeconds = millis / 1000;
    int hours         = totalSeconds / 3600;
    int minutes       = (totalSeconds % 3600) / 60;
    int seconds       = totalSeconds % 60;

    std::string result;
    
    if (hours > 0) 
    {
        result += std::to_string(hours) + ":";
    }

    result += std::to_string(minutes) + ":"; // No leading zero for minutes
    if (seconds < 10) 
    {
        result += "0"; // Ensure seconds always have two digits
    }
    result += std::to_string(seconds);

    return result;
}


/*
** ===================================================================
** showTouchDown()
**    Highlights UI borders in the specified color to indicate a touch-down event.
** ===================================================================
*/
void DisplayUI::showTouchDown(TFTColor color) 
{
    paintTouch(true, color);   
}

/*
** ===================================================================
** showTouchUp()
**    Clears UI border highlights to indicate a touch-up event.
** ===================================================================
*/
void DisplayUI::showTouchUp() 
{
    paintTouch(false, getBackground());  
}

/*
** ===================================================================
** paintTouch()
**    Helper method to paint or clear UI border highlights based on touch state.
** ===================================================================
*/
void DisplayUI::paintTouch(bool isPress, TFTColor color)
{
    const int32_t lineSize = 10;

    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        // Top
        _tft->fillRect(0, 0, _tft->width() , lineSize, toValue(color));

        if (!isSplitBackground())
        {
            // Left 
            _tft->fillRect(0, lineSize, lineSize, _tft->height(), toValue(color));

            // Right
            _tft->fillRect(_tft->width() - lineSize, lineSize, lineSize, _tft->height(), toValue(color));

            // Bottom
            _tft->fillRect(0, _tft->height() - lineSize, _tft->width() , lineSize, toValue(color));
        }

        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }

}

/*
** ===================================================================
** getPreviousActionPoint()
**    top third of the display, left of the cover art
** ===================================================================
*/
Point DisplayUI::getPreviousActionPoint()
{
    // top third of the display, left of the cover art
    int x = (_tft->width() - COVER_ART_SIZE) / 2;
    int y = (_tft->height()) / 3;
    return Point(x,y);
}

/*
** ===================================================================
** getNextActionPoint()
**    top third of the display, right of the cover art
** ===================================================================
*/
Point DisplayUI::getNextActionPoint()
{
    int x = _tft->width() - ((_tft->width() - COVER_ART_SIZE) / 2);
    int y = (_tft->height()) / 3;
    return Point(x,y);
}

/*
** ===================================================================
** pushImageToTft() 
**
** Function will be called as a callback during decoding of a JPEG file to
** render each block to the TFT.
** ===================================================================
*/

bool DisplayUI::pushImageToTft(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  // Stop further decoding as image is running off bottom of screen
  if (y >= _tft->height()) {
    return 0;
  }

  // Automatically clips the image block rendering at the TFT boundaries.
  _tft->pushImage(x, y, w, h, bitmap);

  // Return 1 to decode next block
  return 1;
}

/*
** ===================================================================
** setJpgScaleToSmall()
**
**    Adjusts the JPEG decoder scaling factor to either full scale (1:1)
**    or half scale (1/2) based on the provided flag. Ensures thread 
**    safety by using a semaphore to coordinate access to the display.
**
** Parameters:
**    isSmall - If true, sets the decoder to half scale (80x80 resolution).
**              If false, sets the decoder to full scale (160x160 resolution).
**
** Notes:
**    - Uses `xSemaphoreDisplay` to ensure exclusive access to the display.
**    - If the semaphore cannot be acquired, a log message is generated.
**    - JPEG scaling is controlled using `TJpgDec.setJpgScale()`.
**
** ===================================================================
*/
void DisplayUI::setJpgScaleToSmall(bool isSmall)
{
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        if (isSmall)
        {
            TJpgDec.setJpgScale(2); // half scale
        }
        else
        {
            TJpgDec.setJpgScale(1); // full scale
        }
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }     
}

/*
** ===================================================================
** setJpgScaleToTiny()
**    Sets the JPEG decoder to use 1/8th scaling for rendering.
** ===================================================================
*/
void DisplayUI::setJpgScaleToTiny()
{
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        TJpgDec.setJpgScale(8); // 1/8th scale
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }     
}

/*
** ===================================================================
** drawFsJpg()
**
**    Draws a JPEG image from the filesystem to the display at a 
** specified location. Ensures thread safety by using a semaphore
** to coordinate access to the display. Returns a result code
** indicating success or failure.
**
** Parameters:
**    x         - X-coordinate on the display where the image will be drawn.
**    y         - Y-coordinate on the display where the image will be drawn.
**    pFilename - Path to the JPEG file in the filesystem.
**    fs        - Filesystem instance (e.g., LittleFS, SPIFFS) containing 
**                the image file.
**
** Returns:
**    JRESULT - JPEG decoding result code (e.g., JDR_OK for success).
**
** Notes:
**    - If the file does not exist, the method logs an error and returns
**      a parameter error code (JDR_PAR).
**    - If the semaphore cannot be taken, the method logs a message and
**      skips the drawing operation.
**    - The method is thread-safe due to semaphore protection.
**
** ===================================================================
*/

JRESULT DisplayUI::drawFsJpg(int32_t x, int32_t y, const char *pFilename, fs::FS &fs)
{
  JRESULT result = JDR_PAR;  // Default to parametere error
    
  if (LittleFS.exists(pFilename)) 
  {
    if (xSemaphoreTake(xSemaphoreDisplay, portMAX_DELAY)) 
    {
        result = TJpgDec.drawFsJpg(x, y, pFilename, fs);
        xSemaphoreGive(xSemaphoreDisplay);
    }
    else
    {
        spLogI(LOGTAG_MULTITASK,"Unable to take xSemaphoreDisplay.");
    }
  }
  else
  {
    spLogE(LOGTAG_MULTITASK,"Filename does not exist.");
  }

  return result;

}

/*
** ===================================================================
** jpgCallback()
**    JPEG decoder callback to render or analyze JPEG pixels.
** ===================================================================
*/
bool DisplayUI::jpgCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
    // Implementation for handling the callback
    bool result = false;
    if (pInstance->_isPainting)
    {
        result = pInstance->pushImageToTft(x, y, w, h, bitmap);
    }
    else
    {
        result = pInstance->jpgCalculateAverageColorCallback(x, y, w, h, bitmap);
    }
    
    return result; // Placeholder
}


/*
** ===================================================================
** Helper Function: fromRGB()
**    Converts 8-bit R, G, B values to a 16-bit RGB565 TFTColor.
** ===================================================================
*/
constexpr TFTColor fromRGB(uint8_t red, uint8_t green, uint8_t blue) {
    return static_cast<TFTColor>(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
}

// Globals for color calculation
static uint32_t totalR = 0, totalG = 0, totalB = 0;
static uint32_t pixelCount = 0;

/*
** ===================================================================
** jpgCalculateAverageColorCallback()
**    Callback used during JPEG decoding to compute the average color
**    of the image for background styling or theme adaptation.
** ===================================================================
*/

// Callback function for pixel processing
bool DisplayUI::jpgCalculateAverageColorCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) 
{

    // Sampling factor (e.g., process every 4th pixel)

    const uint16_t samplingFactor = 4; // 16; //4;

    for (uint16_t i = 0; i < w * h; i += samplingFactor) 
    {
        uint16_t pixel = bitmap[i];

        uint8_t  r8    = (pixel >> 11) & 0x1F;  // 5-bit R
        uint8_t  g8    = (pixel >> 5) & 0x3F;   // 6-bit G
        uint8_t  b8    = pixel & 0x1F;          // 5-bit B

        // Scale up to 8-bit
        uint8_t  r     = r8 << 3;
        uint8_t  g     = g8 << 2;
        uint8_t  b     = b8 << 3;

        int brightness = (r + g + b) / 3;

        // Skip extremely bright or extremely dark
        if (brightness < 240 
        &&  brightness > 20) 
        { 

            totalR += r;
            totalG += g;
            totalB += b;
            pixelCount++;
        }        
    }

    return true; // Continue processing

}

/*
** ===================================================================
** calculateAverageColor()
** ===================================================================
*/
// Calculate average color of JPEG file
TFTColor DisplayUI::calculateAverageColor(const char* filename) 
{
    totalR     = 0;
    totalG     = 0;
    totalB     = 0;
    pixelCount = 0;    

    spLogV(LOGTAG_PLAYER, "Entering calculateAverageColor(%s)", filename);

    Monitor::start(MONITOR_ID_CALCULATE_AVG_BACKGROUND_COLOR, LOGTAG_METRICS, "- drawFsJpg(0, 0, filename, LittleFS)");
    _isPainting = false;  // TODO: wrap to make thread safe... maybe this block until it is reset.
    drawFsJpg(0, 0, filename, LittleFS);
    _isPainting = true;
    spLogV(LOGTAG_PLAYER,"decoder.drawFsJpg() result is %d",filename);
    Monitor::stop(MONITOR_ID_CALCULATE_AVG_BACKGROUND_COLOR);

    if (pixelCount == 0) {
        spLogE(LOGTAG_SONG_DATA, "No pixels processed from the image.");
        return TFTColor::Black;
    }

    uint8_t avgR = totalR / pixelCount;
    uint8_t avgG = totalG / pixelCount;
    uint8_t avgB = totalB / pixelCount;

    spLogV(LOGTAG_PLAYER, "Average color: R=%d, G=%d, B=%d", avgR, avgG, avgB);
    spLogV(LOGTAG_PLAYER, "pixelCount = %d", pixelCount);

    // Convert to HSV, boost saturation, convert back ---
    float h, s, v;
    DisplayUI::rgbToHsv(avgR, avgG, avgB, h, s, v);

    // Increase saturation (example: 1.5x)
    s = std::min(s * 1.5f, 1.0f);  // clamp at 1.0

    // Convert back to RGB
    uint8_t finalR, finalG, finalB;
    DisplayUI::hsvToRgb(h, s, v, finalR, finalG, finalB);

    spLogV(LOGTAG_PLAYER, "Boosted color: R=%d, G=%d, B=%d", finalR, finalG, finalB);

    // Convert to 565
    TFTColor answer = fromRGB(finalR, finalG, finalB);

    return answer;
}

/*
** ===================================================================
** rgbToHsv()
**    Converts an RGB color value to HSV (Hue, Saturation, Value).
**    This function modifies the hue, saturation, and value floats
**    passed in by reference.
**
** Parameters:
**    r - The red component in [0..255].
**    g - The green component in [0..255].
**    b - The blue component in [0..255].
**    h - Output for hue in degrees [0..360).
**    s - Output for saturation in [0..1].
**    v - Output for value (brightness) in [0..1].
**
** Returns:
**    None. The results are written to h, s, and v by reference.
** ===================================================================
*/
void DisplayUI::rgbToHsv(uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& v)
{
    // Convert from 8-bit [0..255] to floating-point [0..1]
    float fr = r / 255.0f;
    float fg = g / 255.0f;
    float fb = b / 255.0f;

    // Find the maximum and minimum among R, G, B
    float maxVal = std::max({fr, fg, fb});
    float minVal = std::min({fr, fg, fb});
    float delta  = maxVal - minVal;  // Range between max and min

    // "Value" (brightness) in HSV is simply the maximum component
    v = maxVal;

    // If delta is extremely small, color is essentially grayscale
    if (delta < 0.00001f)
    {
        s = 0.0f;  // No saturation
        h = 0.0f;  // Hue is undefined, treat as 0
        return;
    }

    // Saturation is the ratio of the color range to the brightness
    s = (maxVal > 0.0f) ? (delta / maxVal) : 0.0f;

    // Determine hue based on which channel is the maximum
    if (maxVal == fr)
    {
        // If red is max, hue depends on (green - blue) relative to delta
        h = 60.0f * fmod(((fg - fb) / delta), 6.0f);
        // Wrap negative angles into the [0..360) range
        if (h < 0.0f)
        {
            h += 360.0f;
        }
    }
    else if (maxVal == fg)
    {
        // If green is max, hue is in the 120..180 range
        h = 60.0f * (((fb - fr) / delta) + 2.0f);
    }
    else  // maxVal == fb
    {
        // If blue is max, hue is in the 240..300 range
        h = 60.0f * (((fr - fg) / delta) + 4.0f);
    }
}

/*
** ===================================================================
** hsvToRgb()
**    Converts an HSV (Hue, Saturation, Value) color value to RGB.
**    This function modifies the red, green, and blue bytes passed
**    in by reference.
**
** Parameters:
**    h - The hue in degrees [0..360).
**    s - The saturation in [0..1].
**    v - The value (brightness) in [0..1].
**    r - Output for red in [0..255].
**    g - Output for green in [0..255].
**    b - Output for blue in [0..255].
**
** Returns:
**    None. The results are written to r, g, and b by reference.
** ===================================================================
*/
void DisplayUI::hsvToRgb(float h, float s, float v, uint8_t& r, uint8_t& g, uint8_t& b)
{
    // c is the "chroma"—the intensity of the color (range [0..v])
    float c = v * s;

    // x is an intermediate value used to spread out the color
    // based on how far H is into each 60-degree segment
    float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));

    // m adjusts the final values to match the requested Value (v)
    float m = v - c;

    // Temporary floats for the "pure" R, G, B before adding m
    float fr = 0;
    float fg = 0;
    float fb = 0;

    // Determine which 60-degree segment of the 360-degree Hue range we're in
    if (h < 60)
    {
        // Segment 0: Red is at full chroma, green is partial, blue is 0
        fr = c;
        fg = x;
        fb = 0;
    }
    else if (h < 120)
    {
        // Segment 1: Green is at full chroma, red is partial, blue is 0
        fr = x;
        fg = c;
        fb = 0;
    }
    else if (h < 180)
    {
        // Segment 2: Green is at full chroma, blue is partial, red is 0
        fr = 0;
        fg = c;
        fb = x;
    }
    else if (h < 240)
    {
        // Segment 3: Blue is at full chroma, green is partial, red is 0
        fr = 0;
        fg = x;
        fb = c;
    }
    else if (h < 300)
    {
        // Segment 4: Blue is at full chroma, red is partial, green is 0
        fr = x;
        fg = 0;
        fb = c;
    }
    else
    {
        // Segment 5: Red is at full chroma, blue is partial, green is 0
        fr = c;
        fg = 0;
        fb = x;
    }

    // Convert the float values [0..1] into 8-bit [0..255] range
    // and add m to each channel to match the intended brightness (Value)
    r = static_cast<uint8_t>((fr + m) * 255);
    g = static_cast<uint8_t>((fg + m) * 255);
    b = static_cast<uint8_t>((fb + m) * 255);
}
