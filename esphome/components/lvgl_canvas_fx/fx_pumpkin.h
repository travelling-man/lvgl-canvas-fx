// SPDX-License-Identifier: MIT
//
// Halloween: a carved pumpkin with a flickering candle glowing through its
// face, under a full moon, with the odd bat fluttering across the sky.

#pragma once
#include "fx_base.h"
#include "fx_util.h"
#include <vector>

namespace esphome {
namespace lvgl_canvas_fx {

class FxPumpkin : public FxBase {
 public:
  void on_resize(const Rect &r) override;
  void step(float dt) override;

 private:
  // ---- tuning ----
  static constexpr float PUMPKIN_X = 0.25f;  // horizontal position: 0 = left edge, 0.5 = centre
  static constexpr int BATS = 2;             // bats in the sky at most
  static constexpr int STAR_DIV = 120;       // one star per this many pixels

  struct Bat {
    float x, y, vx, phase;
    float wait;  // seconds until it (re)appears
  };
  struct Star {
    uint16_t x, y;
    float speed, phase;
  };

  int W_{0}, H_{0};
  int cx_{0}, cy_{0};  // pumpkin centre
  float t_{0};
  float flicker_{1.0f}, flicker_target_{1.0f}, flicker_timer_{0};
  std::vector<Bat> bats_;
  std::vector<Star> stars_;

  void reset_bat_(Bat &b, bool first);
};

}  // namespace lvgl_canvas_fx
}  // namespace esphome
