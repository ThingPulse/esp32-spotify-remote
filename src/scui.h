/*-------------------------------------------------------------------------------------------------
**
** scui.h
**
**    Defines Spotify Companion User Interface (SCUI) messaging. This includes
**    message types and structures used for asynchronous, thread-safe
**    communication between tasks via FreeRTOS queues.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-02-08 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#ifndef SCUI_H
#define SCUI_H

#include <Arduino.h>
#include <queue>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Enum for supported message types
enum class SCUIMessageType
{
    UM_STATUS_BOX,
    UM_DOWNLOAD_BOX,
    UM_MARK_DIRTY,
    UM_IDLE,
    UM_PLAYER_REFRESH
};

// Structure for a UI message
struct SCUIMessage
{
    SCUIMessageType type;
    String str; // Generic data payload (e.g., track name, status update)
    int    num; // Generic payload
};

#endif // SCUI_H

