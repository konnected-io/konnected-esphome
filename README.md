# Konnected for ESPHome
This repository houses the officially supported [ESPHome firmware](https://esphome.io) configuration files and recipes for [Konnected](https://konnected.io)
products. Konnected is a U.S. based maker of IoT hardware and software that integrates traditional wired alarm systems and garage door
openers into your smart home.

## Quick Start
1. **Get started quickly** by [flashing your Konnected device with ESPHome firmware using Konnected's web-based tool](https://install.konnected.io/esphome).
1. Power on the device.
1. On your computer or mobile device, connect to the device's captive portal WiFi setup network named "konnected-XXXXXX" where XXXXXX is a unique hexadecimal number. For ethernet devices, plug in the Ethernet cable and skip to step 5.
1. Select your home WiFi network and enter your WiFi password in the captive portal.
1. Discover the Konnected device in [Home Assistant > Settings > Devices & Services](https://my.home-assistant.io/redirect/integrations)
1. (Optional) Discover the device and customize the firmware in ESPHome Dashboard ([see ESPHome add-on](https://my.home-assistant.io/redirect/supervisor_store/)).

## Changes and Release Notes
See [releases](https://github.com/konnected-io/konnected-esphome/releases) for release notes and downloadable pre-built flashable images.

## Made for ESPHome
Konnected's products are made with ESP32 and ESP8266 microcontrollers with integrated USB interfaces, and are completely open to end-user servicing and customization, making them ideal products for ESPHome firmware. Since 2023, Konnected maintains and distributes ESPHome configuration recipies for all products. These firmwares are for Home Assistant users who want a plug-and-play solution. More advanced users can import Konnected's ESPHome configurations into their ESPHome Dashboard and easily customize, build, and update their device(s) with a few simple edits of the well-commented configuration files and packages provided by Konnected.

## Works with Home Assistant
Use these ESPHome firmware configurations for easy and plug-and-play setup in [Home Assistant](https://home-assistant.io) that's local, fast, and completely customizable.

## In This Repository
This repository houses several complete firmware configuration YAML templates for various Konnected devices at the top level. These are the main firmware templates imported by the `dashboard_import` feature when used with ESPHome Dashboard.

### `alarm-panel-pro-esp32-ethernet.yaml`
### `alarm-panel-pro-esp32-wifi.yaml`
Konnected's flagship [Alarm Panel Pro](https://konnected.io/products/konnected-alarm-panel-pro-12-zone-kit) is an ESP32 powered wired alarm panel with Wifi, Ethernet and PoE.

![https://konnected.io/products/konnected-alarm-panel-pro-12-zone-kit](https://s3.amazonaws.com/cdn.freshdesk.com/data/helpdesk/attachments/production/32013019813/original/KH0hcaxBhEf3Y-LBaUUuki1OTTnFjryGuA.jpg?1666818270)

#### Setup Required:
1. **Ethernet type**. If the `ethernet` build is used, Under _GENERAL SETTING > ETHERNET CONFIG_ uncomment one of the `ethernet_type:` substitution variables. Choose `LAN8720` for the Alarm Panel Pro up to v1.7. Choose `RTL8201` for the Alarm Panel Pro v1.8 and above. The hardware version is displayed on the front of the device underneath the logo.

### `alarm-panel-esp8266.yaml`
Konnected's original [6-zone Alarm Panel and Alarm Panel Add-on](https://konnected.io/products/konnected-alarm-panel-wired-alarm-system-conversion-kit) is a compact WiFi-only modular wired alarm panel, infinitely expandable for any size system.

![https://konnected.io/products/konnected-alarm-panel-wired-alarm-system-conversion-kit](https://user-images.githubusercontent.com/12016/139100157-5e792dbe-fd08-45c1-8637-7dedfc0ae7ef.jpg)

This firmware config will also run on a NodeMCU module and Konnected's v1 (2018-2020) Alarm Panel product.

### `garage-door-GDOv1-S.yaml`
A [smart garage door opener](https://konnected.io/products/smart-garage-door-opener) based on the ESP8266 (model: GDOv1-S).

![https://konnected.io/products/smart-garage-door-opener](https://konnected.io/cdn/shop/files/GDOProductImages_2_1.png?v=1683919623&width=600)

### `garage-door-GDOv2-S.yaml`
An improved [smart garage door opener retrofit](https://konnected.io/products/smart-garage-door-opener) based on the ESP32-S3-Mini (GDOv2-S).

### `garage-door-GDOv2-Q.yaml`
Firmware for Konnected's GDO blaQ -- a [smart garage door opener retrofit](https://konnected.io/products/smart-garage-door-opener-blaq-myq-alternative) for Security+ and Security+ 2.0 openers with direct serial connection to the garage door opener unit (GDOv2-Q).

## ESPHome Components
Components are found in the `components` directory of this repo.

### Secplus GDO
Secplus GDO is an ESPHome component for controlling Security+ and Security+ 2.0 garage openers
made by Chamberlain Group, including garage door openers sold since 1997 under the Chamberlain,
LiftMaster, Craftsman and Merlin brands.

#### This component is maintained by Konnected and powers the [Konnected GDO blaQ](https://konnected.io/products/smart-garage-door-opener-blaq-myq-alternative) Konnected's smart garage door opener accessory for Security+ garage door openers.

#### Documentation

See [README](https://github.com/konnected-io/konnected-esphome/tree/master/components/secplus_gdo)

#### Adapted from [ratgdo](https://github.com/ratgdo)

This component was adapted from and inspired by [ratgdo](https://paulwieland.github.io/ratgdo/), however it's not a copy and not feature-equivalent. We initially set out to fork ratgdo and contribute back some improvements, however it ultimately became a complete rewrite. Konnected is publishing this derivative work under the GPLv3 license.

#### Project Goals
The ratgdo and [secplus](https://github.com/argilo/secplus) developers found a way to communicate with Security+ garage door openers over a reverse-engineered serial wireline protocol. This novel development enabled owners of Security+ garage openers to read and send commands directly to the garage unit. Konnected set out to incorporate this technology into a finished consumer product. To do that, we wanted to adapt the code to:

* Remove dependency on Arduino
* Run on the ESP-IDF platform
* Utilize the ESP32's hardware UART (instead relying on the SoftwareSerial arduino library)
* Decouple the Security+ wireline protocol from the ESPHome component

#### Other Differences from ratgdo

1. *Event-driven instead of polling*. This library uses an event-driven architecture to receive data from the garage door unit, instead of polling for updates. This results in a measurably faster response time to state changes.
1. *Two wires instead of three*. The obstruction sensor state can now be read via the serial protocol, therefore it doesn't need to be wired separately to a GPIO. This makes installation simpler, only requiring the red and white wire to the wall button inputs on the garage door opener.
1. *Supports a pre-closing warning*. To comply with [U.S. 16 CFR 1211.14(f)](https://www.ecfr.gov/current/title-16/part-1211#p-1211.14(f))
1. *Automatic detection of Security+ vs Security+ 2.0*. Less for the end-user to think about.
1. *Removes dry contact trigger support*. We feel that this is better handled with a _template cover_ in ESPHome, and this library should focus on the Security+ interaction only.
1. *Removes relay outputs*. Again, simplifying the library to do Security+ only.

#### Noted Dependencies

1. [gdolib](https://github.com/konnected-io/gdolib)
1. [secplus](https://github.com/argilo/secplus)

## Customization

Customize your ESPHome based firmware to your own needs by making simple edits to the firmware configuration file in the ESPHome Dashboard.

### Substitution Variables

#### `name`
A unique name for this device. If you have multiple Konnected panels, choose a different name for each. The name can only include lowercase letters, numbers, and hyphens.

#### `friendly_name`
A human-readable name, for example "Alarm Panel", used to identify the device in Home Assistant front-end. If you have multiple Konnected devices, it's recommended to set a different friendly name for each.

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

Commment-out or remove package lines from this file that you don't want or need. For example, if you do not want a warning beep entity created, comment-out the `warning_beep` package by putting a `#` at the front of the line. See Konnected's support pages for more information:
  * [Customizing ESPHome firmware on Konnected Alarm Panel 6-zone models (ESP8266)](https://support.konnected.io/esphome-on-the-konnected-alarm-panel-esp8266)
  * [Customizing ESPHome firmware on Konnected Alarm Panel Pro (ESP32)](https://support.konnected.io/esphome-customization-on-the-konnected-alarm-panel-pro-esp32)
  * [Customizing ESPHome firmware on Konnected Garage Door Opener](https://support.konnected.io/esphome-customization-on-the-garage-door-opener)

To customize functionality from packages, copy and paste the package contents (from Github) into your device configuration file and comment-out or remove the package import line. Then you can customize the functionality that came from this package for your device.

## Get Help
Konnected customers are welcome to contact Konnected Support at [help.konnected.io](https://help.konnected.io) or by email: [help@konnected.io](mailto://help@konnected.io) for help with your ESPHome firmware on a genuine Konnected device.