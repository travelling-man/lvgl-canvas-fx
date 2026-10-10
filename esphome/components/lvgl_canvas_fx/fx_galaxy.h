// SPDX-License-Identifier: MIT
//
// Spiral galaxy seen at an angle, slowly rotating around its core.

#pragma once
#include "fx_base.h"
#include "fx_util.h"
#include <vector>

namespace esphome {
namespace lvgl_canvas_fx {

class FxGalaxy : public FxBase {
 public:
  void on_resize(const Rect &r) override;
  void step(float dt) override;

 private:
  // ---- tuning ----
  static constexpr float PERIOD_S = 30.0f;  // seconds per full rotation
  static constexpr float TILT = 0.21f;      // in-plane rotation of the disc (rad)
  static constexpr float SQUASH = 0.48f;    // viewing angle: 1.0 = face-on, smaller = flatter
  static constexpr int ARMS = 4;            // number of spiral arms
  static constexpr float WIND = 6.0f;       // how tightly the arms are wound
  static constexpr int ARM_STAR_DIV = 30;   // one arm star per this many pixels
  static constexpr int BACK_STAR_DIV = 128; // one background star per this many pixels

  struct ArmStar {
    float r, a;         // radius, angle in the rotating frame
    float speed, phase; // twinkle
  };
  struct BackStar {
    uint16_t x, y;
    float speed, phase;
  };

  int W_{0}, H_{0};
  float radius_{0};  // disc radius in pixels
  float rot_{0};     // current rotation (rad)
  float t_{0};       // twinkle clock (s)
  float ct_{1}, st_{0};
  std::vector<float> phase_, disk_, core_;  // per pixel
  std::vector<uint8_t> col_;                // per pixel arm colour (r,g,b)
  std::vector<ArmStar> arm_stars_;
  std::vector<BackStar> back_stars_;
};

}  // namespace lvgl_canvas_fx
}  // namespace esphome
