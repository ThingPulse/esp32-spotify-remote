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
#include "../settings.h"
#include "ThingPulse/util.h"  

/*
** ===================================================================
** WiFi settings
** ===================================================================
*/
const char *SSIDEnc      = "SSID Goes Here";
const char *WIFI_PWD_Enc = "WiFi Password Goes Here";


// Updated to use encrypted network credentials
void startWiFi() {
  static const char k[] = "42";
  char decryptedSSID[50]; 
  char decryptedWIFI_PWD[50];
  simpleDecrypt(SSIDEnc, k, decryptedSSID);
  simpleDecrypt(WIFI_PWD_Enc, k , decryptedWIFI_PWD);
  WiFi.begin(decryptedSSID, decryptedWIFI_PWD);
  log_i("Connecting to WiFi '%s'...", decryptedSSID);
  while (WiFi.status() != WL_CONNECTED) {
    //log_i(".");
    Serial.print(".");
    delay(200);
  }
  log_i("");
  log_i("...done. IP: %s, WiFi RSSI: %d.", WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

