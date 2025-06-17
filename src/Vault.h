/*-------------------------------------------------------------------------------------------------
**
** Vault.h
**
**    Definition of the Vault class, which securely manages Spotify and Wi-Fi credentials
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

#pragma once

#include <Arduino.h>
#include <array>

// Max sizes (unencrypted):
// SSID                  - Per 802.11 standard, 32 characters
// WiFi Password         - WPA2/WPA3 maxiumum password length 64 characters
// Spotify Client Id     - 32 characters
// Spotify Client Secret - 40 characters 
// With encryption and base64 encoding, lengths could go as high a 108 characters.
#define VAULT_MAX_CRED_LENGTH 150

// Defines the levels of credential privacy supported by the Vault.
// These levels determine how credentials are retrieved, encrypted,
// and protected during runtime.
enum class VaultPrivacyLevel
{
    None,   // Use plaintext credentials; either from user.ini or hardcoded settings
    Good,   // Use AES-encrypted credentials; not tied to device, reusable across units
    Better  // Use AES-encrypted credentials; tied to device MAC address, non-reusable
};

// Defines credential source
enum class CredentialSource
{
    Hardcoded,   // Plaintext hardcoded values
    INIFile,     // Loaded values from the ini file
};

const char* toString(VaultPrivacyLevel level);
const char* toString(CredentialSource source);

// Vault definition
class Vault
{
public:
    static Vault& getInstance();

    void              setPrivacyLevel(VaultPrivacyLevel level);
    VaultPrivacyLevel getPrivacyLevel();
    CredentialSource  getCredentialSource();

    void              initialize();
    void              eraseEncryptedCredentials();

    String            getSSID();
    String            getWiFiPassword();
    String            getSpotifyClientID();
    String            getSpotifyClientSecret();
    String            getTimezone();
    bool              isUSDateTimeFormattingUsed();

private:
    Vault();
    std::array<uint8_t, 16> getAesKey(bool tiedToDevice);
    String                  encrypt(const char* input, bool tiedToDevice);
    String                  decrypt(const String& encryptedBase64, bool tiedToDevice);
    void                    printEncryptionHints();
    bool                    _useHardcodedValues   = true;
    VaultPrivacyLevel       _privacyLevel         = VaultPrivacyLevel::None;
    bool                    _dateTimeFormatUS     = false;
    std::array<char, VAULT_MAX_CRED_LENGTH>   _ssid{};
    std::array<char, VAULT_MAX_CRED_LENGTH>   _wifiPwd{};
    std::array<char, VAULT_MAX_CRED_LENGTH>   _clientId{};
    std::array<char, VAULT_MAX_CRED_LENGTH>   _clientSecret{};
    std::array<char, VAULT_MAX_CRED_LENGTH>   _timezone{};
    std::array<char, VAULT_MAX_CRED_LENGTH>   _salt{};
};