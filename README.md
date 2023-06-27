# Konnected for ESPHome
This repository houses the officially supported [ESPHome firmware](https://esphome.io) configuration files and recipes for [Konnected](https://konnected.io)
products. Konnected is a U.S. based maker of IoT hardware and software that integrates traditional wired alarm systems and (coming soon!) garage door
openers into your smart home.

## Quick Start
1. **Get started quickly** by [flashing your Konnected device with ESPHome firmware using Konnected's web-based tool](https://install.konnected.io/esphome).
1. Power on the device.
1. On your computer or mobile device, connect to the device's captive portal WiFi setup network named "alarm-panel-XXXXXX" or "garage-door-XXXXXX" (depending on the product) where XXXXXX is a unique hexadecimal number. For ethernet devices, plug in the Ethernet cable and skip to step 5.
1. Select your home WiFi network and enter your WiFi password in the captive portal.
1. Discover the Konnected device in [Home Assistant > Settings > Devices & Services](https://my.home-assistant.io/redirect/integrations)
1. (Optional) Discover the device and customize the firmware in ESPHome Dashboard ([see ESPHome add-on](https://my.home-assistant.io/redirect/supervisor_store/)).

## Changes and Release Notes
See [releases](/konnected-io/konnected-esphome/releases) for release notes and downloadable pre-built flashable images.

## Made for ESPHome
Konnected's products are made with ESP32 and ESP8266 microcontrollers with integrated USB interfaces, and are completely open to end-user servicing and customization, making them ideal products for ESPHome firmware. Since 2023, Konnected maintains and distributes ESPHome configuration recipies for all products. These firmwares are for Home Assistant users who want a plug-and-play solution. More advanced users can import Konnected's ESPHome configurations into their ESPHome Dashboard and easily customize, build, and update their device(s) with a few simple edits of the well-commented configuration files and packages provided by Konnected.

## Works with Home Assistant
Use these ESPHome firmware configurations for easy and plug-and-play setup in [Home Assistant](https://home-assistant.io) that's local, fast, and completely customizable.

## In This Repository
This repository houses several complete firmware configuration YAML templates for various Konnected devices at the top level. These are the main firmware templates imported by the `dashboard_import` feature when used with ESPHome Dashboard.

### `alarm-panel-pro-esp32.yaml`
Konnected's flagship [Alarm Panel Pro](https://konnected.io/products/konnected-alarm-panel-pro-12-zone-kit) is an ESP32 powered wired alarm panel with Wifi, Ethernet and PoE.

![https://konnected.io/products/konnected-alarm-panel-pro-12-zone-kit](https://s3.amazonaws.com/cdn.freshdesk.com/data/helpdesk/attachments/production/32013019813/original/KH0hcaxBhEf3Y-LBaUUuki1OTTnFjryGuA.jpg?1666818270)

#### Setup Required:
When using this template, you _must_ make a few configuration choices, detailed below.
1. **WiFi** or **Ethernet**. Under _PACKAGES_, un-comment either the `wifi` or `ethernet` package depending on the desired connection type. Note: Either `wifi` or `ethernet` component must be enabled, but both cannot be enabled.
1. **Ethernet type**. (If `ethernet` is selected in the step above) Under _GENERAL SETTING > ETHERNET CONFIG_ uncomment one of the `ethernet_type:` substitution variables. Choose `LAN8720` for the Alarm Panel Pro up to v1.7. Choose `RTL8201` for the Alarm Panel Pro v1.8 and above. The hardware version is displayed on the front of the device underneath the logo.

### `alarm-panel-esp8266.yaml`
Konnected's original [6-zone Alarm Panel and Alarm Panel Add-on](https://konnected.io/products/konnected-alarm-panel-wired-alarm-system-conversion-kit) is a compact WiFi-only modular wired alarm panel, infinitely expandable for any size system.

![https://konnected.io/products/konnected-alarm-panel-wired-alarm-system-conversion-kit](https://user-images.githubusercontent.com/12016/139100157-5e792dbe-fd08-45c1-8637-7dedfc0ae7ef.jpg)

This firmware config will also run on a NodeMCU module and Konnected's v1 (2018-2020) Alarm Panel product.


### `garage-door-esp8266.yaml`
A [smart garage door opener](https://konnected.io/products/smart-garage-door-opener) based on the ESP8266.

![https://konnected.io/products/smart-garage-door-opener](https://konnected.io/cdn/shop/files/GDOProductImages_2_1.png?v=1683919623&width=600)

## Customization

Customize your ESPHome based firmware to your own needs by making simple edits to the firmware configuration file in the ESPHome Dashboard.

### Substitution Variables

#### `name`
A unique name for this device. If you have multiple Konnected panels, choose a different name for each. The name can only include lowercase letters, numbers, and hyphens.

#### `friendly_name`
A more friendly looking name, for example "Alarm Panel" used as a prefix on some of the sensors. i.e. "Alarm Panel IP Address". If you have multiple Konnected devices, choose a different friendly name for each.

#### `sensor_debounce_time`
Sensors use a [debouncing](https://esphome.io/components/binary_sensor/gpio.html#debouncing-values) filter to prevent transient sensor state changes or flickers from triggering a false sensor event. Default: `200ms`

#### `warning_beep_pulse_time`
Duration of a warning beep pulse. 

#### `warning_beep_pause_time`
Quiet pause time between warning beeps.

#### `warning_beep_internal_only`
Whether the warning beep entity should be only used internally, or should also be exposed to the API / Home Assistant. 

#### `warning_beep_pin`
The zone that the piezo buzzer is wired to for warning beep.

Possible values (ESP8266): `$zone1` thru `$zone5` or `$out`

Possible values (ESP32): `$zone1` thru `$zone8` or `$alarm1`, `alarm2`, `$out1`, `$out2`

### Packages
The firmware configuration is organized into _packages_ of functionality for different device features. These are defined in the `packages` folder of this repository.

Commment-out or remove package lines from this file that you don't want or need. For example, if you do not want a warning beep entity created, comment-out the `warning_beep` package by putting a `#` at the front of the line.

To customize functionality from packages, copy and paste the package contents (from Github) into your device configuration file and comment-out or remove the package import line. Then you can customize the functionality that came from this package for your device.

## Get Help
Konnected customers are welcome to contact Konnected Support at [help.konnected.io](https://help.konnected.io) or by email: [help@konnected.io](mailto://help@konnected.io) for help with your ESPHome firmware on a genuine Konnected device.