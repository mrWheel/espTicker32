# Build log for espTicker32ParolaS3

Generated: 2026-02-23T16:22:00

$ pio run -e espTicker32ParolaS3
Warning! Ignore unknown configuration option `program_name` in section [env:espTicker32Parola]
Warning! Ignore unknown configuration option `program_binary` in section [env:espTicker32Parola]
Warning! Ignore unknown configuration option `program_name` in section [env:espTicker32ParolaS3]
Warning! Ignore unknown configuration option `program_binary` in section [env:espTicker32ParolaS3]
Warning! Ignore unknown configuration option `program_name` in section [env:espTicker32Neopixels]
Warning! Ignore unknown configuration option `program_binary` in section [env:espTicker32Neopixels]
Processing espTicker32ParolaS3 (platform: espressif32; board: esp32-s3-devkitm-1; framework: arduino)
--------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/esp32-s3-devkitm-1.html
PLATFORM: Espressif 32 (6.10.0) > Espressif ESP32-S3-DevKitM-1
HARDWARE: ESP32S3 240MHz, 320KB RAM, 8MB Flash
DEBUG: Current (esp-builtin) On-board (esp-builtin) External (cmsis-dap, esp-bridge, esp-prog, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa)
PACKAGES: 
 - framework-arduinoespressif32 @ 3.20017.241212+sha.dcc1105b 
 - tool-esptoolpy @ 1.40501.0 (4.5.1) 
 - toolchain-riscv32-esp @ 8.4.0+2021r2-patch5 
 - toolchain-xtensa-esp32s3 @ 8.4.0+2021r2-patch5
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 45 compatible libraries
Scanning dependencies...
Dependency Graph
|-- WebServer @ 2.0.0
|-- LittleFS @ 2.0.0
|-- WiFiManager @ 2.0.17
|-- ArduinoJson @ 6.21.5
|-- WebSockets @ 2.7.3
|-- Networking @ 1.3.3+sha.23e9555
|-- SPAmanager @ 1.3.2+sha.ecbbde0
|-- FSmanager @ 1.0.0+sha.6bf2357
|-- MD_Parola @ 3.7.5
|-- Adafruit GFX Library @ 1.12.4
|-- Adafruit NeoMatrix @ 1.3.3
|-- Adafruit NeoPixel @ 1.15.4
|-- HTTPClient @ 2.0.0
|-- WiFiClientSecure @ 2.0.0
|-- FS @ 2.0.0
|-- WiFi @ 2.0.0
|-- MD_MAX72XX @ 3.5.1
|-- SPI @ 2.0.0
Building in release mode
Touched data/.touch to force filesystem rebuild.
DEBUG: Registering post-build actions
DEBUG: BUILD_DIR: .pio.nosync/build/espTicker32ParolaS3
DEBUG: PROGNAME: firmware
DEBUG: Adding post-action for filesystem binary: .pio.nosync/build/espTicker32ParolaS3/littlefs.bin
DEBUG: Post-build actions registered successfully
Retrieving maximum program size .pio.nosync/build/espTicker32ParolaS3/firmware.elf
Checking size .pio.nosync/build/espTicker32ParolaS3/firmware.elf
Advanced Memory Usage is available via "PlatformIO Home > Project Inspect"
RAM:   [==        ]  24.0% (used 78724 bytes from 327680 bytes)
Flash: [====      ]  41.2% (used 1295337 bytes from 3145728 bytes)
========================= [SUCCESS] Took 4.17 seconds =========================

Environment          Status    Duration
-------------------  --------  ------------
espTicker32ParolaS3  SUCCESS   00:00:04.168
========================= 1 succeeded in 00:00:04.168 =========================
$ pio run -e espTicker32ParolaS3 -t buildfs
Warning! Ignore unknown configuration option `program_name` in section [env:espTicker32Parola]
Warning! Ignore unknown configuration option `program_binary` in section [env:espTicker32Parola]
Warning! Ignore unknown configuration option `program_name` in section [env:espTicker32ParolaS3]
Warning! Ignore unknown configuration option `program_binary` in section [env:espTicker32ParolaS3]
Warning! Ignore unknown configuration option `program_name` in section [env:espTicker32Neopixels]
Warning! Ignore unknown configuration option `program_binary` in section [env:espTicker32Neopixels]
Processing espTicker32ParolaS3 (platform: espressif32; board: esp32-s3-devkitm-1; framework: arduino)
--------------------------------------------------------------------------------
Verbose mode can be enabled via `-v, --verbose` option
CONFIGURATION: https://docs.platformio.org/page/boards/espressif32/esp32-s3-devkitm-1.html
PLATFORM: Espressif 32 (6.10.0) > Espressif ESP32-S3-DevKitM-1
HARDWARE: ESP32S3 240MHz, 320KB RAM, 8MB Flash
DEBUG: Current (esp-builtin) On-board (esp-builtin) External (cmsis-dap, esp-bridge, esp-prog, iot-bus-jtag, jlink, minimodule, olimex-arm-usb-ocd, olimex-arm-usb-ocd-h, olimex-arm-usb-tiny-h, olimex-jtag-tiny, tumpa)
PACKAGES: 
 - framework-arduinoespressif32 @ 3.20017.241212+sha.dcc1105b 
 - tool-esptoolpy @ 1.40501.0 (4.5.1) 
 - tool-mklittlefs @ 1.203.210628 (2.3) 
 - toolchain-riscv32-esp @ 8.4.0+2021r2-patch5 
 - toolchain-xtensa-esp32s3 @ 8.4.0+2021r2-patch5
LDF: Library Dependency Finder -> https://bit.ly/configure-pio-ldf
LDF Modes: Finder ~ chain, Compatibility ~ soft
Found 45 compatible libraries
Scanning dependencies...
Dependency Graph
|-- WebServer @ 2.0.0
|-- LittleFS @ 2.0.0
|-- WiFiManager @ 2.0.17
|-- ArduinoJson @ 6.21.5
|-- WebSockets @ 2.7.3
|-- Networking @ 1.3.3+sha.23e9555
|-- SPAmanager @ 1.3.2+sha.ecbbde0
|-- FSmanager @ 1.0.0+sha.6bf2357
|-- MD_Parola @ 3.7.5
|-- Adafruit GFX Library @ 1.12.4
|-- Adafruit NeoMatrix @ 1.3.3
|-- Adafruit NeoPixel @ 1.15.4
|-- HTTPClient @ 2.0.0
|-- WiFiClientSecure @ 2.0.0
|-- FS @ 2.0.0
|-- WiFi @ 2.0.0
|-- MD_MAX72XX @ 3.5.1
|-- SPI @ 2.0.0
Building in release mode
Touched data/.touch to force filesystem rebuild.
DEBUG: Registering post-build actions
DEBUG: BUILD_DIR: .pio.nosync/build/espTicker32ParolaS3
DEBUG: PROGNAME: firmware
DEBUG: Adding post-action for filesystem binary: .pio.nosync/build/espTicker32ParolaS3/littlefs.bin
DEBUG: Post-build actions registered successfully
Building FS image from 'data' directory to .pio.nosync/build/espTicker32ParolaS3/littlefs.bin
/weerlive.ini
/favicon.ico
/rssFeeds.ini
/parola.ini
/skipVoetbal.txt
/skipWords.txt
/neopixels.ini
/SYS/FSmanager.js
/SYS/disconnected.html
/SYS/SPAmanager.html
/SYS/FSmanager.css
/SYS/espTicker32.js
/SYS/SPAmanager.js
/SYS/SPAmanager.css
/settings.ini
/localMessages.dat
/.touch
handle_filesystem([".pio.nosync/build/espTicker32ParolaS3/littlefs.bin"], ["data"])
DEBUG: handle_filesystem called
DEBUG: source: [<SCons.Node.FS.Dir object at 0xa50612090>]
DEBUG: target: [<SCons.Node.FS.File object at 0xa50612310>]
DEBUG: program_name: espTicker32
DEBUG: Build directory: .pio.nosync/build/espTicker32ParolaS3
DEBUG: Filesystem type: littlefs
DEBUG: Looking for filesystem binary at: .pio.nosync/build/espTicker32ParolaS3/littlefs.bin
DEBUG: Found filesystem binary: .pio.nosync/build/espTicker32ParolaS3/littlefs.bin
DEBUG: Version string from cpp: v1.3.2
DEBUG: Managing binaries with program_name: espTicker32, version: v1.3.2, postfix: FS
DEBUG: manage_binaries called with source_file: .pio.nosync/build/espTicker32ParolaS3/littlefs.bin, program_name: espTicker32, version: v1.3.2, postfix: FS
DEBUG: Source file found on attempt 1: .pio.nosync/build/espTicker32ParolaS3/littlefs.bin
DEBUG: Binaries directory: binaries
DEBUG: Parsed version: (1, 3, 2)
DEBUG: Found 1 existing versions
DEBUG: Checking existing version: (1, 3, 2) at binaries/espTicker32_FS_v1.3.2.bin
DEBUG: Found existing file with same version: binaries/espTicker32_FS_v1.3.2.bin
DEBUG: Deleted existing file with same version: binaries/espTicker32_FS_v1.3.2.bin
DEBUG: New filename will be: espTicker32_FS_v1.3.2.bin
DEBUG: Destination path: binaries/espTicker32_FS_v1.3.2.bin
DEBUG: Successfully copied .pio.nosync/build/espTicker32ParolaS3/littlefs.bin to binaries/espTicker32_FS_v1.3.2.bin
DEBUG: Keeping 1 most recent versions
DEBUG: Files to keep: ['binaries/espTicker32_FS_v1.3.2.bin']
DEBUG: Files to delete: []
========================= [SUCCESS] Took 1.85 seconds =========================

Environment          Status    Duration
-------------------  --------  ------------
espTicker32ParolaS3  SUCCESS   00:00:01.851
========================= 1 succeeded in 00:00:01.851 =========================
skipping .DS_Store
Using partitions source: /Users/WillemA/Documents/platformioProjects/espTicker32/partitionTables/veryBigFlashPartition.csv

Resolved buildDir: /Users/WillemA/Documents/platformioProjects/espTicker32/.pio.nosync/build/espTicker32ParolaS3
