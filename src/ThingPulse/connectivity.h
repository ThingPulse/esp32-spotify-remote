/*-------------------------------------------------------------------------------------------------
**
** connectivity.h
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


#pragma once

// Start the WiFi
void startWiFi();