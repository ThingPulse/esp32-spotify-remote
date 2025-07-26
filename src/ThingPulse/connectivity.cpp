/*-------------------------------------------------------------------------------------------------
**
** connectivity.cpp
**
**    Provides connectivity routines for initializing Wi-Fi with encrypted
**    credentials. Ensures safe access to network parameters and reports
**    connection status for the ESP32 Spotify Remote.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2024-12-26 - Electric Diversions - Copied and renamed to tpConnectivity.h from connectivity.h
**    2024-12-27 - Electric Diversions - Reintroduced connectivity.cpp from separated header.
**    2025-05-04 - Electric Diversions - Moved to ThingPulse folder.
** ------------------------------------------------------------------------------------------------
*/

#include <WiFi.h>

#include "connectivity.h"
#include "ThingPulse/util.h"
#include "../Vault.h"  

/*
** ===================================================================
** startWiFi()
**    Attempt to connect to the configured WiFi network
** ===================================================================
*/
void startWiFi() {

 String ssid = Vault::getInstance().getSSID();

//  log_i("WiFi ssid/pw '%s' '%s'", ssid.c_str(), Vault::getInstance().getWiFiPassword().c_str());

  WiFi.begin(ssid, Vault::getInstance().getWiFiPassword());

  log_i("Connecting to WiFi '%s'...", ssid);
  while (WiFi.status() != WL_CONNECTED) {
    //log_i(".");
    Serial.print(".");
    delay(200);
  }
  log_i("");
  log_i("...done. IP: %s, WiFi RSSI: %d.", WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

