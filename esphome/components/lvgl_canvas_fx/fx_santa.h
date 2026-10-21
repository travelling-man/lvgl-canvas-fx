// SPDX-License-Identifier: MIT
//
// Santa in his sleigh, pulled by two reindeer, flying across a starry night sky
// with a crescent moon and leaving a trail of sparkles.

#pragma once
#include "fx_base.h"
#include "fx_util.h"
#include <vector>

namespace esphome {
namespace lvgl_canvas_fx {

class FxSanta : public FxBase {
 public:
  void on_resize(const Rect &r) override;
  void step(float dt) override;

 private:
  // ---- tuning ----
  // Every flight picks its own random direction, height, speed and size.
  static constexpr float FLIGHT_S_MIN = 6.0f;   // seconds to cross the canvas (fastest)
  static constexpr float FLIGHT_S_MAX = 12.0f;  // seconds to cross the canvas (slowest)
  static constexpr float PAUSE_S_MIN = 1.5f;    // empty sky between two flights
  static constexpr float PAUSE_S_MAX = 6.0f;
  static constexpr float SCALE_MIN = 0.6f;      // smallest size (far away)
  static constexpr float SCALE_MAX = 1.6f;      // largest size (close by)
  static constexpr int CLEAR_BOTTOM = 8;        // rows at the bottom kept free if possible (clock)
  static constexpr int STAR_DIV = 90;           // one star per this many pixels

  struct Star {
    uint16_t x, y;
    float speed, phase;
  };

  struct Flight {
    bool to_right;           // false = right to left
    float dur, pause;        // seconds
    float s0, s1;            // size at start and end (grows = coming closer, shrinks = flying away)
    float y0, y1;            // vertical centre at start and end
    float bob;               // phase of the up-and-down motion
  };

  int W_{0}, H_{0};
  float t_{0};
  float ft_{0};              // time since this flight started
  Flight f_{};

  void new_flight_();
  std::vector<Star> stars_;
};

}  // namespace lvgl_canvas_fx
}  // namespace esphome
