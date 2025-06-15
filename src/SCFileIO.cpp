/*-------------------------------------------------------------------------------------------------
**
** SCFileIO.cpp
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

#include "SCFileIO.h"
#include "SCLogger.h"
#include "logTags.h"

/*
** ===================================================================
** getInstance()
**    Returns the singleton instance of the SCFileIO class.
**
** ===================================================================
*/
SCFileIO& SCFileIO::getInstance()
{
    static SCFileIO instance;
    return instance;
}

/*
** ===================================================================
** SCFileIO Constructor
** ===================================================================
*/
SCFileIO::SCFileIO()
{
    xSemaphoreFileIO = nullptr;
}

/*
** ===================================================================
** SCFileIO Destructor
** ===================================================================
*/
SCFileIO::~SCFileIO()
{
    if (xSemaphoreFileIO)
    {
        vSemaphoreDelete(xSemaphoreFileIO);
    }
}

/*
** ===================================================================
** initialize()
**    Initializes the semaphore and the LittleFS file system.
**    Lists all files in the file system after initialization.
**
** ===================================================================
*/
bool SCFileIO::initialize()
{
    if (!xSemaphoreFileIO)
    {
        xSemaphoreFileIO = xSemaphoreCreateMutex();
        if (!xSemaphoreFileIO)
        {
            spLogE(LOGTAG_FILEIO, "Failed to create semaphore!");
            return false;
        }
    }

    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for initialize().");
        return false;
    }

    if (LittleFS.begin())
    {
        spLogI(LOGTAG_FILEIO, "Flash FS available!");
        giveSemaphore();
        listFiles();
        return true;
    }
    else
    {
        giveSemaphore();
        spLogE(LOGTAG_FILEIO, "Flash FS initialization failed!");
        return false;
    }

}

/*
** ===================================================================
** getPartitionSize()
**    Returns the total size in bytes of the mounted LittleFS partition.
**
** Returns:
**    Size of the partition in bytes. Returns 0 if the value can't be retrieved
**    or the semaphore acquisition fails.
** ===================================================================
*/
size_t SCFileIO::getPartitionSize()
{
    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for getPartitionSize().");
        return 0;
    }

    size_t partitionSize = LittleFS.totalBytes();  // No need for FSInfo

    giveSemaphore();
    return partitionSize;
}

/*
** ===================================================================
** open()
**    Opens a file safely using LittleFS.
**
** Parameters:
**    path - Path to the file
**    mode - File mode (e.g., "r", "w", etc.)
**
** Returns:
**    File object, or an invalid file if the operation fails.
** ===================================================================
*/
File SCFileIO::open(const char *path, const char *mode)
{
    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for open().");
        return File();
    }

    File file = LittleFS.open(path, mode);
    giveSemaphore();

    if (!file)
    {
        spLogE(LOGTAG_FILEIO, "Failed to open file: %s", path);
    }
    return file;
}

/*
** ===================================================================
** write()
**    Writes data to a file safely.
**
** Parameters:
**    file   - File object
**    buffer - Data buffer
**    size   - Size of data to write
**
** Returns:
**    Number of bytes written
** ===================================================================
*/
size_t SCFileIO::write(File &file, const uint8_t *buffer, size_t size)
{
    if (!file)
    {
        spLogE(LOGTAG_FILEIO, "Invalid file object.");
        return 0;
    }

    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for write().");
        return 0;
    }

    size_t bytesWritten = file.write(buffer, size);
    giveSemaphore();
    return bytesWritten;
}

/*
** ===================================================================
** read()
**    Reads data from a file safely.
**
** Parameters:
**    file   - File object
**    buffer - Buffer to read data into
**    size   - Size of data to read
**
** Returns:
**    Number of bytes read
** ===================================================================
*/
size_t SCFileIO::read(File &file, uint8_t *buffer, size_t size)
{
    if (!file)
    {
        spLogE(LOGTAG_FILEIO, "Invalid file object.");
        return 0;
    }

    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for read().");
        return 0;
    }

    size_t bytesRead = file.read(buffer, size);
    giveSemaphore();
    return bytesRead;
}

/*
** ===================================================================
** close()
**    Closes a file safely.
**
** Parameters:
**    file - File object
** ===================================================================
*/
void SCFileIO::close(File &file)
{
    if (!file)
    {
        spLogE(LOGTAG_FILEIO, "Invalid file object.");
        return;
    }

    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for close().");
        return;
    }

    file.close();
    giveSemaphore();
}

/*
** ===================================================================
** remove()
**    Deletes a file safely.
**
** Parameters:
**    path - Path to the file
**
** Returns:
**    True if the file was deleted, false otherwise
** ===================================================================
*/
bool SCFileIO::remove(const char *path)
{
    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for remove().");
        return false;
    }

    bool result = LittleFS.remove(path);
    giveSemaphore();
    return result;
}

/*
** ===================================================================
** exists()
**    Checks if a file exists.
**
** Parameters:
**    path - Path to the file
**
** Returns:
**    True if the file exists, false otherwise
** ===================================================================
*/
bool SCFileIO::exists(const char *path)
{
    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for exists().");
        return false;
    }

    bool result = LittleFS.exists(path);
    giveSemaphore();
    return result;
}

/*
** ===================================================================
** listFiles()
**    Lists all files in the LittleFS file system.
** ===================================================================
*/
void SCFileIO::listFiles()
{
    spLogI(LOGTAG_FILEIO, "Flash FS files found:");

    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for listFiles().");
        return;
    }

    File root = LittleFS.open("/");
    while (true)
    {
        File entry = root.openNextFile();
        if (!entry)
        {
            break;
        }
        spLogI(LOGTAG_FILEIO, "- %s, %d bytes", entry.name(), entry.size());
        entry.close();
    }

    giveSemaphore();
}

/*
** ===================================================================
** readFsString()
**    Reads a string from the specified file path in LittleFS.
**
** Parameters:
**    path - The file path to read from.
**
** Returns:
**    String - The string read from the file. Empty if read fails.
** ===================================================================
*/
String SCFileIO::readFsString(const char *path)
{
    String token = "";
    spLogI(LOGTAG_FILEIO, "Loading string from '%s'.", path);

    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for readFsString().");
        return token;
    }

    File f = LittleFS.open(path, "r");
    if (f)
    {
        token = f.readString();
        spLogD(LOGTAG_FILEIO, "Persisted string: %s", token.c_str());
        f.close();
    }
    else
    {
        spLogE(LOGTAG_FILEIO, "Failed to load string from file system, returning empty.");
    }

    giveSemaphore();
    return token;
}

/*
** ===================================================================
** saveFsString()
**    Saves a string to the specified file path in LittleFS.
**
** Parameters:
**    path   - The file path to save to.
**    string - The string to save.
** ===================================================================
*/
void SCFileIO::saveFsString(const char *path, String string)
{
    spLogI(LOGTAG_FILEIO, "Saving string to '%s'.", path);

    if (!takeSemaphore())
    {
        spLogE(LOGTAG_FILEIO, "Failed to acquire semaphore for saveFsString().");
        return;
    }

    File f = LittleFS.open(path, "w+");
    if (f)
    {
        f.print(string);
        f.close();
    }
    else
    {
        spLogE(LOGTAG_FILEIO, "Failed to open file.");
    }

    giveSemaphore();
}


/*
** ===================================================================
** takeSemaphore()
**    Acquires the semaphore for file I/O operations.
**
** Returns:
**    True if the semaphore was successfully taken, false otherwise
** ===================================================================
*/
bool SCFileIO::takeSemaphore()
{
    return xSemaphoreTake(xSemaphoreFileIO, portMAX_DELAY) == pdTRUE;
}

/*
** ===================================================================
** giveSemaphore()
**    Releases the semaphore for file I/O operations.
** ===================================================================
*/
void SCFileIO::giveSemaphore()
{
    xSemaphoreGive(xSemaphoreFileIO);
}

/*
** ===================================================================
** hexDump()
**
**    Outputs a formatted hexadecimal and ASCII dump of a file's contents
**    to the serial console for inspection. The dump includes offsets, hex
**    values, and printable ASCII characters. Intended for debugging file
**    contents stored in LittleFS.
**
** Parameters:
**    tag      - Logging tag used for context and log filtering.
**    filePath - Path to the file to be dumped.
**
** Notes:
**    - Only runs if the logger's verbosity level for the tag is set to verbose.
**    - Skips execution if the file cannot be opened or is empty.
** ===================================================================
*/
void SCFileIO::hexDump(const char* tag, const char* filePath)
{

    spLogV(tag, "attempting hexDump in SCFileIO::hexDump()... ");
    // Check if logging level is at least verbose
    if (SCLogger::getInstance().getLogLevel(tag) < ESP_LOG_VERBOSE)
    {
        return; // Skip if the log level is less than verbose
    }

    // Open the file
    File file = SCFileIO::getInstance().open(filePath, "r");
    if (!file || file.size() == 0)
    {
        spLogE(tag, "Failed to open file: %s or file is empty.", filePath);
        return;
    }

    // Start hex dump
    Serial.printf("Hex Dump of file: %s\n", filePath);
    Serial.printf("-----------------------------------------------------------------------------\n");

    uint8_t buffer[16];       // Buffer to hold 16 bytes per line
    size_t  fileOffset = 0;   // Tracks current offset in the file
    char    hexLine[50];      // 16 bytes * 3 chars (hex) + 1 space after 8th byte + null terminator
    char    asciiLine[17];    // 16 bytes + null terminator

    while (file.available())
    {
        size_t bytesRead = file.read(buffer, sizeof(buffer)); // Read up to 16 bytes
        memset(hexLine, ' ', sizeof(hexLine));                // Initialize with spaces for alignment
        memset(asciiLine, '\0', sizeof(asciiLine));           // Null-terminate ASCII buffer

        for (size_t i = 0; i < bytesRead; ++i)
        {
            // Append hex value to the line
            // snprintf(hexLine + (i * 3) + (i >= 8 ? 1 : 0), 4, "%02X ", buffer[i]);
            snprintf(hexLine + (i * 3), 4, "%02X ", buffer[i]);

            // Append ASCII representation to the line
            asciiLine[i] = isprint(buffer[i]) ? buffer[i] : '.';
        }

        // Log the formatted hex dump line
        Serial.printf("%08X  %s |%s|\n", fileOffset, hexLine, asciiLine);

        fileOffset += bytesRead; // Increment file offset
    }

    Serial.printf("-----------------------------------------------------------------------------\n");
    file.close();
}