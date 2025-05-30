# espTicker32

A sophisticated ESP32-based ticker display system that supports both LED matrix displays (via MD_Parola) and NeoPixel matrix displays. The system features a comprehensive web interface for configuration, multiple content sources including RSS feeds and weather data, and extensive customization options.

See <a href="https://willem.aandewiel.nl/index.php/2020/06/09/an-esp8266-ticker/" target="_blank">this post</a> for more information about the original ESP8266 version.

As the name suggests, this version is designed for ESP32 and uses three core libraries:
* [esp-networking](https://github.com/mrWheel/esp-networking) - WiFi and NTP management
* [esp-FSmanager](https://github.com/mrWheel/esp-FSmanager) - File system management
* [SPAmanager](https://github.com/mrWheel/SPAmanage) - Single Page Application web interface

## Table of Contents

- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Display Options](#display-options)
- [Installation](#installation)
- [First Setup](#first-setup)
- [Configuration](#configuration)
- [Content Sources](#content-sources)
- [Web Interface](#web-interface)
- [Settings Reference](#settings-reference)
- [Special Message Commands](#special-message-commands)
- [File System Structure](#file-system-structure)
- [Troubleshooting](#troubleshooting)
- [Advanced Usage](#advanced-usage)

## Features

### Core Functionality
- **Dual Display Support**: Choose between Parola (LED matrix) or NeoPixels (WS2812) displays
- **Multiple Content Sources**: Local messages, RSS feeds, weather data, date/time
- **Web-Based Configuration**: Complete SPA interface for all settings
- **WiFi Management**: Auto-configuration with captive portal fallback
- **File System Manager**: Built-in file browser and editor
- **Automatic Brightness**: LDR sensor support for ambient light adjustment
- **Message Rotation**: Intelligent cycling through different content types

### Content Features
- **Local Messages**: Store up to 10 categories (A-J) of custom messages
- **RSS Feed Integration**: Support for up to 5 RSS feeds with health monitoring
- **Weather Integration**: Real-time weather data via Weerlive API
- **Time Display**: Multiple time/date format options
- **Special Commands**: Clear display, pixel tests, feed information

### Display Features
- **Scrolling Effects**: Multiple animation effects for text display
- **Color Control**: RGB color settings for NeoPixel displays
- **Speed Control**: Adjustable scrolling speed
- **Intensity Control**: Brightness adjustment (manual or automatic via LDR)
- **Multi-Zone Support**: Support for multiple display zones (Parola)

## Hardware Requirements

### Essential Components
- **ESP32 Development Board** (any variant with sufficient GPIO pins)
- **Display**: Either LED matrix modules OR NeoPixel matrix
- **Power Supply**: Adequate for ESP32 + display (varies by display size)

### Optional Components
- **LDR (Light Dependent Resistor)**: For automatic brightness control
- **Reset Button**: For WiFi configuration reset
- **Level Shifters**: May be needed for 5V displays

## Display Options

### Option 1: Parola LED Matrix Display

**Supported Hardware Types:**
- FC16_HW (most common)
- GENERIC_HW
- ICSTATION_HW
- PAROLA_HW

**Typical Wiring:**
```
ESP32 Pin -> LED Matrix
GPIO 23   -> DIN (Data In)
GPIO 18   -> CLK (Clock)
GPIO 5    -> CS (Chip Select)
5V        -> VCC
GND       -> GND
```

**Configuration:**
- Number of devices: 1-32 modules
- Number of zones: 1-2 zones
- Hardware type selection via settings

### Option 2: NeoPixel Matrix Display

**Supported Configurations:**
- Matrix sizes: Configurable width × height
- Color orders: RGB, RBG, GRB, GBR, BRG, BGR
- Frequencies: 400kHz or 800kHz
- Matrix layouts: Rows/Columns, Top/Bottom start, Left/Right direction, Progressive/Zigzag

**Typical Wiring:**
```
ESP32 Pin -> NeoPixel Matrix
GPIO 5    -> Data In
5V        -> VCC (or external power for large matrices)
GND       -> GND
```

**Configuration:**
- Matrix dimensions (width × height)
- Pixels per character
- Matrix orientation and sequence
- Color order and frequency

## Installation

### Prerequisites
- [PlatformIO](https://platformio.org/) installed
- Git for cloning the repository

### Build Configuration

1. **Clone the repository:**
   ```bash
   git clone [repository-url]
   cd espTicker32
   ```

2. **Choose display type** in PlatformIO:
   - Open the project in PlatformIO (VS Code)
   - At the bottom of the VS Code window, look for the PlatformIO environment selector
   - Choose the appropriate environment:
     - **For Parola LED Matrix:** Select `env:espTicker32Parola`
     - **For NeoPixel Matrix:** Select `env:espTicker32Neopixels`
   
   **Note:** You don't need to modify any files - the environments are pre-configured!

3. **Build and upload:**
   ```bash
   pio run --target upload
   ```
   Or use the PlatformIO toolbar buttons in VS Code

4. **Upload file system:**
   ```bash
   pio run --target uploadfs
   ```
   Or use the "Upload Filesystem Image" button in PlatformIO


## First Setup

### Initial WiFi Configuration

1. **Power on the device**
2. **Look for WiFi network** named "espTicker32"
3. **Connect to the network** (no password required)
4. **Open browser** and go to `192.168.4.1`
5. **Configure your WiFi** credentials
6. **Device will restart** and connect to your network

### Finding the Device IP

After WiFi configuration, the device will display its IP address on the ticker. You can also:
- Check your router's DHCP client list
- Use network scanning tools
- Access via hostname: `http://espticker32.local` (if mDNS works)

### Initial Web Access

1. **Open browser** and navigate to the device IP
2. **Main interface** will show the ticker monitor
3. **Use the menus** to access configuration pages

## Configuration

### Device Settings

Access via: **Main Menu → Edit → Settings → Device Settings**

| Setting | Description | Default | Range/Options |
|---------|-------------|---------|---------------|
| **devHostname** | Device network hostname | "espTicker32" | String (max 30 chars) |
| **devTickerSpeed** | Scrolling speed | 75 | 25-200 |
| **devMaxIntensiteitLeds** | LED brightness | 2 | 0-15 |
| **devLdrPin** | LDR sensor pin | 34 | GPIO pin number |
| **devResetWiFiPin** | WiFi reset button pin | 0 | GPIO pin number |
| **devSkipWords** | Words to filter from RSS | "" | Comma-separated list |
| **devShowLocalA-J** | Enable local message categories | false | true/false |

### Display-Specific Settings

#### Parola Settings
Access via: **Main Menu → Edit → Settings → Parola Settings**

| Setting | Description | Default | Options |
|---------|-------------|---------|---------|
| **parolaHardwareType** | LED matrix hardware | 1 | 0=Generic, 1=FC16, 2=Parola, 3=ICStation |
| **parolaNumDevices** | Number of matrix modules | 4 | 1-8 |
| **parolaNumZones** | Number of display zones | 1 | 1-4 |
| **parolaPinDIN** | Data input pin | 23 | GPIO pin |
| **parolaPinCLK** | Clock pin | 18 | GPIO pin |
| **parolaPinCS** | Chip select pin | 5 | GPIO pin |

#### NeoPixels Settings
Access via: **Main Menu → Edit → Settings → Neopixels Settings**

| Setting | Description | Default | Options |
|---------|-------------|---------|---------|
| **neopixDataPin** | Data pin | 5 | GPIO pin |
| **neopixWidth** | Matrix width | 32 | 8-64 pixels |
| **neopixHeight** | Matrix height | 8 | 8-32 pixels |
| **neopixPixPerChar** | Pixels per character | 6 | 4-8 pixels |
| **neopixMATRIXTYPEV** | Vertical start | false | false=Top, true=Bottom |
| **neopixMATRIXTYPEH** | Horizontal start | false | false=Left, true=Right |
| **neopixMATRIXORDER** | Matrix order | false | false=Rows, true=Columns |
| **neopixMATRIXSEQUENCE** | Sequence type | false | false=Progressive, true=Zigzag |
| **neopixCOLOR** | Color order | 2 | 0=RGB, 1=RBG, 2=GRB, 3=GBR, 4=BRG, 5=BGR |
| **neopixFREQ** | Signal frequency | false | false=800kHz, true=400kHz |

### Weather Settings (Weerlive)

Access via: **Main Menu → Edit → Settings → Weerlive Settings**

| Setting | Description | Example |
|---------|-------------|---------|
| **weerliveAuthToken** | API authentication token | "your-api-key" |
| **weerlivePlaats** | Location name | "Amsterdam" |
| **weerliveRequestInterval** | Update interval (minutes) | 30 |

**Getting Weerlive API Access:**
1. Register at [Weerlive.nl](https://weerlive.nl/)
2. Obtain your API key
3. Enter the key and your location in settings

### RSS Feed Settings

Access via: **Main Menu → Edit → Settings → RSS feed Settings**

Configure up to 5 RSS feeds:

| Setting | Description | Example |
|---------|-------------|---------|
| **domain0-4** | RSS feed domain | "feeds.bbci.co.uk" |
| **path0-4** | RSS feed path | "/news/rss.xml" |
| **maxFeeds0-4** | Max items to fetch | 10 |
| **requestInterval** | Update interval (minutes) | 60 |

**Popular RSS Feeds:**
- BBC News: `feeds.bbci.co.uk/news/rss.xml`
- CNN: `rss.cnn.com/rss/edition.rss`
- Reuters: `feeds.reuters.com/reuters/topNews`

## Content Sources

### Local Messages

Local messages are stored in categories A through J. Each category can be enabled/disabled independently.

**Managing Local Messages:**
1. Go to **Main Menu → Edit → Local Messages**
2. **Add/edit messages** in the text fields
3. **Use category keys** (A-J) to organize content
4. **Enable categories** in Device Settings
5. **Click Save** to store messages

**Message Categories:**
- **Category A-J**: Each can contain custom text
- **Enable/Disable**: Control which categories are active
- **Rotation**: System cycles through enabled categories

### RSS Feeds

RSS feeds provide dynamic content from news sources and websites.

**Feed Configuration:**
1. **Domain**: The website domain (e.g., "feeds.bbci.co.uk")
2. **Path**: The RSS feed path (e.g., "/news/rss.xml")
3. **Max Items**: Number of items to fetch (1-50)
4. **Update Interval**: How often to check for new content

**Feed Health Monitoring:**
- System monitors feed availability
- Displays feed status information
- Automatically skips failed feeds
- Use `<feedInfo>` command to view feed health

### Weather Data

Weather information from Weerlive provides current conditions and forecasts.

**Setup Requirements:**
1. **API Key**: Register at Weerlive.nl for free API access
2. **Location**: Specify your city/location
3. **Update Interval**: Set refresh frequency (recommended: 30+ minutes)

**Weather Display:**
- Current conditions
- Temperature and humidity
- Weather description
- Automatic updates

## Web Interface

### Main Page - Ticker Monitor

The main page shows a real-time monitor of ticker content:
- **Scrolling Display**: Shows current ticker content
- **Message Queue**: Displays upcoming messages
- **Status Information**: System status and connectivity

### Local Messages Editor

**Features:**
- **Text Input Fields**: One field per message
- **Category Management**: Organize messages by A-J keys
- **Real-time Preview**: See changes immediately
- **Bulk Operations**: Save all messages at once

**Special Keywords:**
- `<time>` - Display current time
- `<date>` - Display current date
- `<datetime>` - Display date and time
- `<weerlive>` - Show weather information
- `<rssfeed>` - Display RSS feed content
- `<spaces>` - Clear the display
- `<feedInfo>` - Show feed health information
- `<clear>` - Clear display

### Settings Pages

**Device Settings:**
- Network configuration
- Display timing and brightness
- GPIO pin assignments
- Feature enable/disable options

**Display Settings (Parola/NeoPixels):**
- Hardware-specific configuration
- Display dimensions and layout
- Color and effect settings

**Content Settings:**
- RSS feed configuration
- Weather service setup
- Update intervals

### File System Manager

**Capabilities:**
- **Browse Files**: Navigate LittleFS file system
- **Upload Files**: Add new files to the system
- **Download Files**: Retrieve files from the device
- **Delete Files**: Remove unwanted files
- **Create Folders**: Organize file structure
- **Edit Text Files**: Modify configuration files

## Settings Reference

### Complete Settings List

#### Device Settings (`settings.ini`)
```ini
[device]
devHostname=espTicker32
devTickerSpeed=75
devMaxIntensiteitLeds=2
devLdrPin=34
devResetWiFiPin=0
devSkipWords=
devShowLocalA=false
devShowLocalB=false
devShowLocalC=false
devShowLocalD=false
devShowLocalE=false
devShowLocalF=false
devShowLocalG=false
devShowLocalH=false
devShowLocalI=false
devShowLocalJ=false
```

#### Parola Settings (`parola.ini`)
```ini
[parola]
parolaHardwareType=1
parolaNumDevices=4
parolaNumZones=1
parolaPinDIN=23
parolaPinCLK=18
parolaPinCS=5
```

#### NeoPixels Settings (`neopixels.ini`)
```ini
[neopixels]
neopixDataPin=5
neopixWidth=32
neopixHeight=8
neopixPixPerChar=6
neopixMATRIXTYPEV=false
neopixMATRIXTYPEH=false
neopixMATRIXORDER=false
neopixMATRIXSEQUENCE=false
neopixCOLOR=2
neopixFREQ=false
```

#### Weather Settings (`weerlive.ini`)
```ini
[weerlive]
weerliveAuthToken=
weerlivePlaats=
weerliveRequestInterval=30
```

#### RSS Feed Settings (`rssFeeds.ini`)
```ini
[rssfeeds]
domain0=
path0=
maxFeeds0=0
domain1=
path1=
maxFeeds1=0
domain2=
path2=
maxFeeds2=0
domain3=
path3=
maxFeeds3=0
domain4=
path4=
maxFeeds4=0
requestInterval=60
```

## Special Message Commands

### Time and Date Commands
- `<time>` - Shows current time (HH:MM format)
- `<date>` - Shows current date (DD/MM/YYYY format)
- `<datetime>` - Shows both date and time

### Content Commands
- `<weerlive>` - Displays weather information
- `<rssfeed>` - Shows next RSS feed item
- `<feedInfo>` - Displays feed health status (cycles through all feeds)
- `<feedInfoReset>` - Resets feed info counter to start

### Display Commands
- `<spaces>` - Fills display with spaces (creates gap)
- `<clear>` - Clears the display completely
- `<pixeltest>` - Tests all pixels (shows all characters as 0x7F)

### Usage Notes
- Commands must be the **only content** in a message field
- Commands are **case-insensitive**
- Commands trigger **automatic color changes** for visual distinction
- Use `<spaces>` to create **visual breaks** between different content types

## File System Structure

### System Files (`/SYS/`)
```
/SYS/
├── SPAmanager.html     # Main web interface
├── SPAmanager.css      # Web interface styling
├── SPAmanager.js       # Web interface functionality
├── FSmanager.css       # File manager styling
├── FSmanager.js        # File manager functionality
├── espTicker32.js      # Device-specific JavaScript
└── disconnected.html   # Offline page
```

### Configuration Files
```
/
├── settings.ini        # Device settings
├── parola.ini         # Parola display settings
├── neopixels.ini      # NeoPixels display settings
├── weerlive.ini       # Weather service settings
├── rssFeeds.ini       # RSS feed configuration
├── localMessages.dat  # Local messages storage
├── skipWords.txt      # Words to filter from RSS
└── favicon.ico        # Web interface icon
```

### User Files
- **Custom configurations**: Additional .ini files
- **Message files**: Custom message storage
- **Logs**: System log files (if enabled)

## Troubleshooting

### WiFi Connection Issues

**Problem**: Device doesn't connect to WiFi
**Solutions:**
1. **Reset WiFi settings**: Hold `WiFiReset` button during startup
2. **Check credentials**: Ensure correct SSID and password
3. **Signal strength**: Move device closer to router
4. **Network compatibility**: Ensure 2.4GHz network (ESP32 doesn't support 5GHz)

**Problem**: Can't access web interface
**Solutions:**
1. **Check IP address**: Look for IP on ticker display
2. **Network connectivity**: Ensure device and computer on same network
3. **Firewall**: Check firewall settings on computer
4. **Browser cache**: Clear browser cache and cookies

### Display Issues

**Problem**: No display output
**Solutions:**
1. **Check wiring**: Verify all connections
2. **Power supply**: Ensure adequate power for display
3. **Pin configuration**: Verify GPIO pin settings
4. **Display type**: Ensure correct build configuration (Parola vs NeoPixels)

**Problem**: Garbled or incorrect display
**Solutions:**
1. **Hardware type**: Check Parola hardware type setting
2. **Matrix configuration**: Verify NeoPixel matrix settings
3. **Signal integrity**: Add pull-up resistors or level shifters
4. **Power stability**: Use adequate, stable power supply

### Content Issues

**Problem**: No RSS feed content
**Solutions:**
1. **Internet connectivity**: Verify device has internet access
2. **Feed URLs**: Check RSS feed URLs are correct and accessible
3. **Feed health**: Use `<feedInfo>` command to check feed status
4. **Update interval**: Ensure reasonable update intervals (not too frequent)

**Problem**: Weather data not updating
**Solutions:**
1. **API key**: Verify Weerlive API key is correct
2. **Location**: Check location name spelling
3. **API limits**: Ensure not exceeding API rate limits
4. **Network connectivity**: Verify internet access

### File System Issues

**Problem**: Settings not saving
**Solutions:**
1. **File system**: Re-upload file system data
2. **Permissions**: Check file system integrity
3. **Storage space**: Ensure adequate free space
4. **Factory reset**: Clear all settings and reconfigure

## Advanced Usage

### Custom Message Rotation

Create sophisticated message sequences by combining different content types:

```
Message A: "Welcome to espTicker32"
Message B: <time>
Message C: <weerlive>
Message D: <rssfeed>
Message E: <spaces>
```

Enable categories A, B, C, D, E for automatic rotation.

### RSS Feed Filtering

Use the `devSkipWords` setting to filter unwanted content:
```
devSkipWords=advertisement,sponsored,breaking,urgent
```

### Automatic Brightness Control

Connect an LDR (Light Dependent Resistor) for automatic brightness adjustment:

**Wiring:**
```
      +- LDR ---+--- 10kΩ resistor --+
      |         |                    |
     GND      GPIOnn                3.3v
```

**Configuration:**
- Set `devLdrPin=34`
- System automatically adjusts brightness based on ambient light

### Custom Color Schemes (NeoPixels)

The system automatically assigns colors to different content types:
- **White**: Local messages
- **Blue**: Weather data
- **Green**: RSS feeds
- **Purple**: Date display
- **Magenta**: Date/time display
- **Yellow**: Feed information

### Network Integration

**mDNS Access:**
- Device accessible via `http://espticker32.local`
- Hostname configurable via `devHostname` setting

**API Integration:**
- Web interface uses WebSocket for real-time updates
- RESTful endpoints for file management
- JSON-based configuration system

### Backup and Restore

**Backup Configuration:**
1. Access File System Manager
2. Download all .ini files
3. Download localMessages.dat
4. Save files to computer

**Restore Configuration:**
1. Access File System Manager
2. Upload saved .ini files
3. Upload localMessages.dat
4. Restart device

### Development and Debugging

**Enable Debug Mode:**
```ini
build_flags = 
  -DUSE_PAROLA  ; or -DUSE_NEOPIXELS
  -DESPTICKER32_DEBUG
```

**Serial Monitor:**
- Baud rate: 115200
- Detailed logging of all operations
- RSS feed processing information
- Network connectivity status

**Custom Builds:**
- Modify source code for custom features
- Add additional RSS feeds (up to 8 supported)
- Implement custom display effects
- Integrate additional sensors

---

## Support and Contributing

For issues, questions, or contributions, please refer to the project repository and documentation. The espTicker32 project builds upon the excellent work of the ESP32 and Arduino communities, particularly the MD_Parola library by Marco Colli.

**Version**: 1.3.0
**License**: Check repository for license information
**Dependencies**: See platformio.ini for complete library list
