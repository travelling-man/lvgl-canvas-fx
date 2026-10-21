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
    {"..HH.", "..HHH", "..FF.", ".CCCC", "CCCC.", ".CCC.", ".P.P.", ".P.P."},  // THROW
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

  gy_ = std::max(KID_H + 4, H_ - GROUND_FROM_BOTTOM);  // first snow row

  Kid &a = kids_[0];
  a.x = 3;
  a.mirror = false;
  a.hat[0] = 200; a.hat[1] = 30; a.hat[2] = 30;
  a.coat[0] = 40; a.coat[1] = 90; a.coat[2] = 200;
  a.wall_x0 = a.x + KID_W + 2;
  a.wall_x1 = a.wall_x0 + 5;

  Kid &b = kids_[1];
  b.x = W_ - 3 - KID_W;
  b.mirror = true;
  b.hat[0] = 30; b.hat[1] = 140; b.hat[2] = 60;
  b.coat[0] = 220; b.coat[1] = 120; b.coat[2] = 20;
  b.wall_x1 = b.x - 2;
  b.wall_x0 = b.wall_x1 - 5;

  for (auto &k : kids_) {
    k.pose = IDLE;
    k.fallen = 0;
    k.wall_h = 3;
    k.regrow = 0;
  }
  for (auto &bl : balls_)
    bl.active = false;

  snow_.resize((size_t) std::max(4, W_ * H_ / SNOW_DIV));
  for (auto &f : snow_) {
    f.x = frand(0.0f, (float) W_);
    f.y = frand(0.0f, (float) gy_);
    f.speed = frand(1.5f, 3.5f);
  }
  t_ = 0;
  phase_ = PAUSE;
  phase_t_ = frand(0.5f, 1.5f);
}

void FxSnowballFight::start_round_() {
  const bool both = irand(1, 100) <= BOTH_PERCENT;
  const int who = irand(0, 1);
  thrower_[0] = both || who == 0;
  thrower_[1] = both || who == 1;
  // Someone still lying in the snow cannot throw
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
  b.outcome = roll <= HIT_PERCENT ? HIT : (roll <= HIT_PERCENT + MISS_PERCENT ? MISS : WALL);
  b.from = from;
  b.x0 = (float) (k.mirror ? k.x : k.x + KID_W - 1);
  b.y0 = (float) (gy_ - KID_H + 3);
  switch (b.outcome) {
    case HIT:
      b.x1 = (float) target.x + 2.0f;
      b.y1 = (float) (gy_ - KID_H + 1);
      break;
    case WALL:
      b.x1 = (float) (target.wall_x0 + target.wall_x1) * 0.5f;
      b.y1 = (float) (gy_ - target.wall_h);
      break;
    default:  // MISS: sails past the target and off the canvas
      b.x1 = k.mirror ? -6.0f : (float) W_ + 5.0f;
      b.y1 = (float) (gy_ - KID_H) + frand(-3.0f, 2.0f);
      break;
  }
  b.arc = std::min(frand(6.0f, 11.0f), std::min(b.y0, b.y1) - 1.0f);
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
  } else if (b.outcome == WALL) {
    burst_(b.x1, b.y1, 7);
    if (target.wall_h > 1)
      target.wall_h--;
    target.regrow = WALL_REGROW_S;
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
  uint8_t r, g, b;
  if (k.pose == FALLEN) {
    // Lies on the ground, head pointing away from the wall
    const int y0 = gy_ - FALL_H;
    const int x0 = k.mirror ? k.x + KID_W - FALL_W + 2 : k.x - 2;
    for (int j = 0; j < FALL_H; ++j)
      for (int i = 0; i < FALL_W; ++i) {
        const int si = k.mirror ? FALL_W - 1 - i : i;
        if (color(FALLEN_SPR[j][si], r, g, b))
          v.put_px(x0 + (k.mirror ? FALL_W - 1 - si : si), y0 + j, pack_rgb(r, g, b));
      }
    return;
  }
  const char *const *spr = KID[k.pose];
  for (int j = 0; j < KID_H; ++j)
    for (int i = 0; i < KID_W; ++i) {
      const int si = k.mirror ? KID_W - 1 - i : i;
      if (color(spr[j][si], r, g, b))
        v.put_px(k.x + i, gy_ - KID_H + j, pack_rgb(r, g, b));
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
        // The target ducks behind its wall when the ball is about to hit the wall
        if (b.outcome == WALL && target.fallen <= 0.0f && b.t > b.dur * 0.4f && !thrower_[1 - b.from])
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
    if (k.wall_h < 3) {
      k.regrow -= dt;
      if (k.regrow <= 0.0f) {
        k.wall_h++;
        k.regrow = WALL_REGROW_S;
      }
    }
  }

  // ---- drawing ----
  v.clear_black();

  for (auto &f : snow_) {
    f.y += f.speed * dt;
    if (f.y >= (float) gy_) {
      f.y = 0.0f;
      f.x = frand(0.0f, (float) W_);
    }
    v.put_px((int) f.x, (int) f.y, pack_rgb(70, 70, 90));
  }

  // Ground: a single white line of snow, black below (room for the clock)
  for (int x = 0; x < W_; ++x)
    v.put_px(x, gy_, pack_rgb(200, 205, 225));

  // Snow walls
  for (const auto &k : kids_)
    for (int x = k.wall_x0; x < k.wall_x1; ++x)
      for (int h = 0; h < k.wall_h; ++h) {
        const int y = gy_ - 1 - h;
        v.put_px(x, y, h == k.wall_h - 1 ? pack_rgb(200, 205, 225) : pack_rgb(150, 155, 180));
      }

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
    if (p.life > 0.0f && p.y < (float) gy_)
      v.put_px((int) p.x, (int) p.y, pack_rgb(240, 240, 255));
  }
  parts_.erase(std::remove_if(parts_.begin(), parts_.end(),
                              [this](const Particle &p) { return p.life <= 0.0f || p.y >= (float) gy_; }),
               parts_.end());

  v.invalidate(canvas_);
}

}  // namespace lvgl_canvas_fx
}  // namespace esphome
