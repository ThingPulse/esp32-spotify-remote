// SPDX-FileCopyrightText: 2023 ThingPulse Ltd., https://thingpulse.com
// SPDX-License-Identifier: MIT

#include <LittleFS.h>

// Specify all #define statements for task scheduler first
#define _TASK_SCHEDULING_OPTIONS
//#define _TASK_TIMECRITICAL
//#define _TASK_SLEEP_ON_IDLE_RUN
#include <TaskScheduler.h>

#include <OpenFontRender.h>
#include <TJpg_Decoder.h>

#include "fonts/open-sans.h"
#include "GfxUi.h"

#include "connectivity.h"
#include "display.h"
#include "persistence.h"
#include "settings.h"
#include "util.h"
#include "spotify.h"



// ----------------------------------------------------------------------------
// Function prototypes (declarations)
// ----------------------------------------------------------------------------
void drawProgress(const char *text, int8_t percentage);
void initJpegDecoder();
void initOpenFontRender();
void initScheduler();
bool pushImageToTft(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);
void syncTime();
void repaint();



// ----------------------------------------------------------------------------
// Globals
// ----------------------------------------------------------------------------
OpenFontRender ofr;
FT6236 ts = FT6236(TFT_HEIGHT, TFT_WIDTH);
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite timeSprite = TFT_eSprite(&tft);
GfxUi ui = GfxUi(&tft, &ofr);

Scheduler schedule;
// Create a task to run every hour to update the 'clock'
Task clockTask(CLOCK_TASK_INTERVAL_MILLIS, TASK_FOREVER, &syncTime);

// time management variables
unsigned long lastTimeSyncMillis = 0;

const int16_t centerWidth = tft.width() / 2;

String spotifyRefreshToken = "";



// ----------------------------------------------------------------------------
// setup() & loop()
// ----------------------------------------------------------------------------
void setup(void) {
  Serial.begin(115200);
  delay(1000);

  logBanner();
  logMemoryStats();

  initJpegDecoder();
  initTouchScreen(&ts);
  initTft(&tft);
  logDisplayDebugInfo(&tft);

  initFileSystem();
  initOpenFontRender();

  initSpotifiy();

  initScheduler();

  tft.fillScreen(TFT_BLACK);
  ui.drawLogo();

  ofr.setFontSize(16);
  ofr.cdrawString(APP_NAME, centerWidth, tft.height() - 50);
  ofr.cdrawString(VERSION, centerWidth, tft.height() - 30);

  drawProgress("Starting WiFi...", 10);
  if (WiFi.status() != WL_CONNECTED) {
    startWiFi();
  }

  drawProgress("Synchronizing time...", 40);
  syncTime();

  drawProgress("Checking Spotify status...", 60);
  spotifyRefreshToken = readFsString(SPOTIFY_REFRESH_TOKEN_FILE_NAME);
  if (spotifyRefreshToken == "") {
    log_i("No Spotify refresh token found. Requesting one through the browser via auth code.");

    drawProgress("Getting Spotify token...", 70);
    ofr.cdrawString(String("Open browser at\nhttp://" SPOTIFY_ESPOTIFIER_NODE_NAME ".local").c_str(), centerWidth, 280);

    String spotifyAuthCode = fetchSpotifyAuthCode();
    spotifyRefreshToken = spotify.requestAccessTokens(spotifyAuthCode.c_str(), SPOTIFY_REDIRECT_URI);
    saveFsString(SPOTIFY_REFRESH_TOKEN_FILE_NAME, spotifyRefreshToken);

    // clear text beneath the progress bar
    tft.fillRect(0, 290, tft.width(), 80, TFT_BLACK);
  } else {
    log_i("Using previously saved Spotify refresh token.");
  }

  drawProgress("Logging into Spotify...", 90);
  // The Spotify library
  // - keeps track of the refresh token and its TTL internally
  // - automatically renews the actual access token using the refresh token
  // -> see SpotifyArduino.h#autoTokenRefresh and SpotifyArduino::checkAndRefreshAccessToken() (called before every API function)
  spotify.setRefreshToken(spotifyRefreshToken.c_str());
  spotify.refreshAccessToken();
  log_i("Authentication against Spotify done. Refresh token: %s", spotifyRefreshToken.c_str());

  drawProgress("Startup completed!", 100);
}

void loop(void) {
  // Task execution
  schedule.execute();

  // if (ts.touched()) {
  //   TS_Point p = ts.getPoint();

  //   uint16_t touchX = p.x;
  //   uint16_t touchY = p.y;

  //   log_d("Touch coordinates: x=%d, y=%d", touchX, touchY);
  //   // Debouncing; avoid returning the same touch multiple times.
  //   delay(50);
  // }
}



// ----------------------------------------------------------------------------
// Functions
// ----------------------------------------------------------------------------

// void drawProgress(const char *text, int8_t percentage) {
//   int numberOfLinebreaks = 0;
//   int numberOfChars = strlen(text);
//   for (int i = 0; i < numberOfChars; i++) {
//     if (text[i] == '\n') {
//       numberOfLinebreaks++;
//     }
//   }

//   ofr.setFontSize(24);
//   int pbWidth = tft.width() - 100;
//   int pbX = (tft.width() - pbWidth)/2;
//   int pbY = 260;
//   int progressTextY = 210;

//   tft.fillRect(0, progressTextY, tft.width(), 40, TFT_BLACK);
//   ofr.cdrawString(text, centerWidth, progressTextY - (numberOfLinebreaks * 26));
//   ui.drawProgressBar(pbX, pbY, pbWidth, 15, percentage, TFT_WHITE, TFT_TP_BLUE);
// }
void drawProgress(const char *text, int8_t percentage) {
  ofr.setFontSize(24);
  int pbWidth = tft.width() - 100;
  int pbX = (tft.width() - pbWidth)/2;
  int pbY = 260;
  int progressTextY = 210;

  tft.fillRect(0, progressTextY, tft.width(), 40, TFT_BLACK);
  ofr.cdrawString(text, centerWidth, progressTextY);
  ui.drawProgressBar(pbX, pbY, pbWidth, 15, percentage, TFT_WHITE, TFT_TP_BLUE);
}

void drawSeparator(uint16_t y) {
  tft.drawFastHLine(10, y, tft.width() - 2 * 15, 0x4228);
}

void initJpegDecoder() {
    // The JPEG image can be scaled by a factor of 1, 2, 4, or 8 (default: 0)
  TJpgDec.setJpgScale(1);
  // The decoder must be given the exact name of the rendering function
  TJpgDec.setCallback(pushImageToTft);
}

void initOpenFontRender() {
  ofr.loadFont(opensans, sizeof(opensans));
  ofr.setDrawer(tft);
  ofr.setFontColor(TFT_WHITE);
  ofr.setBackgroundColor(TFT_BLACK);
}

void initScheduler() {
  // Set the options for the task so that it "catches up" if there is a delay
  clockTask.setSchedulingOption(TASK_SCHEDULE);

  // Initialise the task scheduler and start the tasks
  schedule.init();
  schedule.addTask(clockTask);
  clockTask.enable();
}

// Function will be called as a callback during decoding of a JPEG file to
// render each block to the TFT.
bool pushImageToTft(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  // Stop further decoding as image is running off bottom of screen
  if (y >= tft.height()) {
    return 0;
  }

  // Automatically clips the image block rendering at the TFT boundaries.
  tft.pushImage(x, y, w, h, bitmap);

  // Return 1 to decode next block
  return 1;
}

void syncTime() {
  if (initTime()) {
    lastTimeSyncMillis = millis();
    setTimezone(TIMEZONE);
    log_i("Current local time: %s", getCurrentTimestamp(SYSTEM_TIMESTAMP_FORMAT).c_str());
  }
}
