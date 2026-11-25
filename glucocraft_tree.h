// License: AGPL-3.0-only (LICENSE).
// =============================================================================
// glucocraft_tree.h — helpers for the GlucoCraft LED Christmas tree example
// (packages/core.yaml, packages/display.yaml).
//
// The tree shows GlucoCraft's data: the zone of the current reading and its color
// from GlucoCraft's display colors, both read from Home Assistant. Nothing here
// works out a zone from a number.
// =============================================================================
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

#include "esphome/core/color.h"
#include "esphome/core/helpers.h"
#include "esphome/core/time.h"

namespace gct {

// ---- GlucoCraft zones -------------------------------------------------------
// The seven zones of sensor.glucocraft_last_glucose_reading (attribute `zone`),
// lowest to highest. Index = position in this list.
static const char *const ZONES[7] = {"very_low", "low",  "near_low", "tight_range",
                                     "near_high", "high", "very_high"};

// GlucoCraft's default display colors, used only until the live `zone_colors`
// attribute of sensor.glucocraft_configuration has been received.
static const uint32_t DEFAULT_ZONE_RGB[7] = {0xC62828, 0xEF5350, 0xFFC107, 0x4CAF50,
                                             0xFFC107, 0x42A5F5, 0x1565C0};

inline esphome::Color rgb(uint32_t v) {
  return esphome::Color((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF);
}

// "tight_range" -> 3; anything else (None, unavailable, empty) -> -1.
inline int zone_index(const std::string &zone) {
  for (int i = 0; i < 7; i++) {
    if (zone == ZONES[i]) return i;
  }
  return -1;
}

// The zones GlucoCraft gives a zone label: very low, low, high, very high.
inline bool zone_out_of_range(int zone) { return zone == 0 || zone == 1 || zone == 5 || zone == 6; }

// "#RRGGBB" -> color. Anything else -> false, `out` unchanged.
inline bool parse_hex_color(const std::string &s, esphome::Color &out) {
  if (s.size() != 7 || s[0] != '#') return false;
  char *end = nullptr;
  unsigned long v = strtoul(s.c_str() + 1, &end, 16);
  if (end == nullptr || *end != '\0') return false;
  out = rgb(static_cast<uint32_t>(v));
  return true;
}

// The `zone_colors` attribute arrives as text: "{'very_low': '#C62828', ...}".
// Fills the zones it finds and returns how many; anything else leaves `out` unchanged.
inline int parse_zone_map(const std::string &s, esphome::Color out[7]) {
  int found = 0;
  for (int i = 0; i < 7; i++) {
    const std::string key = std::string("'") + ZONES[i] + "'";
    size_t p = s.find(key);
    if (p == std::string::npos) continue;
    size_t h = s.find('#', p + key.size());
    if (h == std::string::npos || h + 7 > s.size()) continue;
    esphome::Color c;
    if (parse_hex_color(s.substr(h, 7), c)) {
      out[i] = c;
      found++;
    }
  }
  return found;
}

// Minutes from an epoch-seconds text (`last_reading_ts`) to `now`; NAN when either
// is unknown. Parsed as a double: a float loses about two minutes at this size.
inline double minutes_since(const std::string &epoch_text, const esphome::ESPTime &now) {
  if (!now.is_valid() || epoch_text.empty()) return NAN;
  char *end = nullptr;
  double ts = strtod(epoch_text.c_str(), &end);
  if (end == epoch_text.c_str() || ts <= 0) return NAN;
  return ((double) now.timestamp - ts) / 60.0;
}

// ---- Layout -----------------------------------------------------------------
// Very low and very high light the whole strip. The five middle zones each light a
// band, bottom (LED 0) to top, at these fractions of the strip (on 50 LEDs:
// 0-12, 13-24, 25-35, 36-44, 45-49).
static const int BAND_EDGE[6] = {0, 13, 25, 36, 45, 50};  // in 50ths of the strip

inline bool led_in_zone(int zone, int i, int n) {
  if (zone == 0 || zone == 6) return true;
  if (zone < 1 || zone > 5 || n <= 0) return false;
  const int lo = BAND_EDGE[zone - 1] * n / 50, hi = BAND_EDGE[zone] * n / 50;
  return i >= lo && i < hi;
}

// ---- Decoration colors (the "Color" scheme) ---------------------------------
static const int PALETTE_SIZE = 9;
static const uint32_t PALETTE_RGB[PALETTE_SIZE] = {
    0x000080,  // dark blue
    0x00FFFF,  // cyan
    0xFFFF00,  // yellow
    0x00FF00,  // green
    0x800000,  // dark red
    0xFF0000,  // red
    0xFF00FF,  // magenta
    0xFFE6B4,  // warm white (also the whole "White" scheme)
    0xFF3264,  // pink
};
static const int WARM_WHITE = 7;

// Two colors that are easy to confuse (weighted "redmean" RGB distance).
inline bool colors_close(const esphome::Color &a, const esphome::Color &b) {
  long rmean = ((long) a.r + (long) b.r) / 2;
  long dr = (long) a.r - (long) b.r, dg = (long) a.g - (long) b.g, db = (long) a.b - (long) b.b;
  long d2 = (((512 + rmean) * dr * dr) >> 8) + 4 * dg * dg + (((767 - rmean) * db * db) >> 8);
  return d2 < 300L * 300L;
}

// A random decoration color; with `avoid`, one that is not easy to confuse with `zc`.
inline esphome::Color pick_palette(bool avoid, const esphome::Color &zc) {
  esphome::Color c = rgb(PALETTE_RGB[esphome::random_uint32() % PALETTE_SIZE]);
  for (int tries = 0; avoid && colors_close(c, zc) && tries < 20; tries++) {
    c = rgb(PALETTE_RGB[esphome::random_uint32() % PALETTE_SIZE]);
  }
  return c;
}

// The base color of every LED: the decoration (White, or Color spread evenly over
// the palette), then the zone's band on top (zone < 0 = no band). In the Color
// scheme, decoration LEDs that are easy to confuse with the zone color are re-picked.
inline void paint(std::vector<esphome::Color> &base, bool color_scheme, int zone, const esphome::Color &zc) {
  const int n = base.size();
  if (color_scheme) {
    std::vector<int> pick(n);
    for (int i = 0; i < n; i++) pick[i] = i % PALETTE_SIZE;
    for (int i = n - 1; i > 0; i--) std::swap(pick[i], pick[esphome::random_uint32() % (i + 1)]);
    for (int i = 0; i < n; i++) base[i] = rgb(PALETTE_RGB[pick[i]]);
  } else {
    for (int i = 0; i < n; i++) base[i] = rgb(PALETTE_RGB[WARM_WHITE]);
  }
  if (zone < 0) return;
  for (int i = 0; i < n; i++) {
    if (led_in_zone(zone, i, n)) {
      base[i] = zc;
    } else if (color_scheme && colors_close(base[i], zc)) {
      base[i] = pick_palette(true, zc);
    }
  }
}

// ---- Holiday dates (display package) ----------------------------------------
// Day of the week, 0 = Sunday (Sakamoto's method).
inline int dow(int y, int m, int d) {
  static const int t[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) y -= 1;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

// Season ordinal: November d = d, December d = 30 + d (anything else: -1000).
inline int season_ord(int month, int day) {
  if (month == 11) return day;
  if (month == 12) return 30 + day;
  return -1000;
}

// US Thanksgiving (fourth Thursday of November), as a season ordinal.
inline int thanksgiving_ord(int y) { return 1 + (4 - dow(y, 11, 1) + 7) % 7 + 21; }

// First Sunday of Advent (fourth Sunday before Christmas), as a season ordinal.
inline int first_advent_ord(int y) {
  int w = dow(y, 12, 25);
  return 30 + 25 - (w == 0 ? 7 : w) - 21;
}

// Replaces every "{Y}" with the year.
inline std::string with_year(const char *text, int year) {
  std::string s(text);
  const std::string y = std::to_string(year);
  for (size_t p = s.find("{Y}"); p != std::string::npos; p = s.find("{Y}", p + y.size())) {
    s.replace(p, 3, y);
  }
  return s;
}

}  // namespace gct
