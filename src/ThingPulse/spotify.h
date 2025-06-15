/*-------------------------------------------------------------------------------------------------
**
** spotify.h
**
**    Utility routines from ThingPulse for Spotify integration. This includes
**    OAuth authentication, certificate setup, and serving a local web
**    interface to acquire an auth code via redirect URI.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2024-12-27 - Electric Diversions - Copied and renamed to tpSpotify.h from spotify.h
**    2025-05-04 - Electric Diversions - Renamed back to spotify.h and moved to ThingPulse folder
**    2025-06-07 - Electric Diversions - Refactor to scope SpotifyArduino instance to SpotifyPlayer
** ------------------------------------------------------------------------------------------------
*/

#pragma once

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <SpotifyArduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include "Vault.h"

extern const char *spotify_server_cert;
extern const char *spotify_image_server_cert;

// Use http://<value-configured-here>.local/callback/ as the redirect URI for the app on Spotify.
// Hence, the default URI is http://tp-spotify.local/callback/.
// If you change the value here, you need to modify the redirect URI on Spotify as well.
#define SPOTIFY_ESPOTIFIER_NODE_NAME "tp-spotify"

#define SPOTIFY_REFRESH_TOKEN_FILE_NAME "/refresh-token.txt"
// the '/callback/' path is essential as spotify.h#fetchSpotifyAuthCode() registers a handler for it
#define SPOTIFY_REDIRECT_URI "http%3A%2F%2F" SPOTIFY_ESPOTIFIER_NODE_NAME ".local%2Fcallback%2F"

String authCode = "";
String scope    = "user-read-playback-state%20user-modify-playback-state";
WebServer server(80);
WiFiClientSecure client;

// Note: SpotifyArduino does not initialize its _refreshToken pointer.
// When declared as a global, this works because globals are zero-initialized by default,
// making _refreshToken safely nullptr. If the instance is created dynamically or locally,
// this assumption can lead to heap corruption when delete is called on an uninitialized
// _refreshToken in setRefreshToken().
SpotifyArduino spotify(client);

const char *webpageTemplate =
    R"(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
  </head>
  <body>
    <div>
     <a href="https://accounts.spotify.com/authorize?client_id=%s&response_type=code&redirect_uri=%s&scope=%s">Click</a> to load Spotify authentication code
    </div>
  </body>
</html>
)";

void handleCallback() {
  log_i("###### handleCallback().");
  String code = "";
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "code") {
      authCode = server.arg(i);
    }
  }

  if (authCode == "") {
    server.send(404, "text/plain", "Failed to fetch Spotify authentication code, check serial monitor. Maybe go back in browser history and try again.");
  } else {
    server.send(200, "text/plain", "Succesfully fetched Spotify authentication code. Follow instructions on device.");
  }
}

void handleFavicon() {
  log_i("*** Entering handleFavicon()");
  server.send(200, "image/vnd.microsoft.icon", "00000100");
}

void handleNotFound() {
  log_i("*** Entering handleNotFound()");
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  log_e("%s", message.c_str());
  server.send(404, "text/plain", message);
}

void printRootWebpage() {
  char webpage[800];
  sprintf(webpage, webpageTemplate, Vault::getInstance().getSpotifyClientID().c_str(), SPOTIFY_REDIRECT_URI, scope.c_str());
  log_i("webpage: '%s",webpage);
}

void handleRoot() {
  log_i("*** Entering handleRoot()");
  char webpage[800];
  sprintf(webpage, webpageTemplate, Vault::getInstance().getSpotifyClientID().c_str(), SPOTIFY_REDIRECT_URI, scope.c_str());
  server.send(200, "text/html", webpage);
}

String fetchSpotifyAuthCode() {
  log_i("*** Entering fetchSpotifyAuthCode()");
  if (MDNS.begin(SPOTIFY_ESPOTIFIER_NODE_NAME)) {
    log_i("MDNS responder started for node name '%s'.", SPOTIFY_ESPOTIFIER_NODE_NAME);
    log_i("Open browser at http://%s.local", SPOTIFY_ESPOTIFIER_NODE_NAME);
  }

  server.on("/", handleRoot);
  server.on("/callback/", handleCallback);
  server.on("/favicon.ico", handleFavicon);
  server.onNotFound(handleNotFound);
  server.begin();
  log_i("HTTP server started");

  while (authCode == "") {
    //log_i("--- calling server.handleClient().");
    server.handleClient();
    yield();
  }

  log_i("Successfully loaded Spotify authentication code: '%s'.", authCode.c_str());

  log_i("Stopping HTTP server");
  server.stop();
  log_i("Stopping MDNS responder");
  MDNS.end();

  return authCode;
}
