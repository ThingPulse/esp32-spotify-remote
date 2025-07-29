# Color Kit Grande Spotify Controller

[![Build Status](https://github.com/ThingPulse/esp32-spotify-remote/actions/workflows/main.yml/badge.svg)](https://github.com/ThingPulse/esp32-spotify-remote/actions)

Spotify controller application for the [ThingPulse Color Kit Grande](https://thingpulse.com/product/esp32-wifi-color-display-kit-grande/).

<!-- Main image -->
<a href="./images/HomeView.jpg">
  <img src="./images/HomeView.jpg" alt="HomeView UI Example" width="600"/>
</a>

<!-- Thumbnails -->
<p>
  <a href="./images/CoverArtView.jpg">
    <img src="./images/CoverArtView.jpg" alt="Cover View" width="200"/>
  </a>
  <a href="./images/ClockView.jpg">
    <img src="./images/ClockView.jpg" alt="Clock View" width="200"/>
  </a>
  <a href="./images/DiagnosticView.jpg">
    <img src="./images/DiagnosticView.jpg" alt="Diagnostic View" width="200"/>
  </a>
</p>

## Purpose of this project

Using the ThingPulse Spotify Controller you control a Spotify player (phone, browser, etc) from an Espressif ESP32 microcontroller.
Album artwork as well as title and artist name are loaded from the Spotify Web API over WiFi and displayed on a color TFT touch-screen.
The currently playing song can be paused, resumed and skipped to the next or previous song in the playlist.

A full OAuth 2.0 web flow is used to acquire the necessary access and refresh tokens to permit the user to control the player.
In order to run this project on your device, you will need to setup an application on your Spotify dashboard (instructions below).

 ## Features

- **Spotify Playback Control**
  - Play, pause, skip to next/previous track from the touch screen. ⚠️ Spotify requires a premium account for this!
  - Control playback on any active Spotify Connect device linked to your account (e.g., phone, browser, smart speaker)

- **Album Art Display**
  - Downloads and displays album artwork via Spotify Web API
  - Syncs background color with album art
  - Caches artwork locally for performance

- **Multiple UI Modes**
  - Home view with track metadata and album art
  - Cover Art only view
  - Clock view with time and playback progress
  - Supports 12-hour and 24-hour time formats
  - Diagnostics view with system stats and Spotify state

- **OAuth 2.0 Authorization Flow**
  - Authentication and authorization (OAuth 2.0 flow) on device

- **Designed for ESP32 + TFT Touch**
  - Built using PlatformIO and Arduino
  - Touch event handling for UI buttons and screen navigation
  - Takes advantage of ESP32 dual-core architecture: UI logic runs on one core and background song and album art refreshing runs on the other

- **Extensible Design**
  - Modular architecture allows easy addition of new Views (UI screens)
  - Built-in monitoring tools for measuring system performance and UI responsiveness
  - Structured and tag-based logging system for easier debugging and analysis
  - Optional `user.ini` file for easy credential and time zone management


<!--Design Context Diagram -->
<a href="./documentation/SCDesign.jpg">
  <img src="./documentation/SCDesign.jpg" alt="Design" width="600"/>
</a>

## Using the Spotify Controller

- Tap the **Prev**, **Pause/Play**, and **Next** buttons to control music playback.
- Tap the **album art** on the Home view to switch to the **Cover Art view**.
- Tap the **clock** to switch to the **Clock view**.
- Tap the **network status box** in the lower-right corner to open the **Diagnostics view**.
- Tap the top-left corner for **Prev**, the top-center for **Pause/Play**, and the top-right for **Next** when using views other than Home.

> For detailed display logic and diagnostics layout, see `DiagnosticsView.cpp`.

## Service level promise

<table><tr><td><img src="https://thingpulse.com/assets/ThingPulse-open-source-community.png" width="150">
</td><td>This is a ThingPulse <em>community</em> project. See our <a href="https://thingpulse.com/about/open-source-commitment/">open-source commitment declaration</a> for what this means.</td></tr></table>

## Setup instructions

### Precondition

The below instructions assume a properly configured Visual Studio Code installation with PlatformIO.
See our [instructions](https://docs.thingpulse.com/guides/esp32-color-kit-grande/#development-environment) if you need help with this.

### Get access to the Spotify API

1. Go to [https://developer.spotify.com/dashboard/login](https://developer.spotify.com/dashboard/login) and login to or sign up for the Spotify Developer Dashboard

2. Select "Create app"

   <img src="./images/SpotifyDashboard.png" width="400">

3. Fill out the form. Give your new app a name you can attribute to this project.
It's safe to select "I don't know" for the type of application.
Add "http://tp-spotify.local/callback/" to the Redirect URIs section.

   **NOTE** If you are running more than ThingPulse Spotify Remote in the same WiFi network, you should choose a unique name rather than "tp-spotify". Regardless of what you choose it has to reflect what you set for `SPOTIFY_ESPOTIFIER_NODE_NAME` in `spotify.h` in the project.

   <img src="./images/SpotifyCreateApp.png" width="400">

   **Don't forget to save your settings.**

4. Set the unique Client ID and Client Secret as values for the respective variables in `src/spotify.h`.

   <img src="./images/SpotifyClientId.png" width="400">

### Filesystem setup

5. Upload the file system to the device

- Hit the PlatformIO icon on the navigation bar on the left side (alien face).

- Select the Platform > Upload Filesystem Image task.  Unless you later erase the flash or modify certain files, you only need to do this once if it succeeds. Pay attention to the output in the VS Code console that opens. If it reports any errors like e.g. if it cannot connect to the board or if stops midway, close VS Code completely, restart it, and then repeat the process.

 <img src="./images/platformio-filesystem.png" width="400">

- If startup fails with the following message displayed: "FATAL ERROR - Filesystem Not Initialized", this step was not successful or done.

### User settings

6. The fastest way to get up and running is to open the `src/settings.h` file and adjust the handful of configuration parameters in the "User settings" section at the top. They are all documented inside the file directly. Everything should be self-explanatory.  The spotify settings were updated in step 4 above.

 <img src="./images/UserSettings.png" width="400">

See [full user settings documentation](./documentation/UserSettings.md) for details about all available fields, encryption options, and using `user.ini`.

 ### Upload code to device

 7. Select the General > Upload and Monitor task. You do this every time you change code or settings.h.

  <img src="./images/platformio-task-upload.png" width="400">

See [instructions](https://docs.thingpulse.com/guides/esp32-color-kit-grande/#development-environment) if you encounter problems and need Trouble Shooting tips.

## Tips and Known Issues

To see a list of tips and known issues, see [Tips and Known Issues](./documentation/TipsAndKnownIssues.md).
