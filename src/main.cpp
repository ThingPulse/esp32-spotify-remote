/*-------------------------------------------------------------------------------------------------
**
** main.cpp
**
**    Main entry point for the Spotify Companion application. Initializes system
**    components, configures logging, synchronizes time, handles Wi-Fi and Spotify
**    authentication, and launches UI and background tasks on the ESP32.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-02-09 - Electric Diversions - Major update with refined UI task, touch input, and queue handling.
** ------------------------------------------------------------------------------------------------
*/

/*
** ===================================================================
** Include main library dependencies
** ===================================================================
*/        

//
// *** Task Scheduler Include ***
// Cooperative multitasking for Arduino, ESPx, STM32 
// and other microcontrollers
// https://github.com/arkhipenko/TaskScheduler
//
// Specify all #define statements for task scheduler first
#define _TASK_SCHEDULING_OPTIONS
//#define _TASK_TIMECRITICAL
//#define _TASK_SLEEP_ON_IDLE_RUN
#include <TaskScheduler.h>

//
// *** Open Font Render Include ***
// TTF font render support library for microcomputer using Arduino IDE.
// This library can render TTF font files in the SD card or TTF font files 
// embedded in the program.
// https://github.com/takkaO/OpenFontRender
//
#include <OpenFontRender.h>

//
// *** WiFi Include ***
// esp32 Wifi support. Based on WiFi.h from Arduino WiFi shield library.
// Modified by Ivan Grokhotkov, December 2014
//
#include <WiFi.h>

//
// *** WiFi Secure Client Include ***
// Base class that provides Client SSL to ESP32
// Additions Copyright (C) 2017 Evandro Luis Copercini.
//
#include <WiFiClientSecure.h>

//
// *** HTTP Client Include ***
// 2015 Markus Sattler. This file is part of the HTTPClient for Arduino.
// Port to ESP32 by Evandro Luis Copercini (2017) changed fingerprints to CA verification. 	
#include <HTTPClient.h>

/*
** ===================================================================
** Local Includes
** ===================================================================
*/

#include "esp_task_wdt.h"               // FreeRTOS Watchdog Timer

#include "settings.h"                   // Global settings to configure app
#include "Vault.h"                      // Credential management
#include "DisplayUI.h"                  // Base class for the UI
#include "ThingPulse/connectivity.h"    // Connectivity support
#include "SpotifyPlayer.h"              // Spotify Player for all the controls
#include "logTags.h"                    // Tags for logging
#include "SCLogger.h"                   // Logging framework
#include "Monitor.h"                    // Monitoring

#include "ThingPulse/display.h"         // ThingPulse display routines
#include "ThingPulse/util.h"            // ThingPulse utility routines
#include "SCFileIO.h"                   // File IO routines - thread safe
#include "SpotifyArtMgr.h"
#include "UIViews/UIViewManager.h"
#include "scui.h"

/*
** ===================================================================
** Used Fonts
** ===================================================================
*/
#include "fonts/open-sans.h"
#include "fonts/cousine-bold.h"

/*
** ===================================================================
** Local function prototypes
** ===================================================================
*/
void syncTime();                     // syncs the sytem time
void setupLogging();                 // set up logging
void initOpenFontRender();           // initialize the font renderer
void initScheduler();                // Initialize the task scheduler
void registerMonitorDescriptions();  // Initialize the monitor descriptions
void processPerformanceMetrics();    // Show metrics on schedule
void handleTouchInput();             // Check if LCD pressed
void dispatchSCUIQueueMessages();    // Dispatch messages to active view

// Task handles
TaskHandle_t     UITaskHandle;
void             uiHandlerTask(void *pvParameters);
QueueHandle_t    scuiQueue;

unsigned long delayBetweenRequests = 60000; // Time between requests (1 minute)
unsigned long requestDueTime;               // time when request due


bool          bIsInitialPress = false;
bool          bIsInitialLift  = false;

/*
** ===================================================================
** Globals
** ===================================================================
*/
OpenFontRender    ofr;
OpenFontRender    clockFont;
FT6236            ts            = FT6236(TFT_HEIGHT, TFT_WIDTH);     // Touch Controller
TFT_eSPI          tft           = TFT_eSPI();                        // LCD display
DisplayUI         ui            = DisplayUI(&tft, &ofr, &clockFont); // Routines to update UI
SpotifyPlayer&    spotifyPlayer = SpotifyPlayer::getInstance();      // Spotify Player
Vault&            vault         = Vault::getInstance();              // Credential management


// Task Scheduler to use for time sync.  clockTask runs every hour to update
// the 'clock'.
Scheduler      schedule;                                   
Task           clockTask(CLOCK_TASK_INTERVAL_MILLIS, TASK_FOREVER, &syncTime);

// Time management variables
unsigned long  lastTimeSyncMillis = 0;   // last time was synced

/*
** ===================================================================
** setup() - called before the loop.  runs just once.
** ===================================================================
*/

void setup() 
{
    // Initialize serial output
    Serial.begin(115200);
    delay(3000); // Extended due to early logging being clipped or missing

    // Set up logging levels
    setupLogging();
    registerMonitorDescriptions();

    // Initializd DisplayUI instance
    ui.init();

    // Log the banner and memory stats
    logBanner();
    logMemoryStats();

    // Initialize everything
    initTouchScreen(&ts);
    initTft(&tft);
    logDisplayDebugInfo(&tft);
    initOpenFontRender();

    if (!SCFileIO::getInstance().initialize())
    {
        // initialization failed
        ui.setBackground(TFTColor::Yellow, true); 
        ui.cDrawString("FATAL ERROR - Filesystem Not Initialized", 240, 115, 24, TFTColor::Black, ui.getBackground(), "");
        ui.cDrawString("Please verify that the filesystem image", 240, 145, 24, TFTColor::Black, ui.getBackground(), "");
        ui.cDrawString("was uploaded.", 240, 175, 24, TFTColor::Black, ui.getBackground(), "");
        while (true)
        {
            delay(1000); // Sit here indefinitely
        }              
    }

    vault.initialize();

    spotifyPlayer.initialize(&scuiQueue);
    initScheduler();

    // Draw logo and app info to screen
    ui.setBackground(TFTColor::Black);
    ui.drawLogo();
    ui.drawAppInfo();

    // === Start the start up sequence ===

    // Connect WiFi
    ui.drawProgress("Starting WiFi...", 10);
    if (WiFi.status() != WL_CONNECTED) {
        startWiFi();
    }

    // Sync Time
    ui.drawProgress("Synchronizing time...", 40);
    syncTime();

    // Prep to log into Spotify
    ui.drawProgress("Checking Spotify status...", 60);
    if (spotifyPlayer.isRefreshTokenAvailable())
    {
        // token found
    }
    else
    {
        // clear logo
        tft.fillRect(60, 20, tft.width() - 120, 130, TFT_BLACK);  
        // token not found
        ui.drawProgress("Getting Spotify token...", 70);
       
        String msg;
        msg += "From another device\n";
        msg += "on the same network,\n";
        msg += "open a browser at\nhttp://";
        msg += spotifyPlayer.getNodeName();
        msg += ".local";  

        ui.cDrawString(msg.c_str(), ui.getCenterWidth(), 20, 24, TFTColor::Yellow, ui.getBackground(), "");
        spotifyPlayer.requestRefreshToken();

        // clear instructions
        tft.fillRect(0, 20, tft.width(), 300, TFT_BLACK); 
        ui.drawLogo();
    }

    // Log into Spotify
    ui.drawProgress("Logging into Spotify...", 90);
    spotifyPlayer.login();

    vault.eraseEncryptedCredentials();

    // Update to show complete
    ui.drawProgress("Startup completed!", 100);

    delay(1000);

    // Initialize Display Modes
    UIViewManager& dm = UIViewManager::getInstance();
    dm.setDisplayUI(&ui);    

    spotifyPlayer.startBackgroundRefreshes();

    // Create task and queue to process the UI

    // Initialize queue
    scuiQueue = xQueueCreate(10, sizeof(SCUIMessage));
    if (scuiQueue == NULL)
    {
        spLogE(LOGTAG_MULTITASK, "Failed to create scuiQueue!");
    }

    spLogI(LOGTAG_MULTITASK, "creating background task - pinned to core 0");
    xTaskCreatePinnedToCore(
        uiHandlerTask,          // Task function
        "UIHandler",            // Task name
        8192,                   // Stack size
        NULL,                   // Task parameter
        2,                      // Task priority
        &UITaskHandle,          // Task handle
        0                       // Core ID
    );    
    spLogI(LOGTAG_MULTITASK, "background uiHandlerTask task created");    
}

/*
** ===================================================================
** syncTime() - Call back routine to sync the time and keep it 
**              accurate.
** ===================================================================
*/
void syncTime() {
  if (initTime()) 
  {
    lastTimeSyncMillis = millis();
    setTimezone(Vault::getInstance().getTimezone().c_str());
    spLogI(LOGTAG_GENERAL, "Current local time: %s", getCurrentTimestamp(SYSTEM_TIMESTAMP_FORMAT).c_str());
  }
}

/*
** ===================================================================
** setupLogging()
**    Configures the logging levels for various tags using SCLogger.
**
** ===================================================================
*/
void setupLogging()
{

    Serial.printf("Entering setupLogging()\n");
    // Create the logger instance
    SCLogger& logger = SCLogger::getInstance();

    // Set default logging level globally (optional)
    logger.setLogLevel("*", ESP_LOG_WARN); // Suppresses all Informational messages

    // Configure specific logging levels for each tag
    logger.setLogLevel(LOGTAG_INPUT, ESP_LOG_VERBOSE);
    // logger.setLogLevel(LOGTAG_SONG_DATA, ESP_LOG_ERROR);
    logger.setLogLevel(LOGTAG_PLAYER, ESP_LOG_INFO);
    // logger.setLogLevel(LOGTAG_GUI, ESP_LOG_INFO);
    logger.setLogLevel(LOGTAG_GENERAL, ESP_LOG_INFO);
    // logger.setLogLevel(LOGTAG_MULTITASK, ESP_LOG_VERBOSE);
    logger.setLogLevel(LOGTAG_METRICS, ESP_LOG_INFO);
    logger.setLogLevel(LOGTAG_HEAP, ESP_LOG_INFO);
    //logger.setLogLevel(LOGTAG_TRACE, ESP_LOG_INFO);
    logger.setLogLevel(LOGTAG_FILEIO, ESP_LOG_INFO);
    // logger.setLogLevel(LOGTAG_CACHE, ESP_LOG_INFO);
    logger.setLogLevel(LOGTAG_VAULT, ESP_LOG_INFO);    

    // Supress logs for ESP32 components
    logger.setLogLevel("ssl_client", ESP_LOG_NONE);

    // Print log levels to serial for verification
    Serial.printf("Log level for General: %d\n", logger.getLogLevel(LOGTAG_GENERAL));
    Serial.printf("Log level for GUI: %d\n", logger.getLogLevel(LOGTAG_GUI));
    Serial.printf("Log level for Input: %d\n", logger.getLogLevel(LOGTAG_INPUT));
    Serial.printf("Log level for Multitask: %d\n", logger.getLogLevel(LOGTAG_MULTITASK));
    Serial.printf("Log level for Player: %d\n", logger.getLogLevel(LOGTAG_PLAYER));
    Serial.printf("Log level for SongData: %d\n", logger.getLogLevel(LOGTAG_SONG_DATA));
    Serial.printf("Log level for Metrics: %d\n", logger.getLogLevel(LOGTAG_METRICS));
    Serial.printf("Log level for Heap: %d\n", logger.getLogLevel(LOGTAG_HEAP));
    Serial.printf("Log level for Trace: %d\n", logger.getLogLevel(LOGTAG_TRACE));
    Serial.printf("Log level for File IO: %d\n", logger.getLogLevel(LOGTAG_FILEIO));
    Serial.printf("Log level for Cache: %d\n", logger.getLogLevel(LOGTAG_CACHE));
    Serial.printf("Log level for Vault: %d\n", logger.getLogLevel(LOGTAG_VAULT));

    // Example of logging initialization completion
    spLogI(LOGTAG_GENERAL, "Logging levels initialized.");

    //
    // ESP_LOG_NONE      - 0: No log output. Suppresses all log messages for the tag.
    // ESP_LOG_ERROR     - 1: Errors only. Outputs critical errors that might require immediate attention.
    // ESP_LOG_WARN      - 2: Warnings and Errors. Includes non-critical issues and potential problems.
    // ESP_LOG_INFO      - 3: Informational logs. General information about application flow.
    // ESP_LOG_DEBUG     - 4: Debugging logs. Detailed information useful for debugging during development.
    // ESP_LOG_VERBOSE   - 5: All logs. Includes extremely detailed and low-level messages.
    //
    // Note:
    //   Default level for all tags can be set using:
    //     esp_log_level_set("*", <level>);
    //   Individual tags override the default level.    
}

/*
** ===================================================================
** Register Monitor Descriptions
**    This function registers human-readable descriptions for all
**    monitor IDs defined in the application. Call this function
**    during application initialization.
** ===================================================================
*/
void registerMonitorDescriptions()
{
    Monitor::registerDescription(MONITOR_ID_CALCULATE_AVG_BACKGROUND_COLOR, "Calc Color Avg");
    Monitor::registerDescription(MONITOR_ID_SPOTIFY_GET_CURRENTLY_PLAYING,  "Curr Playing REST");
    Monitor::registerDescription(MONITOR_ID_SPOTIFY_IMAGE_HTTP_GET,         "Art http get");
    Monitor::registerDescription(MONITOR_ID_SPOTIFY_IMAGE_FILE_SAVE,        "Art file save");
    Monitor::registerDescription(MONITOR_ID_SPOTIFY_IMAGE_FILE_LOAD,        "Art file load");
    Monitor::registerDescription(MONITOR_ID_SPOTIFY_IMAGE_CACHE_LOAD,       "Art index load");
    Monitor::registerDescription(MONITOR_ID_SPOTIFY_IMAGE_CACHE_SAVE,       "Art index save");
    Monitor::registerDescription(MONITOR_ID_FETCH_ALBUM_ART,                "Total art fetch");
    Monitor::registerDescription(MONITOR_ID_SCUI_QUEUE_DELAY,               "SCUI Queue Delay");
}

/*
** ===================================================================
** initOpenFontRender() 
**
** initialize open font render
** ===================================================================
*/
void initOpenFontRender() 
{
  ofr.loadFont(opensans, sizeof(opensans));
  ofr.setDrawer(tft);
  ofr.setFontColor(TFT_WHITE);
  ofr.setBackgroundColor(TFT_BLACK);

  clockFont.loadFont(cousineBold, sizeof(cousineBold));
  clockFont.setDrawer(tft);
  clockFont.setFontColor(TFT_WHITE);
  clockFont.setBackgroundColor(TFT_BLACK);  
}

/*
** ===================================================================
** initScheduler() 
**
** initialize the scheduler to update the clock
** ===================================================================
*/
void initScheduler() 
{
  // Set the options for the task so that it "catches up" if there is a delay
  clockTask.setSchedulingOption(TASK_SCHEDULE);

  // Initialise the task scheduler and start the tasks
  schedule.init();
  schedule.addTask(clockTask);
  clockTask.enable();
}

/*
** ===================================================================
** loop()
**    The main application loop. This function runs continuously 
**    but is designed to yield execution to FreeRTOS tasks.
**
**    - Delays execution to prevent excessive CPU usage.
**    - Executes scheduled tasks using the TaskScheduler.
**
**    Since FreeRTOS tasks handle most functionality, this loop 
**    primarily serves as a placeholder.
** ===================================================================
*/

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));  // Keep loop from consuming resources    
    schedule.execute();
}

/*
** ===================================================================
** UI Handler Task
**    This task is responsible for managing UI updates and input 
**    processing in a continuous loop. It performs the following:
**
**      - Retrieves the currently active UI view from UIViewManager.
**      - Processes system performance metrics.
**      - Handles touch input events.
**      - Dispatches messages from the SCUI queue.
**      - Delays execution to maintain a 60 FPS refresh rate and 
**        allow other tasks to execute.
**
**    Since vTaskDelay() is used, the Task Watchdog does not require 
**    manual resets.
**
**    This task runs indefinitely.
**
** Parameters:
**    pvParameters - Unused parameter required by FreeRTOS.
** ===================================================================
*/
UIView *pActiveView = nullptr;
void uiHandlerTask(void *pvParameters) 
{
    spLogI(LOGTAG_MULTITASK, "uiHandlerTask task executing.  about to enter loop.");

    while (true) 
    {
        // Feed the Task Watchdog to avoid timeout (not needed with vTaskDelay below)
        // esp_task_wdt_reset();

        processPerformanceMetrics();

        pActiveView = UIViewManager::getInstance().getActiveView(); 

        handleTouchInput();    

        // Reset pActiveView in case it changed after handling input
        // (if you don't do this, messages will go to the wrong
        //  view.)
        pActiveView = UIViewManager::getInstance().getActiveView(); 

        dispatchSCUIQueueMessages();

        // Allow other tasks to execute
        vTaskDelay(pdMS_TO_TICKS(16)); // 60 fps refresh rate    
    }
}

/*
** ===================================================================
** processPerformanceMetrics()
** ===================================================================
*/
void processPerformanceMetrics()
{

    if (millis() > requestDueTime)
    {
        bool heapWarning = !(Monitor::watchHeap(HEAP_LOW_THRESHOLD));
        if (heapWarning)
        {
            // ui.setBackground(TFTColor::SC_LowHeap, true);   
            // char s[50];
            // snprintf(s, sizeof(s), "Low Heap: %u", Monitor::getFreeHeap());
            // ui.drawTextToLCD(s, (ui.getCenterHeight() - 10));
            // vTaskDelay(pdMS_TO_TICKS(3000)); // 3 second delay to keep msg up
        }
        Monitor::dumpStats(LOGTAG_METRICS);
        Monitor::logUptime(LOGTAG_METRICS);
        requestDueTime = millis() + delayBetweenRequests;

    } 

}

/*
** ===================================================================
** handleTouchInput()
** ===================================================================
*/
void handleTouchInput()
{
    static bool isTouchInProgress = false;
    if (ts.touched())
    {
        // track touch has started
        isTouchInProgress = true;

        // x and y are the portrait coordinates.
        // not sure why... 
        TS_Point p = ts.getPoint();

        // flip the cooridnates to be landscape
        uint16_t touchX = p.y;
        uint16_t touchY = tft.height() - p.x;

        p.x = touchX;
        p.y = touchY;

        bIsInitialLift = false;
        if (!bIsInitialPress)
        {

            spLogI(LOGTAG_INPUT, "Initial Press Detected x=%d, y=%d", touchX, touchY);
            bIsInitialPress = true;

            UBaseType_t        messagesInQueue = uxQueueMessagesWaiting(scuiQueue);

            pActiveView->onTouchDown(p);

            // if there was a message in the queue when the press took place, 
            // there is a good chance that the UI is being updated and the onTouchDown()
            // event which happens immediately will act in the middle of something.  Leaving
            // this for now since responding immediately can be a good thing but let's 
            // at least clean up afterwards if something paints on top of the resulting
            // action.  TODO: Consider putting user input on the queue instead of letting it
            // interrupt whatever is going on; however, if this works reasonably well
            // it might be worth leaving as is since it provides a bit of responsiveness.
            if (messagesInQueue > 1)
            {
                spLogI(LOGTAG_GENERAL, "Pending messages in queue when processing touch. count: %u", messagesInQueue);
                SCUIMessage msg;
                msg.type = SCUIMessageType::UM_MARK_DIRTY;
                msg.str  = "";
                msg.num  = true;

                Monitor::start(MONITOR_ID_SCUI_QUEUE_DELAY, LOGTAG_MULTITASK, "handleTouchInput()");
                if (xQueueSend(scuiQueue, &msg, pdMS_TO_TICKS(10)) != pdPASS)
                {
                    spLogE(LOGTAG_PLAYER, "Failed to send SCUI message to queue");
                }                    
            }

        }
    } 
    else
    {
        if (isTouchInProgress)
        {
            bIsInitialPress = false;
            if (!bIsInitialLift)
            {
                bIsInitialLift    = true;
                isTouchInProgress = false;
                pActiveView->onTouchUp();
            } 
        }
    }    
}

/*
** ===================================================================
** dispatchSCUIQueueMessages()
** ===================================================================
*/
void dispatchSCUIQueueMessages()
{ 
    // make static to avoid reallocations 
    static SCUIMessage message;

    // watch the size
    Monitor::watchQueue(scuiQueue, 1);

    if (xQueueReceive(scuiQueue, &message, 0) != pdPASS)
    {
        // Send in IDLE if nothing in the queue
        message.type = SCUIMessageType::UM_IDLE;
        message.str  = "";
        message.num  = 0;
    }
    else
    {
        // Track how long these messages are taking
        Monitor::stop(MONITOR_ID_SCUI_QUEUE_DELAY);
    }
    pActiveView->handleMessage(&message);
}

