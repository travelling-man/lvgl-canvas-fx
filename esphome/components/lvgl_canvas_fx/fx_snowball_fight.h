// SPDX-License-Identifier: MIT
//
// Snowball fight around a clock overlay in the lower right corner.
// The left kid stands on the ground behind a snow wall. The right kid uses the
// clock as a fort: sometimes it throws from on top of the clock, sometimes
// from the ground next to it, and every now and then it climbs up or jumps
// down. Throws hit the cover (the target ducks), score a direct hit (the
// target falls over for a moment) or miss - all at random.

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
  static constexpr int CLOCK_W = 25;            // size of the clock overlay in the lower right corner
  static constexpr int CLOCK_H = 8;
  static constexpr float PAUSE_MIN_S = 0.6f;    // breather between two throws
  static constexpr float PAUSE_MAX_S = 2.2f;
  static constexpr float FLIGHT_MIN_S = 0.8f;   // flight time of a snowball
  static constexpr float FLIGHT_MAX_S = 1.3f;
  static constexpr int HIT_PERCENT = 30;        // direct hit
  static constexpr int MISS_PERCENT = 25;       // flies past; the rest hits the cover
  static constexpr int BOTH_PERCENT = 15;       // both throw at the same time
  static constexpr int MOVE_PERCENT = 20;       // chance per round that the right kid changes position
  static constexpr float WALK_SPEED = 9.0f;     // px/s
  static constexpr float CLIMB_SPEED = 5.0f;    // px/s
  static constexpr float WALL_REGROW_S = 6.0f;  // snow wall grows back one row after this long
  static constexpr int SNOW_DIV = 90;           // one snowflake per this many pixels

  enum Pose : uint8_t { IDLE, CROUCH, THROW, FALLEN };
  enum Outcome : uint8_t { COVER, HIT, MISS };
  enum Phase : uint8_t { PAUSE, MAKE, WINDUP, FLY, AFTER, MOVE };

  struct Waypoint {
    float x, feet, speed;
    bool jump;  // arc instead of a straight line
  };
  struct Kid {
    float x, feet;  // left edge of the sprite, row the feet stand on
    bool mirror;    // true = faces left
    uint8_t hat[3], coat[3];
    Pose pose{IDLE};
    float fallen{0};
    bool on_top{false};
    // snow wall (left kid only)
    bool has_wall{false};
    int wall_x0{0}, wall_x1{0}, wall_h{3};
    float regrow{0};
    // movement
    std::vector<Waypoint> path;
    float seg_t{0};
    float from_x{0}, from_feet{0};
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
  int ground_{0};     // row of the white ground line
  int clock_x0_{0};   // first column of the clock
  int clock_top_{0};  // first row of the clock
  float top_x_{0}, bottom_x_{0};  // the right kid's two positions
  float t_{0};
  Phase phase_{PAUSE};
  float phase_t_{0};
  bool thrower_[2]{false, false};
  Kid kids_[2];
  Ball balls_[2];
  std::vector<Particle> parts_;
  std::vector<Flake> snow_;

  void start_round_();
  void start_move_();
  bool update_move_(Kid &k, float dt);  // true while still moving
  void launch_(int from);
  void impact_(Ball &b);
  void burst_(float x, float y, int n);
  void draw_kid_(const fxutil::CanvasView &v, const Kid &k) const;
};

}  // namespace lvgl_canvas_fx
}  // namespace esphome
