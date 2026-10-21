// SPDX-License-Identifier: MIT

#include "fx_snowball_fight.h"

namespace esphome {
namespace lvgl_canvas_fx {

using fxutil::CanvasView;
using fxutil::frand;
using fxutil::irand;
using fxutil::pack_rgb;

// Kid sprites, 5 x 8, facing right. H = hat, F = face, C = coat, P = trousers
static constexpr int KID_W = 5, KID_H = 8;
static const char *const KID[3][KID_H] = {
    {"..HH.", "..HHH", "..FF.", ".CCC.", "CCCCC", ".CCC.", ".P.P.", ".P.P."},  // IDLE
    {".....", ".....", ".....", "..HH.", "..HHH", "..FF.", ".CCCC", ".P.P."},  // CROUCH (forming / ducking)
    {"..HH.", "..HHH", "..FF.", ".CCCC", "CCCC.", ".CCC.", ".P.P.", ".P.P."},  // THROW (also climbing)
};
// Lying in the snow after a direct hit, 8 x 3, head on the left
static constexpr int FALL_W = 8, FALL_H = 3;
static const char *const FALLEN_SPR[FALL_H] = {"HH......", "HFCCCCPP", "..CC...."};

void FxSnowballFight::on_resize(const Rect &r) {
  FxBase::on_resize(r);
  W_ = r.w;
  H_ = r.h;
  parts_.clear();
  snow_.clear();
  if (W_ <= 0 || H_ <= 0)
    return;

  ground_ = H_ - 1;
  clock_x0_ = std::max(KID_W * 4, W_ - CLOCK_W);
  clock_top_ = std::max(KID_H + 4, H_ - CLOCK_H);

  Kid &a = kids_[0];
  a.x = 3;
  a.feet = (float) (ground_ - 1);
  a.mirror = false;
  a.hat[0] = 200; a.hat[1] = 30; a.hat[2] = 30;
  a.coat[0] = 40; a.coat[1] = 90; a.coat[2] = 200;
  a.has_wall = true;
  a.wall_x0 = (int) a.x + KID_W + 2;
  a.wall_x1 = a.wall_x0 + 5;

  // The right kid: on top of the clock, or on the ground just left of it
  top_x_ = (float) (clock_x0_ + CLOCK_W / 2 - KID_W / 2);
  bottom_x_ = (float) (clock_x0_ - KID_W - 4);
  Kid &b = kids_[1];
  b.mirror = true;
  b.hat[0] = 30; b.hat[1] = 140; b.hat[2] = 60;
  b.coat[0] = 220; b.coat[1] = 120; b.coat[2] = 20;
  b.has_wall = false;
  b.on_top = true;
  b.x = top_x_;
  b.feet = (float) (clock_top_ - 1);

  for (auto &k : kids_) {
    k.pose = IDLE;
    k.fallen = 0;
    k.wall_h = 3;
    k.regrow = 0;
    k.path.clear();
  }
  for (auto &bl : balls_)
    bl.active = false;

  snow_.resize((size_t) std::max(4, W_ * H_ / SNOW_DIV));
  for (auto &f : snow_) {
    f.x = frand(0.0f, (float) W_);
    f.y = frand(0.0f, (float) H_);
    f.speed = frand(1.5f, 3.5f);
  }
  t_ = 0;
  phase_ = PAUSE;
  phase_t_ = frand(0.5f, 1.5f);
}

void FxSnowballFight::start_move_() {
  Kid &k = kids_[1];
  k.path.clear();
  const float edge = (float) (clock_x0_ - KID_W);  // hugging the clock's left side
  const float top_feet = (float) (clock_top_ - 1), ground_feet = (float) (ground_ - 1);
  if (k.on_top) {
    // walk to the edge of the clock and jump down
    k.path.push_back({(float) clock_x0_ + 1.0f, top_feet, WALK_SPEED, false});
    k.path.push_back({bottom_x_, ground_feet, WALK_SPEED * 1.5f, true});
  } else {
    // walk to the clock, climb up its side, step onto the top
    k.path.push_back({edge, ground_feet, WALK_SPEED, false});
    k.path.push_back({edge, top_feet, CLIMB_SPEED, false});
    k.path.push_back({(float) clock_x0_ + 1.0f, top_feet, WALK_SPEED, false});
    k.path.push_back({top_x_, top_feet, WALK_SPEED, false});
  }
  k.on_top = !k.on_top;
  k.seg_t = 0;
  k.from_x = k.x;
  k.from_feet = k.feet;
  phase_ = MOVE;
}

bool FxSnowballFight::update_move_(Kid &k, float dt) {
  if (k.path.empty())
    return false;
  const Waypoint &w = k.path.front();
  const float dist = std::max(std::fabs(w.x - k.from_x), std::fabs(w.feet - k.from_feet));
  const float dur = std::max(0.15f, dist / w.speed);
  k.seg_t += dt;
  const float u = std::min(1.0f, k.seg_t / dur);
  k.x = k.from_x + (w.x - k.from_x) * u;
  k.feet = k.from_feet + (w.feet - k.from_feet) * u;
  if (w.jump)
    k.feet -= 5.0f * 4.0f * u * (1.0f - u);
  // Climbing (vertical move) and walking both swing the arms
  k.pose = (((int) (t_ * 7.0f)) & 1) ? THROW : IDLE;
  if (w.jump)
    k.pose = THROW;
  if (u >= 1.0f) {
    k.x = w.x;
    k.feet = w.feet;
    k.path.erase(k.path.begin());
    k.seg_t = 0;
    k.from_x = k.x;
    k.from_feet = k.feet;
    if (k.path.empty()) {
      k.pose = IDLE;
      return false;
    }
  }
  return true;
}

void FxSnowballFight::start_round_() {
  if (kids_[1].fallen <= 0.0f && irand(1, 100) <= MOVE_PERCENT) {
    start_move_();
    return;
  }
  const bool both = irand(1, 100) <= BOTH_PERCENT;
  const int who = irand(0, 1);
  thrower_[0] = both || who == 0;
  thrower_[1] = both || who == 1;
  for (int i = 0; i < 2; ++i)
    if (kids_[i].fallen > 0.0f)
      thrower_[i] = false;
  if (!thrower_[0] && !thrower_[1]) {
    phase_ = PAUSE;
    phase_t_ = 0.5f;
    return;
  }
  for (int i = 0; i < 2; ++i)
    if (thrower_[i])
      kids_[i].pose = CROUCH;  // forming the snowball
  phase_ = MAKE;
  phase_t_ = frand(0.5f, 0.9f);
}

void FxSnowballFight::launch_(int from) {
  Ball &b = balls_[from];
  const Kid &k = kids_[from];
  const Kid &target = kids_[1 - from];
  const int roll = irand(1, 100);
  b.outcome = roll <= HIT_PERCENT ? HIT : (roll <= HIT_PERCENT + MISS_PERCENT ? MISS : COVER);
  b.from = from;
  b.x0 = k.mirror ? k.x : k.x + KID_W - 1;
  b.y0 = k.feet - 4.0f;
  switch (b.outcome) {
    case HIT:
      b.x1 = target.x + 2.0f;
      b.y1 = target.feet - 6.0f;
      break;
    case COVER:
      if (target.has_wall) {  // the target's snow wall
        b.x1 = (float) (target.wall_x0 + target.wall_x1) * 0.5f;
        b.y1 = (float) (ground_ - target.wall_h);
      } else if (target.on_top) {  // the upper left corner of the clock
        b.x1 = (float) clock_x0_;
        b.y1 = (float) clock_top_ - 1.0f;
      } else {  // falls short into the snow in front of the target
        b.x1 = target.x - 3.0f;
        b.y1 = (float) ground_ - 1.0f;
      }
      break;
    default:  // MISS: sails past the target and off the canvas
      b.x1 = k.mirror ? -6.0f : (float) W_ + 5.0f;
      b.y1 = target.feet - 8.0f + frand(-3.0f, 2.0f);
      break;
  }
  b.arc = std::max(2.0f, std::min(frand(6.0f, 11.0f), std::min(b.y0, b.y1) - 1.0f));
  b.t = 0;
  b.dur = frand(FLIGHT_MIN_S, FLIGHT_MAX_S);
  b.active = true;
}

void FxSnowballFight::burst_(float x, float y, int n) {
  for (int i = 0; i < n; ++i) {
    const float a = frand(-3.0f, -0.15f);  // mostly upwards
    const float s = frand(6.0f, 16.0f);
    parts_.push_back(Particle{x, y, std::cos(a) * s, std::sin(a) * s, frand(0.35f, 0.7f)});
  }
}

void FxSnowballFight::impact_(Ball &b) {
  b.active = false;
  Kid &target = kids_[1 - b.from];
  if (b.outcome == HIT) {
    burst_(b.x1, b.y1, 10);
    target.pose = FALLEN;
    target.fallen = frand(1.2f, 2.0f);
  } else if (b.outcome == COVER) {
    burst_(b.x1, b.y1, 7);
    if (target.has_wall && target.wall_h > 1) {
      target.wall_h--;
      target.regrow = WALL_REGROW_S;
    }
  }
}

void FxSnowballFight::draw_kid_(const CanvasView &v, const Kid &k) const {
  auto color = [&k](char ch, uint8_t &r, uint8_t &g, uint8_t &b) {
    switch (ch) {
      case 'H': r = k.hat[0]; g = k.hat[1]; b = k.hat[2]; return true;
      case 'F': r = 240; g = 190; b = 150; return true;
      case 'C': r = k.coat[0]; g = k.coat[1]; b = k.coat[2]; return true;
      case 'P': r = 60; g = 60; b = 80; return true;
      default: return false;
    }
  };
  const int kx = (int) std::lround(k.x);
  const int feet = (int) std::lround(k.feet);
  uint8_t r, g, b;
  if (k.pose == FALLEN) {
    // Lies on the ground, head pointing away from the opponent
    const int y0 = feet - FALL_H + 1;
    const int x0 = k.mirror ? kx + KID_W - FALL_W + 2 : kx - 2;
    for (int j = 0; j < FALL_H; ++j)
      for (int i = 0; i < FALL_W; ++i) {
        const int si = k.mirror ? FALL_W - 1 - i : i;
        if (color(FALLEN_SPR[j][si], r, g, b))
          v.put_px(x0 + i, y0 + j, pack_rgb(r, g, b));
      }
    return;
  }
  const char *const *spr = KID[k.pose];
  for (int j = 0; j < KID_H; ++j)
    for (int i = 0; i < KID_W; ++i) {
      const int si = k.mirror ? KID_W - 1 - i : i;
      if (color(spr[j][si], r, g, b))
        v.put_px(kx + i, feet - KID_H + 1 + j, pack_rgb(r, g, b));
    }
}

void FxSnowballFight::step(float dt) {
  auto v = CanvasView::acquire(canvas_, area_);
  if (!v.ok() || v.w != W_ || v.h != H_ || snow_.empty())
    return;

  t_ += dt;

  // ---- choreography ----
  phase_t_ -= dt;
  switch (phase_) {
    case PAUSE:
      if (phase_t_ <= 0.0f)
        start_round_();
      break;
    case MOVE:
      if (!update_move_(kids_[1], dt)) {
        phase_ = PAUSE;
        phase_t_ = frand(0.4f, 1.0f);
      }
      break;
    case MAKE:
      if (phase_t_ <= 0.0f) {
        for (int i = 0; i < 2; ++i)
          if (thrower_[i])
            kids_[i].pose = THROW;
        phase_ = WINDUP;
        phase_t_ = 0.25f;
      }
      break;
    case WINDUP:
      if (phase_t_ <= 0.0f) {
        for (int i = 0; i < 2; ++i)
          if (thrower_[i])
            launch_(i);
        phase_ = FLY;
      }
      break;
    case FLY: {
      bool any = false;
      for (auto &b : balls_) {
        if (!b.active)
          continue;
        any = true;
        b.t += dt;
        Kid &target = kids_[1 - b.from];
        // The target ducks behind its cover when the ball is about to hit it
        if (b.outcome == COVER && target.fallen <= 0.0f && b.t > b.dur * 0.4f && !thrower_[1 - b.from])
          target.pose = CROUCH;
        if (b.t >= b.dur)
          impact_(b);
      }
      if (phase_t_ < -0.2f)
        for (int i = 0; i < 2; ++i)
          if (thrower_[i] && kids_[i].fallen <= 0.0f)
            kids_[i].pose = IDLE;
      if (!any) {
        phase_ = AFTER;
        phase_t_ = 0.5f;
      }
      break;
    }
    case AFTER:
      if (phase_t_ <= 0.0f) {
        for (auto &k : kids_)
          if (k.fallen <= 0.0f)
            k.pose = IDLE;
        phase_ = PAUSE;
        phase_t_ = frand(PAUSE_MIN_S, PAUSE_MAX_S);
      }
      break;
  }

  for (auto &k : kids_) {
    if (k.fallen > 0.0f) {
      k.fallen -= dt;
      if (k.fallen <= 0.0f)
        k.pose = IDLE;  // gets back up
    }
    if (k.has_wall && k.wall_h < 3) {
      k.regrow -= dt;
      if (k.regrow <= 0.0f) {
        k.wall_h++;
        k.regrow = WALL_REGROW_S;
      }
    }
  }

  // ---- drawing ----
  v.clear_black();

  // Snowfall, settling on the ground or on the clock
  for (auto &f : snow_) {
    f.y += f.speed * dt;
    const int floor = (int) f.x >= clock_x0_ ? clock_top_ - 1 : ground_;
    if (f.y >= (float) floor) {
      f.y = 0.0f;
      f.x = frand(0.0f, (float) W_);
    }
    v.put_px((int) f.x, (int) f.y, pack_rgb(70, 70, 90));
  }

  // Ground: a white line left of the clock, and a snow cap on top of the clock
  const auto snow_white = pack_rgb(200, 205, 225);
  for (int x = 0; x < clock_x0_; ++x)
    v.put_px(x, ground_, snow_white);
  for (int x = clock_x0_; x < W_; ++x)
    v.put_px(x, clock_top_, snow_white);

  // Snow wall of the left kid
  for (const auto &k : kids_)
    if (k.has_wall)
      for (int x = k.wall_x0; x < k.wall_x1; ++x)
        for (int h = 0; h < k.wall_h; ++h)
          v.put_px(x, ground_ - 1 - h, h == k.wall_h - 1 ? snow_white : pack_rgb(150, 155, 180));

  for (const auto &k : kids_)
    draw_kid_(v, k);

  // Snowballs in flight (2 x 2)
  for (const auto &b : balls_) {
    if (!b.active)
      continue;
    const float u = b.t / b.dur;
    const int x = (int) std::lround(b.x0 + (b.x1 - b.x0) * u);
    const int y = (int) std::lround(b.y0 + (b.y1 - b.y0) * u - b.arc * 4.0f * u * (1.0f - u));
    const auto white = pack_rgb(255, 255, 255);
    v.put_px(x, y, white);
    v.put_px(x + 1, y, white);
    v.put_px(x, y + 1, white);
    v.put_px(x + 1, y + 1, white);
  }

  // Splash particles with gravity
  for (auto &p : parts_) {
    p.life -= dt;
    p.vy += 40.0f * dt;
    p.x += p.vx * dt;
    p.y += p.vy * dt;
    if (p.life > 0.0f)
      v.put_px((int) p.x, (int) p.y, pack_rgb(240, 240, 255));
  }
  parts_.erase(std::remove_if(parts_.begin(), parts_.end(),
                              [this](const Particle &p) { return p.life <= 0.0f || p.y >= (float) H_; }),
               parts_.end());

  v.invalidate(canvas_);
}

}  // namespace lvgl_canvas_fx
}  // namespace esphome
