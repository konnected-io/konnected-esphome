# secplus_gdo

## Documentation and Example Usage

See [packages/secplus-gdo.yaml](../packages/secplus-gdo.yaml) for a complete example.

### Setup
Import the component into your ESPHome config:

```
external_components:
  - source: github://konnected-io/konnected-esphome@master
    components: [ secplus_gdo ]
```

Include the component:
```
secplus_gdo:
  id: gdo_blaq
  input_gdo_pin: ${uart_rx_pin}
  output_gdo_pin: ${uart_tx_pin}
```

### Cover

```
cover:
  - platform: secplus_gdo
    name: Garage Door
    secplus_gdo_id: gdo_blaq
    id: gdo_door
    pre_close_warning_duration: 5s
    pre_close_warning_start:
      - button.press: pre_close_warning

```

#### Configuration options

* **secplus_gdo_id** (**Required**, string): The ID of the component set above.
* **pre_close_warning_duration** (_Optional_, Time): Duration of the pre-close warning
* **pre_close_warning_start** (_Optional_, Action): Action(s) to execute the pre-close warning
* **pre_close_warning_end** (_Optional_, Action): Action(s) to execute when the pre-close warning stops
* **toggle_only** (_Optional_, boolean)_: When true, all open/close commands are sent as a toggle door actions to the garage opener. This is a compatibility fix for some openers, such as some Merlin brand openers that are known to only respond to toggle commands and don't respond to discrete open/close commands.