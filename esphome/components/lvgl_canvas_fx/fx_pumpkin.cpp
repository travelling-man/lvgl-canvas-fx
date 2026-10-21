// SPDX-License-Identifier: MIT

#include "fx_pumpkin.h"

namespace esphome {
namespace lvgl_canvas_fx {

using fxutil::CanvasView;
using fxutil::frand;
using fxutil::irand;
using fxutil::pack_rgb;

static constexpr float TWO_PI = 6.2831853f;

// Pumpkin, 21 x 17. O/o = orange skin (light/dark rib), G = stem,
// F = carved face (glows with the candle), . = transparent
static constexpr int PK_W = 21, PK_H = 17;
static const char *const PUMPKIN[PK_H] = {
    "..........GG.........",
    ".........GG..........",
    "....OOOooOOOooOOO....",
    "..OOOOoOOOOOOoOOOOO..",
    ".OOOOoOOOOOOOOoOOOOO.",
    ".OOOoOFOOOOOOOFOoOOO.",
    "OOOOoFFFOOOOOFFFoOOOO",
    "OOOOFFFFFOOOFFFFFOOOO",
    "OOOOoOOOOOFOOOOOoOOOO",
    "OOOOoOOOOFFFOOOOoOOOO",
    "OOOOoOOOOOOOOOOOoOOOO",
    "OOOFoOOOOOOOOOOOoFOOO",
    "OOOOFFFFFFFFFFFFFOOOO",
    ".OOOoFFOFFFFFOFFoOOO.",
    "..OOOoOFFOOOFFOoOOO..",
    "....OOOooOOOooOOO....",
    ".......ooooooo.......",
};

// Bat, two wing positions, 7 x 3
static const char *const BAT[2][3] = {
    {"B.....B", "BB.B.BB", "..BBB.."},
    {".......", "..BBB..", "BBB.BBB"},
};

void FxPumpkin::on_resize(const Rect &r) {
  FxBase::on_resize(r);
  W_ = r.w;
  H_ = r.h;
  bats_.clear();
  stars_.clear();
  if (W_ <= 0 || H_ <= 0)
    return;

  cx_ = std::max(PK_W / 2 + 1, std::min(W_ - PK_W / 2 - 2, (int) std::lround((float) W_ * PUMPKIN_X)));
  cy_ = H_ - 2 - PK_H / 2;

  stars_.resize((size_t) std::max(4, W_ * H_ / STAR_DIV));
  for (auto &s : stars_) {
    s.x = (uint16_t) irand(0, W_ - 1);
    s.y = (uint16_t) irand(0, std::max(1, H_ / 2));
    s.speed = frand(0.8f, 2.5f);
    s.phase = frand(0.0f, TWO_PI);
  }
  bats_.resize(BATS);
  for (auto &b : bats_)
    reset_bat_(b, true);
  t_ = 0.0f;
}

void FxPumpkin::reset_bat_(Bat &b, bool first) {
  const bool to_right = irand(0, 1) == 1;
  b.vx = (to_right ? 1.0f : -1.0f) * frand(6.0f, 12.0f);
  b.x = to_right ? -8.0f : (float) W_ + 1.0f;
  b.y = frand(1.0f, (float) H_ * 0.45f);
  b.phase = frand(0.0f, TWO_PI);
  b.wait = first ? frand(1.0f, 6.0f) : frand(3.0f, 12.0f);
}

void FxPumpkin::step(float dt) {
  auto v = CanvasView::acquire(canvas_, area_);
  if (!v.ok() || v.w != W_ || v.h != H_ || stars_.empty())
    return;

  t_ += dt;
  if (t_ >= 3600.0f)
    t_ -= 3600.0f;

  v.clear_black();

  // Stars
  for (const auto &s : stars_) {
    const uint8_t b = (uint8_t) (30.0f + 60.0f * (0.5f + 0.5f * std::sin(t_ * s.speed + s.phase)));
    v.put_px(s.x, s.y, pack_rgb(b, b, b));
  }

  // Full moon, upper right, with a faint halo
  const int mx = W_ - 12, my = 7;
  for (int y = -6; y <= 6; ++y)
    for (int x = -6; x <= 6; ++x) {
      const float d = std::sqrt((float) (x * x + y * y));
      if (d <= 3.6f) {
        const bool crater = (x == -1 && y == -1) || (x == 1 && y == 1) || (x == 2 && y == -2);
        v.put_px(mx + x, my + y, crater ? pack_rgb(170, 165, 130) : pack_rgb(225, 220, 175));
      } else if (d <= 5.5f) {
        const uint8_t a = (uint8_t) (18.0f * (5.5f - d));
        v.put_px_add(mx + x, my + y, a, a, (uint8_t) (a * 0.8f));
      }
    }

  // Bats
  for (auto &b : bats_) {
    if (b.wait > 0.0f) {
      b.wait -= dt;
      continue;
    }
    b.x += b.vx * dt;
    const int frame = ((int) (t_ * 8.0f + b.phase)) & 1;
    const int bx = (int) b.x, by = (int) (b.y + 1.5f * std::sin(t_ * 3.0f + b.phase));
    for (int j = 0; j < 3; ++j)
      for (int i = 0; i < 7; ++i)
        if (BAT[frame][j][i] == 'B')
          v.put_px(bx + i, by + j, pack_rgb(120, 90, 150));
    if (b.x < -10.0f || b.x > (float) W_ + 3.0f)
      reset_bat_(b, false);
  }

  // Candle flicker: wanders towards a new random brightness every few frames
  flicker_timer_ -= dt;
  if (flicker_timer_ <= 0.0f) {
    flicker_target_ = frand(0.45f, 1.0f);
    flicker_timer_ = frand(0.05f, 0.18f);
  }
  flicker_ += (flicker_target_ - flicker_) * std::min(1.0f, dt * 12.0f);
  const float f = flicker_;

  // Warm glow on the ground around the pumpkin
  for (int dx = -PK_W; dx <= PK_W; ++dx) {
    const float a = f * 0.6f * (1.0f - (float) std::abs(dx) / (float) PK_W);
    if (a > 0.0f)
      v.put_px_add(cx_ + dx, H_ - 1, (uint8_t) (120 * a), (uint8_t) (55 * a), 0);
  }

  // The pumpkin
  const int x0 = cx_ - PK_W / 2, y0 = cy_ - PK_H / 2;
  for (int j = 0; j < PK_H; ++j)
    for (int i = 0; i < PK_W; ++i) {
      const int x = x0 + i, y = y0 + j;
      switch (PUMPKIN[j][i]) {
        case 'O': v.put_px(x, y, pack_rgb(200, 80, 0)); break;
        case 'o': v.put_px(x, y, pack_rgb(120, 40, 0)); break;
        case 'G': v.put_px(x, y, pack_rgb(60, 120, 30)); break;
        case 'F':
          v.put_px(x, y, pack_rgb((uint8_t) (255 * f), (uint8_t) (235 * f), (uint8_t) (110 * f)));
          break;
        default: break;
      }
    }

  v.invalidate(canvas_);
}

}  // namespace lvgl_canvas_fx
}  // namespace esphome
