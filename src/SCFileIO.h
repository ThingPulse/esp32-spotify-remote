/*-------------------------------------------------------------------------------------------------
**
** SCFileIO.h
**
**    A singleton class to manage file I/O operations safely using semaphores.
**    Wraps LittleFS to provide thread-safe access to the file system, offering
**    methods for reading, writing, and managing files on the ESP32.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-01-12 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/
#pragma once

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

/*
** ===================================================================
** SCFileIO
** ===================================================================
*/
class SCFileIO
{
public:
    // Access the singleton instance
    static SCFileIO& getInstance();

    // Initializes the semaphore. Must be called before using the class.
    bool initialize();

    // Get partition size
    size_t getPartitionSize();

    // Opens a file safely using LittleFS
    File open(const char *path, const char *mode);

    // Writes data to a file safely
    size_t write(File &file, const uint8_t *buffer, size_t size);

    // Reads data from a file safely
    size_t read(File &file, uint8_t *buffer, size_t size);

    // Closes a file safely
    void close(File &file);

    // Deletes a file safely
    bool remove(const char *path);

    // Returns true if a file exists
    bool exists(const char *path);

    // String read/write methods
    String readFsString(const char *path);
    void   saveFsString(const char *path, String string);
    void   listFiles();
    void   hexDump(const char* tag, const char* filePath);

    // Public semaphore for external use if needed
    SemaphoreHandle_t xSemaphoreFileIO;

private:
    // Private constructor and destructor to enforce singleton pattern
    SCFileIO();
    ~SCFileIO();

    // Deleted copy constructor and assignment operator
    SCFileIO(const SCFileIO&) = delete;
    SCFileIO& operator=(const SCFileIO&) = delete;

    // Acquires the semaphore
    bool takeSemaphore();

    // Releases the semaphore
    void giveSemaphore();
};