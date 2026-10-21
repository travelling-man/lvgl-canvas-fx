// SPDX-License-Identifier: MIT
//
// Snowball fight between two kids standing diagonally opposite each other:
// one in the lower left, one in the upper right (above a clock overlay in the
// lower right corner), each behind a small snow wall. Throws hit the wall (the
// target ducks), score a direct hit (the target falls over for a moment) or
// miss - all at random. No visible ground, the kids stand on thin air.

#pragma once
#include "fx_base.h"
#include "fx_util.h"
#include <vector>

namespace esphome {
namespace lvgl_canvas_fx {

class FxSnowballFight : public FxBase {
 public:
  void on_resize(const Rect &r) override;
  void step(float dt) override;

 private:
  // ---- tuning ----
  static constexpr float UPPER_FEET = 0.34f;    // feet row of the upper right kid, fraction of the height
  static constexpr float PAUSE_MIN_S = 0.6f;    // breather between two throws
  static constexpr float PAUSE_MAX_S = 2.2f;
  static constexpr float FLIGHT_MIN_S = 0.8f;   // flight time of a snowball
  static constexpr float FLIGHT_MAX_S = 1.3f;
  static constexpr int HIT_PERCENT = 30;        // direct hit
  static constexpr int MISS_PERCENT = 25;       // flies past; the rest hits the cover
  static constexpr int BOTH_PERCENT = 15;       // both throw at the same time
  static constexpr float WALL_REGROW_S = 6.0f;  // snow wall grows back one row after this long
  static constexpr int SNOW_DIV = 90;           // one snowflake per this many pixels

  enum Pose : uint8_t { IDLE, CROUCH, THROW, FALLEN };
  enum Outcome : uint8_t { COVER, HIT, MISS };
  enum Phase : uint8_t { PAUSE, MAKE, WINDUP, FLY, AFTER };

  struct Kid {
    float x, feet;  // left edge of the sprite, row the feet stand on
    bool mirror;    // true = faces left
    uint8_t hat[3], coat[3];
    Pose pose{IDLE};
    float fallen{0};
    // snow wall in front of the kid, standing on row wall_base
    int wall_x0{0}, wall_x1{0}, wall_base{0}, wall_h{3};
    float regrow{0};
  };
  struct Ball {
    bool active{false};
    int from;
    Outcome outcome;
    float x0, y0, x1, y1, arc, t, dur;
  };
  struct Particle {
    float x, y, vx, vy, life;
  };
  struct Flake {
    float x, y, speed;
  };

  int W_{0}, H_{0};
  float t_{0};
  Phase phase_{PAUSE};
  float phase_t_{0};
  bool thrower_[2]{false, false};
  Kid kids_[2];
  Ball balls_[2];
  std::vector<Particle> parts_;
  std::vector<Flake> snow_;

  void start_round_();
  void launch_(int from);
  void impact_(Ball &b);
  void burst_(float x, float y, int n);
  void draw_kid_(const fxutil::CanvasView &v, const Kid &k) const;
};

}  // namespace lvgl_canvas_fx
}  // namespace esphome
