# GlucoCraft LED Christmas Tree

An example ESPHome configuration for an LED strip on a Christmas tree that shows
[GlucoCraft](https://github.com/Verstreubulator/glucocraft)'s current zone as a band of light in GlucoCraft's display
colors. It is a decoration that displays GlucoCraft's data; it is not part of GlucoCraft.

*For development and testing only. Not for medical use.* ([GlucoCraft's disclaimer](https://github.com/Verstreubulator/glucocraft#glucocraft))

- Very low and very high light the whole strip; each of the other five zones lights its own band.
- Without a current reading, as GlucoCraft defines it, no band is shown and the strip shows its decoration colors only.
- An optional 128 × 64 screen shows the trend arrow, the value, the 5-minute delta and the zone name; the colors alone
  do not name the zone.

## What it shows

| GlucoCraft zone | LEDs (0 = the bottom of the strip; on 50 LEDs) |
|---|---|
| Very low | the whole strip |
| Low | 0–12 |
| Near low | 13–24 |
| Tight range | 25–35 |
| Near high | 36–44 |
| High | 45–49 |
| Very high | the whole strip |

The bands scale with the strip's length. The colors are GlucoCraft's display colors, read from Home Assistant: a
color or range changed in GlucoCraft's settings reaches the tree without changing this configuration. The tree reads
GlucoCraft's entities (`sensor.glucocraft_last_glucose_reading`, `sensor.glucocraft_configuration`,
`binary_sensor.glucocraft_no_current_data`, `binary_sensor.glucocraft_sensor_warm_up`,
`sensor.glucocraft_last_reading_time`; the screen also `sensor.glucocraft_trend_arrow` and
`sensor.glucocraft_5_minute_delta`), as described in GlucoCraft's [entities and API reference](https://github.com/Verstreubulator/glucocraft/blob/main/docs/ENTITIES_AND_API.md).

**No current reading** means: GlucoCraft reports no current data, the sensor is warming up, or the tree's own clock
finds the last reading older than GlucoCraft's limit.

## Controls in Home Assistant

| Entity | What it does |
|---|---|
| Tree Power | The strip (and the screen) on or off |
| Glucose Mode | On: the zone's band is shown. Off: decoration only |
| Tree Color Scheme | **White** (warm white) or **Color** (mixed colors; with a band, the other LEDs run at half brightness and avoid colors that are easy to confuse with the band) |
| Tree Effect, Tree Effect Intensity | Solid, Twinkle, Shimmer, Chase, Wave or Sparkle, at intensity 1–3 |
| Tree Brightness | 0–100 % |
| Tree Demo | Steps through "no current reading" and the seven zones in GlucoCraft's display colors, one step every 5 seconds |
| Display Mode (screen) | Glucose, Message (holiday messages from Thanksgiving to January 6) or Off |
| Glucose Data Valid, Glucose Data Age | Whether a current reading is shown, and the minutes since the last reading |

Settings are kept through restarts.

## Hardware

- An ESP32 board supported by ESPHome (the example uses a Seeed Studio XIAO ESP32-C6).
- An addressable LED strip (the example: 50 × WS2812) with its own power supply sized for the strip; the data line to
  the board's LED pin and a common ground.
- Optional: an SSD1306 128 × 64 screen and a BME280 sensor on the board's I2C pins.

## Building

1. Copy `christmas-tree.yaml`, `glucocraft_tree.h` and the `packages/` folder into the ESPHome configuration folder.
2. Copy `secrets.yaml.example` to `secrets.yaml` and fill in the values (never publish `secrets.yaml`).
3. In `christmas-tree.yaml`, set the substitutions to the board's pins and the strip, and remove the optional packages
   that are not used.
4. Build and install with ESPHome, then add the device in Home Assistant (Settings → Devices & services).

GlucoCraft must be installed and set up in the same Home Assistant.

## Files

| File | Contents |
|---|---|
| `christmas-tree.yaml` | The example device: board, network, pins, which packages |
| `packages/core.yaml` | Required: GlucoCraft's entities, the controls, the strip and its effects |
| `packages/display.yaml` | Optional: the screen |
| `packages/enclosure-bme280.yaml` | Optional: temperature, humidity and pressure in the box |
| `glucocraft_tree.h` | Helpers: the zones, colors, bands and holiday dates |

## License

Copyright (C) 2026 The GlucoCraft authors. Licensed under the GNU Affero General Public License, version 3 only
(SPDX identifier `AGPL-3.0-only`; see [LICENSE](LICENSE)). Provided WITHOUT ANY WARRANTY. ESPHome, which builds the
firmware, has its own license.
