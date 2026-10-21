// SPDX-License-Identifier: MIT
//
// Christmas tree with twinkling baubles, a pulsing star and gentle snowfall.

#pragma once
#include "fx_base.h"
#include "fx_util.h"
#include <vector>

namespace esphome {
namespace lvgl_canvas_fx {

class FxChristmasTree : public FxBase {
 public:
  void on_resize(const Rect &r) override;
  void step(float dt) override;

 private:
  // ---- tuning ----
  static constexpr int BAUBLES = 16;         // number of baubles on the tree
  static constexpr int SNOW_DIV = 50;        // one snowflake per this many pixels
  static constexpr float TWINKLE_HZ = 0.4f;  // average bauble twinkle speed

  struct Bauble {
    int x, y;
    uint8_t r, g, b;
    float speed, phase;
  };
  struct Flake {
    float x, y, speed;
  };

  int W_{0}, H_{0};
  int cx_{0}, top_{0}, base_{0};
  float t_{0};
  std::vector<Bauble> baubles_;
  std::vector<Flake> snow_;

  int half_width_(int y) const;  // half width of the tree at row y (-1 = outside)
};

}  // namespace lvgl_canvas_fx
}  // namespace esphome
