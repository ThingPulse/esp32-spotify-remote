// SPDX-FileCopyrightText: 2023 ThingPulse Ltd., https://thingpulse.com
// SPDX-License-Identifier: MIT

#pragma once

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <SpotifyArduino.h>
#include <SpotifyArduinoCert.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include "settings.h"

String authCode = "";
String scope = "user-read-playback-state%20user-modify-playback-state";
WebServer server(80);
WiFiClientSecure client;
SpotifyArduino spotify(client, SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET);

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

void initSpotifiy() {
  client.setCACert(spotify_server_cert);
}

void handleCallback() {
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
  server.send(200, "image/vnd.microsoft.icon", "00000100");
}

void handleNotFound() {
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

void handleRoot() {
  char webpage[800];
  sprintf(webpage, webpageTemplate, SPOTIFY_CLIENT_ID, SPOTIFY_REDIRECT_URI, scope.c_str());
  server.send(200, "text/html", webpage);
}

String fetchSpotifyAuthCode() {
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
