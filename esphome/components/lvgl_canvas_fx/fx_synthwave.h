// SPDX-License-Identifier: MIT
//
// Retro synthwave drive: a striped sun setting behind a mountain silhouette,
// twinkling stars, and a neon grid racing towards the viewer.

#pragma once
#include "fx_base.h"
#include "fx_util.h"
#include <vector>

namespace esphome {
namespace lvgl_canvas_fx {

class FxSynthwave : public FxBase {
 public:
  void on_resize(const Rect &r) override;
  void step(float dt) override;

 private:
  // ---- tuning ----
  static constexpr float SPEED = 2.0f;     // grid lines per second
  static constexpr float HORIZON = 0.53f;  // horizon row, fraction of the height
  static constexpr int GRID_LINES = 5;     // receding grid lines on each side of the centre
  static constexpr int STAR_DIV = 90;      // one star per this many sky pixels

  struct Star {
    uint16_t x, y;
    float speed, phase;
  };

  int W_{0}, H_{0}, hz_{0};
  float t_{0};
  std::vector<Star> stars_;
  std::vector<uint8_t> mountain_;  // mountain height per column
};

}  // namespace lvgl_canvas_fx
}  // namespace esphome
