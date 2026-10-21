// SPDX-License-Identifier: MIT
//
// A string of fairy lights hanging in loops from the top edge, bulbs glowing
// one after another, with light snowfall behind.

#pragma once
#include "fx_base.h"
#include "fx_util.h"
#include <vector>

namespace esphome {
namespace lvgl_canvas_fx {

class FxFairyLights : public FxBase {
 public:
  void on_resize(const Rect &r) override;
  void step(float dt) override;

 private:
  // ---- tuning ----
  static constexpr int LOOP_WIDTH = 21;     // approx. width of one hanging loop in pixels
  static constexpr int BULBS_PER_LOOP = 3;
  static constexpr float SAG = 0.34f;       // how far a loop hangs down, as a fraction of the height
  static constexpr float CHASE_HZ = 0.29f;  // speed of the glow running along the string
  static constexpr int SNOW_DIV = 80;       // one snowflake per this many pixels

  struct Bulb {
    int x, y;  // position of the cap; the glass hangs below it
    uint8_t r, g, b;
  };
  struct Flake {
    float x, y, speed;
  };

  int W_{0}, H_{0};
  float t_{0};
  std::vector<int16_t> wire_y_;  // y of the wire per column
  std::vector<Bulb> bulbs_;
  std::vector<Flake> snow_;
};

}  // namespace lvgl_canvas_fx
}  // namespace esphome
