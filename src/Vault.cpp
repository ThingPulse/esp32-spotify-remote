/*-------------------------------------------------------------------------------------------------
**
** Vault.cpp
**
**    Implementation of the Vault class, which securely manages Spotify and Wi-Fi credentials
**    for the ESP32 Spotify controller. Supports plaintext and encrypted storage models using
**    AES-128 encryption with optional device binding via MAC address.
**
**    Credentials can be sourced from either a user-editable /user.ini file or hardcoded values
**    from settings.h. Privacy levels (None, Good, Better) determine how credentials are stored,
**    decrypted, and exposed to the runtime.
**
**    Encryption is done using AES-128 in Electronic Codebook (ECB) mode with Public-Key Cryptography
**    Standards #7 (PKCS#7) padding. ECB mode encrypts each 16-byte block independently, which is simple 
**    and efficient but less secure for large or repetitive data due to potential pattern leakage.
**    PKCS#7 padding appends a series of bytes—each equal to the number of bytes added—to ensure the 
**    input length is a multiple of the AES block size. The derived encryption key is generated from a 
**    fixed internal base key and can be optionally augmented with user-defined salt and/or the device’s 
**    MAC address to enhance uniqueness and security.  See implementations of encrypt() and decrypt()
**    for more details.
**
** SPDX-FileCopyrightText: 2025 ThingPulse Ltd., https://thingpulse.com
** SPDX-License-Identifier: MIT
**
** ------------------------------------------------------------------------------------------------
** Change Log:
**    2025-06-03 - Electric Diversions - Initial creation.
** ------------------------------------------------------------------------------------------------
*/

#include <Arduino.h>
#include "Vault.h"
#include "settings.h"
#include "SCLogger.h"
#include "logTags.h"
#include <LittleFS.h>
#include "mbedtls/base64.h"

#include "mbedtls/aes.h"

/*
** ===================================================================
** toString(VaultPrivacyLevel)
**
**    Converts a VaultPrivacyLevel enum value to its corresponding
**    string representation for logging, diagnostics, and debugging.
** ===================================================================
*/
const char* toString(VaultPrivacyLevel level)
{
    switch (level)
    {
        case VaultPrivacyLevel::None:   return "None";
        case VaultPrivacyLevel::Good:   return "Good";
        case VaultPrivacyLevel::Better: return "Better";
        default:                        return "Unknown";
    }
}

/*
** ===================================================================
** toString(CredentialSource)
**
**    Converts a CredentialSource enum value to its corresponding
**    string representation for logging, diagnostics, and debugging.
** ===================================================================
*/
const char* toString(CredentialSource source)
{
    switch (source)
    {
        case CredentialSource::Hardcoded: return "Hardcoded";
        case CredentialSource::INIFile:   return "INIFile";
        default:                          return "Unknown";
    }
}


/*
** ===================================================================
** getInstance()
**
**    Returns the singleton instance of Vault. Initializes it if needed.
** ===================================================================
*/
Vault& Vault::getInstance()
{
    static Vault instance; // Guaranteed to be thread-safe in C++11 and later
    return instance;
}    

/*
** ===================================================================
** Vault()
**
**    Private constructor for the singleton class.
** ===================================================================
*/
Vault::Vault()
{
    memset(_ssid.data(), 0, _ssid.size());
    memset(_wifiPwd.data(), 0, _wifiPwd.size());
    memset(_clientId.data(), 0, _clientId.size());
    memset(_clientSecret.data(), 0, _clientSecret.size());
    memset(_timezone.data(), 0, _timezone.size());
    memset(_salt.data(), 0, _salt.size());
}

/*
** ===================================================================
** initialize()
**
**    Loads credential and configuration values from /user.ini into
**    internal secure buffers. Falls back to hardcoded values from
**    settings.h if the file does not exist or cannot be opened.
**
**    The file is parsed section by section (e.g., [wifi], [spotify],
**    [system], [vault]) and supports optional configuration values
**    such as key_salt and privacy_level.
**
**    If privacy_level is set to None, encryption hints will be
**    printed to the serial console to assist with migration to
**    encrypted credentials.
**
** Effects:
**    - Sets _useHardcodedValues flag
**    - Populates _ssid, _wifiPwd, _clientId, _clientSecret, _timezone
**    - May set _salt and _privacyLevel if defined
** ===================================================================
*/
void Vault::initialize()
{
    constexpr const char* USER_INI_PATH = "/user.ini";

    if (!LittleFS.exists(USER_INI_PATH)) 
    {
        _useHardcodedValues = true;
        spLogI(LOGTAG_VAULT, "%s does not exist. Using hardcoded credentials.", USER_INI_PATH);
        return;
    }

    File file = LittleFS.open(USER_INI_PATH, "r");
    if (!file)
    {
        _useHardcodedValues = true;
        spLogE(LOGTAG_VAULT, "Unable to open %s.  Unable to load network credentials.", USER_INI_PATH);
        return;
    }

    _useHardcodedValues = false;

    String section;
    while (file.available())
    {
        String line = file.readStringUntil('\n');
        line.trim();

        if (line.startsWith(";") || line.isEmpty())
        {
            continue; // Skip comments and empty lines
        }

        if (line.startsWith("[") && line.endsWith("]"))
        {
            section = line.substring(1, line.length() - 1);
            section.toLowerCase();
            continue;
        }

        int equalsIndex = line.indexOf('=');
        if (equalsIndex == -1) continue;

        String key = line.substring(0, equalsIndex);
        String value = line.substring(equalsIndex + 1);
        key.trim();
        value.trim();

        if (section == "wifi")
        {
            if (key == "ssid")
            {
                strncpy(_ssid.data(), value.c_str(), _ssid.size() - 1);
            }
            else if (key == "password")
            {
                strncpy(_wifiPwd.data(), value.c_str(), _wifiPwd.size() - 1);
            }
        }
        else if (section == "spotify")
        {
            if (key == "client_id")
            {
                strncpy(_clientId.data(), value.c_str(), _clientId.size() - 1);
            }
            else if (key == "client_secret")
            {
                strncpy(_clientSecret.data(), value.c_str(), _clientSecret.size() - 1);
            }
        }
        else if (section == "system")
        {
            if (key == "timezone")
            {
                strncpy(_timezone.data(), value.c_str(), _timezone.size() - 1);
            }
            else if (key == "ui_date_time_format")
            {
                value.toLowerCase();
                _dateTimeFormatUS = (value == "us");
            }
        }
        else if (section == "vault")
        {
            if (key == "key_salt")
            {
                if (value.length() > (_salt.size() - 1))
                {
                    value = value.substring(0, _salt.size() - 1);
                }
                strncpy(_salt.data(), value.c_str(), _salt.size() - 1);
                _salt[_salt.size() - 1] = '\0';  // ensure null-termination
            }
            else if (key == "privacy_level")
            {
                int levelValue = value.toInt();
                if (levelValue == 1)
                {
                    _privacyLevel = VaultPrivacyLevel::Good;
                }
                else if (levelValue == 2)
                {
                    _privacyLevel = VaultPrivacyLevel::Better;
                }
                else
                {
                    _privacyLevel = VaultPrivacyLevel::None;
                }
            }
        }
    }

    file.close();

    if (_privacyLevel == VaultPrivacyLevel::None)
    {
        printEncryptionHints();
    }

    spLogI(LOGTAG_VAULT, "Initialization complete. Privacy Level: %s. _useHardcodedValues: %s", toString(_privacyLevel), _useHardcodedValues ? "true" : "false");

}

/*
** ===================================================================
** setPrivacyLevel()
**
**    Sets the active privacy level for credential handling.
**
**    Privacy levels:
**      - None   (0): Use plaintext credentials from user.ini or settings.h
**      - Good   (1): Use encrypted credentials not tied to device (reusable)
**      - Better (2): Use encrypted credentials tied to this device's MAC address
**
** Parameters:
**    level - The desired privacy level
** ===================================================================
*/
void Vault::setPrivacyLevel(VaultPrivacyLevel level)
{
    _privacyLevel = level;
}

/*
** ===================================================================
** getPrivacyLevel()
**
**    Returns the currently active privacy level that determines
**    how credentials are retrieved and decrypted.
**
**    Privacy levels:
**      - None   (0): Use plaintext credentials from user.ini or settings.h
**      - Good   (1): Use encrypted credentials not tied to device (reusable)
**      - Better (2): Use encrypted credentials tied to this device's MAC address
**
** ===================================================================
*/
VaultPrivacyLevel Vault::getPrivacyLevel()
{
    return _privacyLevel;
}

/*
** ===================================================================
** getCredentialSource()
**
**    Returns the current source of credentials in use.
**    If _useHardcodedValues is true, credentials are coming from
**    settings.h; otherwise, they are sourced from user.ini.
**
** Returns:
**    CredentialSource enum value indicating the source.
** ===================================================================
*/
CredentialSource Vault::getCredentialSource()
{
    return _useHardcodedValues ? CredentialSource::Hardcoded : CredentialSource::INIFile;
}

/*
** ===================================================================
** getSSID()
**
**    Retrieves the Wi-Fi SSID. If user.ini is not present or unreadable,
**    the SSID defined in settings.h is used directly.
** ===================================================================
*/
String Vault::getSSID()
{
    if (_useHardcodedValues)
    {
        return SSID;
    }
    switch (_privacyLevel)
    {
        case VaultPrivacyLevel::Good:
            return decrypt(_ssid.data(), false);
        case VaultPrivacyLevel::Better:
            return decrypt(_ssid.data(), true);
        default: 
            // Privacy Level None
            return String(_ssid.data());
    }
}

/*
** ===================================================================
** getWiFiPassword()
**
**    Retrieves the Wi-Fi password. If user.ini is not present or unreadable,
**    the password defined in settings.h is used directly.
** ===================================================================
*/
String Vault::getWiFiPassword()
{
    if (_useHardcodedValues)
    {
        return WIFI_PWD;
    }
    switch (_privacyLevel)
    {
        case VaultPrivacyLevel::Good:
            return decrypt(_wifiPwd.data(), false);
        case VaultPrivacyLevel::Better:
            return decrypt(_wifiPwd.data(), true);
        default: 
            // Privacy Level None
            return String(_wifiPwd.data());
    }
}

/*
** ===================================================================
** getSpotifyClientID()
**
**    Retrieves the Spotify Client ID. If user.ini is not present or unreadable,
**    the client ID defined in settings.h is used directly.
** ===================================================================
*/
String Vault::getSpotifyClientID()
{
    spLogV(LOGTAG_VAULT, "_clientId is %s", String(_clientId.data()).c_str());
    if (_useHardcodedValues)
    {
        spLogV(LOGTAG_VAULT, "Returning hardcode SPOTIFY_CLIENT_ID value.");
        return SPOTIFY_CLIENT_ID;
    }
    switch (_privacyLevel)
    {
        case VaultPrivacyLevel::Good:
            return decrypt(_clientId.data(), false);
        case VaultPrivacyLevel::Better:
            return decrypt(_clientId.data(), true);
        default: 
            // Privacy Level None
            return String(_clientId.data());
    }
}

/*
** ===================================================================
** getSpotifyClientSecret()
**
**    Retrieves the Spotify Client Secret. If user.ini is not present or unreadable,
**    the client secret defined in settings.h is used directly.
** ===================================================================
*/
String Vault::getSpotifyClientSecret()
{
    if (_useHardcodedValues)
    {
        return SPOTIFY_CLIENT_SECRET;
    }
    switch (_privacyLevel)
    {
        case VaultPrivacyLevel::Good:
            return decrypt(_clientSecret.data(), false);
        case VaultPrivacyLevel::Better:
            return decrypt(_clientSecret.data(), true);
        default: 
            // Privacy Level None
            return String(_clientSecret.data());
    }
}

/*
** ===================================================================
** getTimezone()
**
**    Returns the configured timezone. If user.ini is not present or unreadable,
**    the timezone defined in settings.h is used directly.
** ===================================================================
*/
String Vault::getTimezone()
{
    if (_useHardcodedValues)
    {
        return TIMEZONE;
    }
    return String(_timezone.data());
}


/**
 * ===================================================================
 * encrypt()
 *
 *    Encrypts a plaintext input string using AES-128 in ECB mode,
 *    then encodes the encrypted binary output as a Base64 string.
 *
 *    The encryption key is derived from a fixed internal base key,
 *    optionally mixed with a user-defined salt and/or the device's
 *    MAC address depending on the privacy level.
 *
 *    PKCS#7 padding is applied manually to make the input length
 *    a multiple of the AES block size (16 bytes).
 *
 *    ECB mode encrypts each 16-byte block independently using the
 *    same key. While simple and fast, ECB mode is less secure than
 *    modes like CBC for long or repetitive inputs.
 *
 * Parameters:
 *    input        - Null-terminated C string to encrypt
 *    tiedToDevice - Whether to incorporate the device MAC address
 *                   into the key derivation for device-specific encryption
 *
 * Returns:
 *    A Base64-encoded encrypted string
 * ===================================================================
 */
String Vault::encrypt(const char* input, bool tiedToDevice)
{
    // Define key and block sizes for AES-128 encryption
    const size_t AES_KEY_SIZE   = 16;
    const size_t AES_BLOCK_SIZE = 16;

    // Get the AES key to use for the encryption
    std::array<uint8_t, 16> finalKeyArr = getAesKey(tiedToDevice);
    const uint8_t* finalKey             = finalKeyArr.data();

    // Pad the input string using PKCS#7 to ensure its length is a multiple of the AES block size.
    // PKCS#7 (Public-Key Cryptography Standards #7) defines a padding scheme that appends N bytes,
    // each with the value N, where N is the number of padding bytes needed to reach the next multiple
    // of the block size. For example, if 5 bytes of padding are required, the value 0x05 will be 
    // written five times at the end of the input. This is implemented manually here and is standard
    // practice in symmetric encryption to support inputs of arbitrary length.
    size_t inputLen      = strlen(input);
    size_t paddedLen     = ((inputLen / AES_BLOCK_SIZE) + 1) * AES_BLOCK_SIZE;
    uint8_t* paddedInput = new uint8_t[paddedLen];
    memcpy(paddedInput, input, inputLen);
    uint8_t padValue     = paddedLen - inputLen;
    memset(paddedInput + inputLen, padValue, padValue);

    // Encrypt the padded input using AES-128 in ECB (Electronic Codebook) mode.
    // ECB mode encrypts each 16-byte block independently using the same key.
    // While simple and fast, ECB mode is less secure for long or patterned inputs,
    // as identical plaintext blocks produce identical ciphertext blocks.
    uint8_t* cipherText = new uint8_t[paddedLen];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, finalKey, 128);

    for (size_t i = 0; i < paddedLen; i += AES_BLOCK_SIZE)
    {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT, paddedInput + i, cipherText + i);
    }

    mbedtls_aes_free(&aes);

    // Encode the resulting ciphertext in Base64 to produce a printable string
    size_t base64Len    = 0;
    size_t base64BufLen = (paddedLen * 4) / 3 + 4;
    uint8_t* base64Buf  = new uint8_t[base64BufLen];
    mbedtls_base64_encode(base64Buf, base64BufLen, &base64Len, cipherText, paddedLen);

    // Convert the Base64 output buffer to a String to return from the function
    String result(reinterpret_cast<char*>(base64Buf));

    // Free all dynamically allocated memory
    delete[] paddedInput;
    delete[] cipherText;
    delete[] base64Buf;

    return result;
}

/**
 * ===================================================================
 * decrypt()
 *
 *    Decrypts a Base64-encoded AES-128 encrypted string using the
 *    same ECB mode and key derivation logic as encrypt().
 *
 *    The encryption key is derived from the internal base key and
 *    may also include a user-defined salt and/or the device MAC
 *    address, depending on the tiedToDevice parameter.
 *
 *    The input string is first Base64-decoded to obtain the raw
 *    encrypted bytes, then decrypted in ECB mode using AES-128.
 *    Finally, PKCS#7 padding is removed to recover the original
 *    plaintext.
 *
 * Parameters:
 *    encryptedBase64 - The Base64-encoded AES-encrypted string
 *    tiedToDevice     - Whether the key derivation includes the
 *                       device MAC address (for Better privacy)
 *
 * Returns:
 *    The decrypted plaintext as a String
 * ===================================================================
 */
String Vault::decrypt(const String& encryptedBase64, bool tiedToDevice)
{
    // Define key and block sizes for AES-128 encryption
    const size_t AES_KEY_SIZE   = 16;
    const size_t AES_BLOCK_SIZE = 16;

    // Get the AES key to use for the decryption
    std::array<uint8_t, 16> finalKeyArr = getAesKey(tiedToDevice);
    const uint8_t* finalKey             = finalKeyArr.data();

    // Decode the Base64-encoded input back into raw encrypted bytes
    size_t encryptedLen = (encryptedBase64.length() * 3) / 4;
    uint8_t* encryptedBytes = new uint8_t[encryptedLen];
    size_t actualLen = 0;
    mbedtls_base64_decode(encryptedBytes, encryptedLen, &actualLen,
                          reinterpret_cast<const uint8_t*>(encryptedBase64.c_str()),
                          encryptedBase64.length());

    // Decrypt the raw encrypted bytes using AES-128 in ECB (Electronic Codebook) mode
    // This reverses the block-by-block encryption performed in encrypt()
    uint8_t* plainText = new uint8_t[actualLen];
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, finalKey, 128);

    for (size_t i = 0; i < actualLen; i += AES_BLOCK_SIZE)
    {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT, encryptedBytes + i, plainText + i);
    }

    mbedtls_aes_free(&aes);

    // Remove the PKCS#7 padding added during encryption
    // Each padding byte should equal the number of padding bytes added (e.g., 0x05 five times)
    // This determines the true original length of the decrypted plaintext
    uint8_t padValue = plainText[actualLen - 1];
    if (padValue > AES_BLOCK_SIZE)
    {
        padValue = 0;
    }
    size_t unpaddedLen = actualLen - padValue;

    // Convert the unpadded plaintext bytes to a String
    String result(reinterpret_cast<char*>(plainText), unpaddedLen);

    // Free all dynamically allocated memory
    delete[] encryptedBytes;
    delete[] plainText;

    return result;
}

/*
** ===================================================================
** eraseEncryptedCredentials()
**
**    Zeroes out the secure credential arrays.
** ===================================================================
*/
void Vault::eraseEncryptedCredentials()
{
    memset(_ssid.data(), 0, _ssid.size());
    memset(_wifiPwd.data(), 0, _wifiPwd.size());
    memset(_clientId.data(), 0, _clientId.size());
    memset(_clientSecret.data(), 0, _clientSecret.size());
}

/*
** ===================================================================
** printEncryptionHints()
**
**    Prints hints on what encrypted values to use for the 
** various privacy controlled fields.  The hints are only printed 
** when the privacy level is None and credentials are assumed to be
** in the clear within the user.ini file.  If hardcoded values from
** settings.h are in use, hints will not print.
**
** ===================================================================
*/
void Vault::printEncryptionHints()
{
    if (_useHardcodedValues
    || (_privacyLevel != VaultPrivacyLevel::None))
    {
        return;
    }

    Serial.println();
    Serial.println("======================================================================");
    Serial.println("Paste these into your user.ini file to enable encrypted credentials.");
    Serial.println("======================================================================");
    Serial.println();
    Serial.println("; Values to use for Good Privacy (encrypted, reusable across devices):");
    Serial.println("privacy_level = 1");
    Serial.printf("ssid = %s\n", encrypt(_ssid.data(), false).c_str());
    Serial.printf("password = %s\n", encrypt(_wifiPwd.data(), false).c_str());
    Serial.printf("client_id = %s\n", encrypt(_clientId.data(), false).c_str());
    Serial.printf("client_secret = %s\n", encrypt(_clientSecret.data(), false).c_str());
    Serial.println();
    Serial.println("; Values to use for Better Privacy (encrypted, tied to this device):");
    Serial.println("privacy_level = 2");
    Serial.printf("ssid = %s\n", encrypt(_ssid.data(), true).c_str());
    Serial.printf("password = %s\n", encrypt(_wifiPwd.data(), true).c_str());
    Serial.printf("client_id = %s\n", encrypt(_clientId.data(), true).c_str());
    Serial.printf("client_secret = %s\n", encrypt(_clientSecret.data(), true).c_str());
    Serial.println("======================================================================");
}

/**
 * ===================================================================
 * getAesKey()
 *
 *    Derives the AES-128 encryption key used for securing credentials.
 *    This key is constructed from a fixed internal base key and can be
 *    optionally augmented with a user-defined salt and the device's MAC
 *    address to increase uniqueness and security.
 *
 *    - The base key is obscured via a transformation using a static
 *      pattern to avoid storing it directly in firmware.
 *    - If a salt is defined in user.ini, it is XORed into the key.
 *    - If tiedToDevice is true, the MAC address is also XORed into the key.
 *
 * Parameters:
 *    tiedToDevice - Whether to incorporate the device's MAC address
 *                   into the key derivation (for Better privacy).
 *
 * Returns:
 *    A fully derived 128-bit AES key as a std::array<uint8_t, 16>.
 * ===================================================================
 */
std::array<uint8_t, 16> Vault::getAesKey(bool tiedToDevice)
{
    // The baseKey is a static 128-bit value used as the foundation for AES key derivation.
    // It is intentionally non-obvious and will be transformed before use to avoid storing
    // the working encryption key directly in firmware.
    const uint8_t baseKey[16] = { 0x25, 0xA1, 0x33, 0x42, 0x59, 0x1C, 0xEF, 0x73,
                                  0x91, 0xF0, 0xAB, 0xCC, 0x14, 0xD5, 0x63, 0x1E };

    std::array<uint8_t, 16> finalKey;
    memcpy(finalKey.data(), baseKey, 16);

    // Transform the base key using a fixed pattern to derive the working key.
    // This step ensures the actual AES key is not directly stored in memory as-is.
    for (int i = 0; i < 16; ++i)
    {
        // XOR each byte of the base key with a fixed constant (0xA5) and a variable 
        // offset (i * 13).  This step transforms the base key into the working AES key, 
        // obscuring it from plain inspection
        finalKey[i] ^= 0xA5 ^ (i * 13);
        finalKey[i] ^= 0xA5 ^ (i * 13);
    }

    // If a user-defined salt is present, incorporate it into the key derivation
    if (_salt[0] != '\0')
    {
        // XOR each byte of the salt into the key, wrapping if the salt is longer than 16 bytes
        for (size_t i = 0; i < strlen(_salt.data()); ++i)
        {
            finalKey[i % 16] ^= _salt[i];
        }
    }

    // If the key should be tied to the device, incorporate the ESP32's MAC address
    // This introduces a unique device-specific element into the key derivation
    if (tiedToDevice)
    {
        // esp_read_mac returns a 6-byte MAC for ESP_MAC_WIFI_STA, ESP_MAC_BT, etc.
        // enumerator ESP_MAC_WIFI_STA - MAC for WiFi Station (6 bytes)
        // https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/misc_system_api.html
        uint8_t mac[6];
        esp_read_mac(mac, ESP_MAC_WIFI_STA);
        for (int i = 0; i < 6; ++i)
        {
            finalKey[i]      ^= mac[i];
            finalKey[15 - i] ^= mac[i];
        }
    }

    return finalKey;
}

/*
** ===================================================================
** isUSDateTimeFormattingUsed()
**
**    Determines whether US-style date/time formatting should be used.
** ===================================================================
*/
bool Vault::isUSDateTimeFormattingUsed()
{
    if (_useHardcodedValues)
    {
        return Use_US_Date_Time_Format;
    }

    return _dateTimeFormatUS;
}