# Color Kit Grande Spotify Controller User Settings

In order for this project to function, it needs to be able to connect to a WiFi network and authenticate with Spotify. This document explains how to configure it to do these things. 

These values must be set before the device can connect to your network or Spotify account.

---

## Configuration Options

You can configure the settings using one of the following methods:

### 1. **Edit the `settings.h` file (Quick Start)**

Open the `src/settings.h` file and look for the **User Settings** section at the top. This contains the fields for Wi-Fi and Spotify credentials, as well as your timezone.

This is the fastest way to get started. However, values in `settings.h` will be compiled into the firmware and are not encrypted. They will also be tracked by git unless you modify `.gitignore`.  If your system backs up files and/or you copy the files to network locations, the credentials may be visible in clear text by others.  To avoid these limitations, this project uses an optional `user.ini` file discussed in the next section.

### 2. **Use a `user.ini` File (Recommended for Privacy and Flexibility)**

A `user.ini.template` file is provided in the `/data` folder. You can copy it and rename the copy to `user.ini`, then update the values inside. This file will be uploaded to the device's filesystem using the PlatformIO “Upload Filesystem Image” task.  It has already been added to `.gitignore` and will not be tracked by git by default. **NOTE: If `user.ini` is found and loaded, the conflicting values in `settings.h` will be ignored and not used.**

The template includes guidance on privacy levels and where to enter your credentials.

---

## Required Settings

| Section   | Key              | Description                                                               |
|-----------|------------------|---------------------------------------------------------------------------|
| `[vault]` | `privacy_level`  | `0` = None (plaintext), `1` = Good (encrypted, portable), `2` = Better (encrypted, device-tied) |
| `[wifi]`  | `ssid`           | Your Wi-Fi network name                                                   |
|           | `password`       | Your Wi-Fi password                                                       |
| `[spotify]` | `client_id`    | Your Spotify Developer App Client ID                                     |
|           | `client_secret`  | Your Spotify Developer App Client Secret                                 |
| `[system]` | `timezone`      | POSIX timezone format string (see example or link below)                 |
|             | `ui_date_time_format` | Optional: `US` for 12-hour clock and month-first date format; omit or use other values for international formatting |

> 🔗 Refer to the [POSIX timezone list](https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv) for formatting guidance.


## Understanding Privacy Levels

The `privacy_level` setting controls how credentials are stored and handled by the device. It directly affects whether the values in your `user.ini` file are used in plaintext or must be encrypted before use.

### Level 0 – None (plaintext)
- Credentials in `user.ini` are stored and used in clear text.
- This is the easiest option for beginners and debugging.
- **At startup, the device will print encrypted alternatives to the serial console, which can be copy-pasted into the file if you choose to increase privacy later.**

```text
00:30:06.427 > ======================================================================
00:30:06.432 > Paste these into your user.ini file to enable encrypted credentials.
00:30:06.438 > ======================================================================
00:30:06.443 > 
00:30:06.443 > ; Values to use for Good Privacy (encrypted, reusable across devices):
00:30:06.449 > privacy_level = 1
00:30:06.449 > ssid = kfHankF998h4coKUwRMj6w==
00:30:06.454 > password = sFnXUDMIsmci6KV5mlh2rw==
00:30:06.454 > client_id = NMwdXOJ8JNvkekePuwow53nHnlcbJqNKZuQSnASx/DuJBxChGn+u7rVSwVf0sf34
00:30:06.466 > client_secret = NMwdXOJ8JNvkekePuwow53nHnlcbJqNKZuQSnASx/DuJBxChGn+u7rVSwVf0sf34
00:30:06.471 > 
00:30:06.471 > ; Values to use for Better Privacy (encrypted, tied to this device):
00:30:06.476 > privacy_level = 2
00:30:06.476 > ssid = bzvfcf5V5qpzfd251wqlDQ==
00:30:06.482 > password = /PybaE2diyJwdlH4kZC8bQ==
00:30:06.482 > client_id = yub/jzKomMIza3IP8OAKDB04QHNvqLLtDfwWTcy/WpIg8PahhpE5gXVMYO65677j
00:30:06.493 > client_secret = yub/jzKomMIza3IP8OAKDB04QHNvqLLtDfwWTcy/WpIg8PahhpE5gXVMYO65677j
```

### Level 1 – Good (encrypted, reusable across devices)
- Encrypted credentials are required in `user.ini`.
- Encryption uses AES-128 in ECB mode with PKCS#7 padding.
- A static internal base key is used to encrypt and decrypt values.
- Encrypted values can be reused on any compatible device.

### Level 2 – Better (encrypted, tied to device)
- Same encryption method as Level 1, but the AES key is derived from both the internal base key and the device's MAC address.
- Encrypted values can only be decrypted by the device that created them.
- Provides stronger security, but values must be regenerated for each device.

Good and Better levels support an optional `key_salt` setting in the `[vault]` section to increase key complexity and further personalize the encryption key derivation.

---

## Example `user.ini`

```ini
;
; User settings for Spotify Controller.  Make sure to Upload Filesystem
; Image after updating.
;
; ----------------------------------------------------------------------
; Privacy Levels:
;    0 - None (credentials in clear text)
;    1 - Good (encrypted, reusable across devices)
;    2 - Better (encrypted, tied to this device)
; ----------------------------------------------------------------------
[vault]
privacy_level = 0
; ----------------------------------------------------------------------
; WiFi:
; ----------------------------------------------------------------------
[wifi]
ssid = MyNetworkName
password = MyPassword123
; ----------------------------------------------------------------------
; Spotify:
; ----------------------------------------------------------------------
[spotify]
client_id = abcdefghijklmnopqrstuv1234567890
client_secret = abcdefghijklmnopqrstuv1234567890
; ----------------------------------------------------------------------
; System Settings:
; Timezone format - see
;      https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
; For US date and time formatting, include:
; ui_date_time_format = US
; ----------------------------------------------------------------------
[system]
timezone = CST6CDT,M3.2.0,M11.1.0
ui_date_time_format = US