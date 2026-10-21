// SPDX-License-Identifier: MIT

#include "fx_santa.h"

namespace esphome {
namespace lvgl_canvas_fx {

using fxutil::CanvasView;
using fxutil::frand;
using fxutil::irand;
using fxutil::pack_rgb;

static constexpr float TWO_PI = 6.2831853f;

// Sleigh with Santa and two reindeer, facing left. Two frames for the running legs.
static constexpr int SPR_W = 27, SPR_H = 9;
static const char *const SPRITE[2][SPR_H] = {
    {
        "....................w......",
        "b.b.......b.b.......SS.....",
        "BBb.......BBb......SSSS....",
        "KBBBBBBBl.BBBBBBBl.SFFS....",
        "..BBBBBB.l..BBBBBB.wwww....",
        "..b...b...l.b...b.RwwwwRRR.",
        "..b...b....lllllllRRwwRRRR.",
        "..................RRRRRRRR.",
        "..................gg....gg.",
    },
    {
        "....................w......",
        "b.b.......b.b.......SS.....",
        "BBb.......BBb......SSSS....",
        "KBBBBBBBl.BBBBBBBl.SFFS....",
        "..BBBBBB.l..BBBBBB.wwww....",
        ".b.....b..l..b...b.RwwwwRRR",
        "b.......b..lllllllRRwwRRRR.",
        "..................RRRRRRRR.",
        "..................gg....gg.",
    },
};

static bool sprite_color(char ch, uint8_t &r, uint8_t &g, uint8_t &b) {
  switch (ch) {
    case 'B': r = 120; g = 70; b = 30; return true;   // reindeer body
    case 'b': r = 90; g = 55; b = 25; return true;    // antlers, legs
    case 'K': r = 60; g = 35; b = 20; return true;    // nose
    case 'l': r = 150; g = 110; b = 60; return true;  // reins
    case 'R': r = 200; g = 20; b = 20; return true;   // sleigh
    case 'S': r = 210; g = 25; b = 25; return true;   // hat and coat
    case 'w': r = 240; g = 240; b = 240; return true; // beard, fur, pompom
    case 'F': r = 240; g = 190; b = 150; return true; // face
    case 'g': r = 220; g = 180; b = 40; return true;  // runners
    default: return false;
  }
}

void FxSanta::on_resize(const Rect &r) {
  FxBase::on_resize(r);
  W_ = r.w;
  H_ = r.h;
  stars_.clear();
  if (W_ <= 0 || H_ <= 0)
    return;

  stars_.resize((size_t) std::max(6, W_ * H_ / STAR_DIV));
  for (auto &s : stars_) {
    s.x = (uint16_t) irand(0, W_ - 1);
    s.y = (uint16_t) irand(0, H_ - 1);
    s.speed = frand(0.8f, 2.5f);
    s.phase = frand(0.0f, TWO_PI);
  }
  t_ = 0.0f;
  new_flight_();
  ft_ = f_.dur;  // start with a pause, then the first flight
}

void FxSanta::new_flight_() {
  f_.to_right = irand(0, 1) == 1;
  f_.dur = frand(FLIGHT_S_MIN, FLIGHT_S_MAX);
  f_.pause = frand(PAUSE_S_MIN, PAUSE_S_MAX);
  f_.s0 = frand(SCALE_MIN, SCALE_MAX);
  f_.s1 = frand(SCALE_MIN, SCALE_MAX);
  // Vertical range so that the sleigh stays on the canvas and, where possible, above the clock
  const float hmax = (float) SPR_H * std::max(f_.s0, f_.s1);
  const float lo = hmax * 0.5f + 1.0f;
  const float hi = std::max(lo, (float) (H_ - CLEAR_BOTTOM) - hmax * 0.5f);
  f_.y0 = frand(lo, hi);
  f_.y1 = fxutil::clampf(f_.y0 + frand(-6.0f, 6.0f), lo, hi);
  f_.bob = frand(0.0f, TWO_PI);
  ft_ = 0.0f;
}

void FxSanta::step(float dt) {
  auto v = CanvasView::acquire(canvas_, area_);
  if (!v.ok() || v.w != W_ || v.h != H_ || stars_.empty())
    return;

  t_ += dt;
  if (t_ >= 3600.0f)
    t_ -= 3600.0f;

  v.clear_black();

  // Twinkling stars
  for (const auto &s : stars_) {
    const uint8_t b = (uint8_t) (40.0f + 80.0f * (0.5f + 0.5f * std::sin(t_ * s.speed + s.phase)));
    v.put_px(s.x, s.y, pack_rgb(b, b, (uint8_t) std::min(255, b + 10)));
  }

  // Crescent moon, top right
  const int mx = W_ - 11, my = 3;
  for (int y = 0; y < 6; ++y)
    for (int x = 0; x < 6; ++x) {
      const float a = (x - 2.5f) * (x - 2.5f) + (y - 2.5f) * (y - 2.5f);
      const float c = (x - 4.0f) * (x - 4.0f) + (y - 1.5f) * (y - 1.5f);
      if (a < 7.0f && c >= 5.0f)
        v.put_px(mx + x, my + y, pack_rgb(230, 220, 150));
    }

  // The sleigh
  ft_ += dt;
  if (ft_ >= f_.dur + f_.pause)
    new_flight_();
  if (ft_ < f_.dur) {
    const float u = ft_ / f_.dur;
    const float s = fxutil::lerpf(f_.s0, f_.s1, u);
    const int w = std::max(1, (int) std::lround((float) SPR_W * s));
    const int h = std::max(1, (int) std::lround((float) SPR_H * s));
    // From fully outside on one side to fully outside on the other (trail included)
    const float span = (float) (W_ + 2 * w + 24);
    float left = -(float) w - 12.0f + span * u;
    if (!f_.to_right)
      left = (float) W_ + 12.0f - span * u;
    const int sx = (int) std::lround(left);
    const float cy = fxutil::lerpf(f_.y0, f_.y1, u) + 1.5f * s * std::sin(t_ * 2.2f + f_.bob);
    const int sy = (int) std::lround(cy - (float) h * 0.5f);

    // Sparkle trail behind the sleigh
    const int dir = f_.to_right ? -1 : 1;
    const int tx0 = f_.to_right ? sx - 1 : sx + w;
    const int n = std::max(4, (int) (10.0f * s));
    for (int k = 0; k < n; ++k) {
      const int tx = tx0 + dir * k * 2;
      const int ty = sy + (int) std::lround((float) h * 0.8f + std::sin(t_ * 3.0f + (float) k));
      const float a = (1.0f - (float) k / (float) n) * (0.6f + 0.4f * std::sin(t_ * 9.0f + (float) k));
      if (a > 0.0f)
        v.put_px_add(tx, ty, (uint8_t) (255 * a), (uint8_t) (220 * a), (uint8_t) (120 * a));
    }

    // Sprite, scaled with nearest-neighbour sampling and mirrored when flying to the right
    const int frame = ((int) (t_ * 6.0f)) & 1;
    for (int j = 0; j < h; ++j) {
      const int src_y = std::min(SPR_H - 1, (int) ((float) j / s));
      const char *row = SPRITE[frame][src_y];
      for (int i = 0; i < w; ++i) {
        int src_x = std::min(SPR_W - 1, (int) ((float) i / s));
        if (f_.to_right)
          src_x = SPR_W - 1 - src_x;
        uint8_t r, g, b;
        if (sprite_color(row[src_x], r, g, b))
          v.put_px(sx + i, sy + j, pack_rgb(r, g, b));
      }
    }
  }

  v.invalidate(canvas_);
}

}  // namespace lvgl_canvas_fx
}  // namespace esphome
