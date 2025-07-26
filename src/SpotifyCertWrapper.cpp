/*
** ===================================================================
** SpotifyCertWrapper.cpp
**
** Purpose:
**    This file is created to resolve multiple definition errors 
**    arising from the inclusion of `SpotifyArduinoCert.h` in 
**    multiple source files. The header file defines symbols 
**    (`spotify_server_cert` and `spotify_image_server_cert`) 
**    directly, which leads to multiple definitions at the linking 
**    stage when the header is included in more than one source file.
**
** Resolution:
**    To avoid this issue, this wrapper file includes 
**    `SpotifyArduinoCert.h` in a single compilation unit. This 
**    ensures that the symbols are defined exactly once in the 
**    compiled output.
**
** Usage:
**    - Include this file in the build system as part of the 
**      project compilation.
**    - In other source files that need access to these symbols, 
**      use `extern` declarations instead of including 
**      `SpotifyArduinoCert.h` directly.
**
** Example:
**    Instead of including `SpotifyArduinoCert.h`:
**        #include "SpotifyArduinoCert.h"
**
**    Use the following `extern` declarations:
**        extern const char *spotify_server_cert;
**        extern const char *spotify_image_server_cert;
**
** Rationale:
**    By limiting the inclusion of `SpotifyArduinoCert.h` to this 
**    file, the project adheres to the "One Definition Rule" (ODR), 
**    a key principle in C++ that ensures symbols are defined 
**    exactly once.
**
** Notes:
**    Failure to follow these guidelines will reintroduce the 
**    multiple definition errors during the linking stage.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-01-11 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include "SpotifyArduinoCert.h"