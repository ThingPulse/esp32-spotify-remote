# Tips
- To suppress logging from the [SpotifyArduino library](https://github.com/witnessmenow/spotify-api-arduino/), go to its `SpotifyArduino.h` in `.pio/libdeps/thingpulse-color-kit-grande/SpotifyArduino/src` and commend out the following lines:

```
#define SPOTIFY_DEBUG 1

// Comment out if you want to disable any serial output from this library (also comment out DEBUG and PRINT_JSON_PARSE)
#define SPOTIFY_SERIAL_OUTPUT 1

// Prints the JSON received to serial (only use for debugging as it will be slow)
#define SPOTIFY_PRINT_JSON_PARSE 1
```

- If you have an ESP32 Wrover-B with 8 MB or more of Flash memory, you can expand the album art cache from 10 albums to 60 albums by using these settings in `platformio.ini`.
See the file for additional comments.

```
board = custom_esp-wrover-kit
board_build.partitions = partitions/custom_no_ota.csv
```

- If you want to update your WiFi and Spotify credentials without modifying the source code, use the optional `user.ini` file.
The file is ignored by Git and does not require changing code.
See [full user settings documentation](./UserSettings.md) for details.

# Known Issues
- Some capabilities such as the player controls (stop, start, skip, etc.) require a Spotify Premium subscription and may not work on the ad-supported tier.
See the [Spotify API documentation](https://developer.spotify.com/documentation/web-playback-sdk) for details.

- At startup if nothing is playing, the following may be logged repeatedly:
`20:13:34.148 > [ 15318][E][ssl_client.cpp:37] _handle_error(): [data_to_read():361]: (-76) UNKNOWN ERROR CODE (004C)`
While there is no currently active device or a device has been stopped for a period of time, playback controls may not work and these errors may be seen in the logs.
Once music is started/resumed on the active device, the errors will go away.

- When playback operations are performed, the SpotifyArduino library may log the following.
It does not appear to affect the operation from actually working.

```
23:24:36.283 > [ 67244][V][ssl_client.cpp:369] send_ssl_data(): Writing HTTP request with 0 bytes...
23:24:36.440 > [ 67407][V][ssl_client.cpp:381] send_ssl_data(): Handling error -80
23:24:36.446 > [ 67408][E][ssl_client.cpp:37] _handle_error(): [send_ssl_data():382]: (-80) UNKNOWN ERROR CODE (0050)
23:24:36.452 > [ 67412][V][ssl_client.cpp:321] stop_ssl_socket(): Cleaning SSL connection.
23:24:36.457 > Failed to send request
```
