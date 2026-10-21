// SPDX-License-Identifier: MIT
//
// Two kids in a snowball fight, each behind a small snow wall. They form
// snowballs, throw them in an arc and duck, hit the wall, score a direct hit
// (the other one falls over for a moment) or miss completely - all at random.

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
  static constexpr int GROUND_FROM_BOTTOM = 9;  // ground height above the bottom edge (room for the clock)
  static constexpr float PAUSE_MIN_S = 0.6f;    // breather between two throws
  static constexpr float PAUSE_MAX_S = 2.2f;
  static constexpr float FLIGHT_MIN_S = 0.8f;   // flight time of a snowball
  static constexpr float FLIGHT_MAX_S = 1.3f;
  static constexpr int HIT_PERCENT = 30;        // direct hit
  static constexpr int MISS_PERCENT = 25;       // flies past; the rest hits the wall
  static constexpr int BOTH_PERCENT = 15;       // both throw at the same time
  static constexpr float WALL_REGROW_S = 6.0f;  // snow wall grows back one row after this long
  static constexpr int SNOW_DIV = 90;           // one snowflake per this many pixels

  enum Pose : uint8_t { IDLE, CROUCH, THROW, FALLEN };
  enum Outcome : uint8_t { WALL, HIT, MISS };
  enum Phase : uint8_t { PAUSE, MAKE, WINDUP, FLY, AFTER };

  struct Kid {
    int x;                 // left edge of the sprite
    bool mirror;           // true = faces left
    uint8_t hat[3], coat[3];
    Pose pose{IDLE};
    float fallen{0};       // seconds left lying in the snow
    int wall_x0, wall_x1;  // snow wall in front of the kid
    int wall_h{3};
    float regrow{0};
  };
  struct Ball {
    bool active{false};
    int from;              // index of the throwing kid
    Outcome outcome;
    float x0, y0, x1, y1, arc, t, dur;
  };
  struct Particle {
    float x, y, vx, vy, life;
  };
  struct Flake {
    float x, y, speed;
  };

  int W_{0}, H_{0}, gy_{0};
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
