// SPDX-FileCopyrightText: 2023 ThingPulse Ltd., https://thingpulse.com
// SPDX-License-Identifier: MIT

#pragma once

#include <LittleFS.h>

void listFiles();

void initFileSystem() {
  if (LittleFS.begin()) {
    log_i("Flash FS available!");
  } else {
    log_e("Flash FS initialisation failed!");
  }

  listFiles();
}

void listFiles() {
  log_i("Flash FS files found:");

  File root = LittleFS.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      break;
    }
    log_i("- %s, %d bytes", entry.name(), entry.size());
    entry.close();
  }
}

String readFsString(const char *path) {
  String token = "";
  log_i("Loading string from '%s'.", path);
  File f = LittleFS.open(path, "r");
  if (f) {
    token = f.readString();
    log_d("Persisted string: %s", token.c_str());
    f.close();
  } else {
    log_e("Failed to load string from file system, returning empty.");
  }
  return token;
}

void saveFsString(const char *path, String string) {
  log_i("Saving string to '%s'.", path);
  File f = LittleFS.open(path, "w+");
  if (f) {
    f.print(string);
    f.close();
  } else {
    log_e("Failed to open file.");
  }
}
