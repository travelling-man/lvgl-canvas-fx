// SPDX-License-Identifier: MIT

#include "fx_christmas_tree.h"

namespace esphome {
namespace lvgl_canvas_fx {

using fxutil::CanvasView;
using fxutil::frand;
using fxutil::irand;
using fxutil::pack_rgb;

static constexpr float TWO_PI = 6.2831853f;
static const uint8_t BAUBLE_COLORS[][3] = {
    {255, 40, 30}, {255, 180, 20}, {40, 120, 255}, {230, 60, 220}, {255, 255, 255},
};

int FxChristmasTree::half_width_(int y) const {
  if (y < top_ || y > base_)
    return -1;
  const int d = y - top_;
  // Widens downwards, with a small step every few rows so the branches read as tiers
  return (int) ((float) d * 0.55f) + (d % 7) / 3;
}

void FxChristmasTree::on_resize(const Rect &r) {
  FxBase::on_resize(r);
  W_ = r.w;
  H_ = r.h;
  baubles_.clear();
  snow_.clear();
  if (W_ <= 0 || H_ <= 0)
    return;

  top_ = std::max(2, H_ / 8);
  base_ = H_ - 5;
  // Keep the widest row (plus a pixel of air) on the canvas
  const int widest = half_width_(base_) + 2;
  cx_ = std::max(widest, std::min(W_ - 1 - widest, (int) std::lround((float) W_ * TREE_X)));

  baubles_.resize(BAUBLES);
  for (size_t i = 0; i < baubles_.size(); ++i) {
    auto &b = baubles_[i];
    b.y = irand(top_ + 3, base_ - 1);
    const int hw = half_width_(b.y);
    b.x = cx_ + irand(-hw + 1, hw - 1);
    const auto &c = BAUBLE_COLORS[i % 5];
    b.r = c[0];
    b.g = c[1];
    b.b = c[2];
    b.speed = TWINKLE_HZ * frand(0.6f, 1.6f);
    b.phase = frand(0.0f, 1.0f);
  }

  snow_.resize((size_t) std::max(6, W_ * H_ / SNOW_DIV));
  for (auto &f : snow_) {
    f.x = frand(0.0f, (float) W_);
    f.y = frand(0.0f, (float) H_);
    f.speed = frand(1.5f, 4.0f);
  }
  t_ = 0.0f;
}

void FxChristmasTree::step(float dt) {
  auto v = CanvasView::acquire(canvas_, area_);
  if (!v.ok() || v.w != W_ || v.h != H_ || baubles_.empty())
    return;

  t_ += dt;
  if (t_ >= 3600.0f)
    t_ -= 3600.0f;

  v.clear_black();

  // Snow in the background, ground line at the bottom
  for (auto &f : snow_) {
    f.y += f.speed * dt;
    if (f.y >= (float) (H_ - 1)) {
      f.y = 0.0f;
      f.x = frand(0.0f, (float) W_);
    }
    v.put_px((int) f.x, (int) f.y, pack_rgb(90, 90, 110));
  }
  for (int x = 0; x < W_; ++x)
    v.put_px(x, H_ - 1, pack_rgb(150, 150, 170));

  // Tree: slightly checkered green so it does not look flat
  for (int y = top_; y <= base_; ++y) {
    const int hw = half_width_(y);
    for (int x = cx_ - hw; x <= cx_ + hw; ++x) {
      const uint8_t g = ((x + y) % 3 == 0) ? 110 : 80;
      v.put_px(x, y, pack_rgb(0, g, g / 3));
    }
  }
  // Trunk
  for (int y = base_ + 1; y < H_ - 1; ++y)
    for (int x = cx_ - 2; x < cx_ + 2; ++x)
      v.put_px(x, y, pack_rgb(90, 50, 20));

  // Baubles glow up and down independently
  for (auto &b : baubles_) {
    b.phase += b.speed * dt;
    if (b.phase >= 1.0f)
      b.phase -= 1.0f;
    const float a = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(b.phase * TWO_PI));
    v.put_px(b.x, b.y, pack_rgb((uint8_t) (b.r * a), (uint8_t) (b.g * a), (uint8_t) (b.b * a)));
  }

  // Star on top, gently pulsing
  const float s = 0.6f + 0.4f * std::sin(t_ * 3.0f);
  const auto star = pack_rgb((uint8_t) (255 * s), (uint8_t) (220 * s), (uint8_t) (60 * s));
  const int sy = top_ - 1;
  v.put_px(cx_, sy, star);
  v.put_px(cx_ - 1, sy, star);
  v.put_px(cx_ + 1, sy, star);
  v.put_px(cx_, sy - 1, star);
  v.put_px(cx_, sy + 1, star);

  v.invalidate(canvas_);
}

}  // namespace lvgl_canvas_fx
}  // namespace esphome
