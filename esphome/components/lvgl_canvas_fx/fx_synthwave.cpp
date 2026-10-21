// SPDX-License-Identifier: MIT

#include "fx_synthwave.h"

namespace esphome {
namespace lvgl_canvas_fx {

using fxutil::CanvasView;
using fxutil::frand;
using fxutil::irand;
using fxutil::pack_rgb;

void FxSynthwave::on_resize(const Rect &r) {
  FxBase::on_resize(r);
  W_ = r.w;
  H_ = r.h;
  stars_.clear();
  mountain_.clear();
  if (W_ <= 0 || H_ <= 0)
    return;
  hz_ = std::max(4, (int) ((float) H_ * HORIZON));

  stars_.resize((size_t) std::max(4, W_ * hz_ / STAR_DIV));
  for (auto &s : stars_) {
    s.x = (uint16_t) irand(0, W_ - 1);
    s.y = (uint16_t) irand(0, std::max(1, hz_ - 6));
    s.speed = frand(0.6f, 2.0f);
    s.phase = frand(0.0f, 6.2832f);
  }
  mountain_.resize((size_t) W_);
  for (int x = 0; x < W_; ++x)
    mountain_[x] = (uint8_t) (3.0f + 3.0f * std::fabs(std::sin(x * 0.21f)) + 2.0f * std::fabs(std::sin(x * 0.53f + 1.0f)));
}

void FxSynthwave::step(float dt) {
  auto v = CanvasView::acquire(canvas_, area_);
  if (!v.ok() || v.w != W_ || v.h != H_ || mountain_.empty())
    return;

  t_ += dt;
  if (t_ >= 3600.0f)
    t_ -= 3600.0f;

  // Sky gradient, deep violet to purple
  for (int y = 0; y < hz_; ++y) {
    const float u = (float) y / (float) hz_;
    const auto c = pack_rgb((uint8_t) (20 + 60 * u), (uint8_t) (5 + 10 * u), (uint8_t) (50 + 40 * u));
    for (int x = 0; x < W_; ++x)
      v.put_px(x, y, c);
  }
  for (const auto &s : stars_) {
    const uint8_t b = (uint8_t) (60.0f + 140.0f * (0.5f + 0.5f * std::sin(t_ * s.speed + s.phase)));
    v.put_px(s.x, s.y, pack_rgb(b, b, b));
  }

  // Striped sun sitting on the horizon; the gaps widen towards the bottom
  const int cx = W_ / 2, r = std::max(4, hz_ * 2 / 3);
  for (int y = hz_ - r; y < hz_; ++y) {
    const int band = y - (hz_ - r);
    if (band >= 5 && (band - 5) % 3 < 1 + (band - 5) / 4)
      continue;
    const float u = (float) band / (float) r;
    const auto c = pack_rgb(255, (uint8_t) (220 - 170 * u), (uint8_t) (60 + 80 * u));
    for (int x = cx - r; x <= cx + r; ++x)
      if ((x - cx) * (x - cx) + (y - hz_) * (y - hz_) <= r * r)
        v.put_px(x, y, c);
  }

  // Mountain silhouette, leaving the middle of the sun visible
  for (int x = 0; x < W_; ++x)
    for (int y = hz_ - mountain_[x]; y < hz_; ++y)
      if (std::abs(x - cx) > r - 2 || y > hz_ - 2)
        v.put_px(x, y, pack_rgb(40, 10, 70));

  // Floor
  for (int y = hz_ + 1; y < H_; ++y)
    for (int x = 0; x < W_; ++x)
      v.put_px(x, y, pack_rgb(14, 0, 30));

  auto grid = [this](int y) {
    const float a = std::min(1.0f, 0.3f + (float) (y - hz_) / (float) (H_ - hz_) * 1.1f);
    return pack_rgb((uint8_t) (255 * a), (uint8_t) (50 * a), (uint8_t) (210 * a));
  };

  // Cross lines rushing towards the viewer (only where they are at least two rows apart)
  const float K = 12.0f * (float) (H_ - hz_) / 15.0f;
  const float off = std::fmod(t_ * SPEED, 1.0f);
  for (int y = hz_ + 1; y < H_; ++y) {
    const int d = y - hz_;
    const float z0 = K / (float) d, z1 = K / (float) (d + 1);
    if (z0 - z1 <= 0.5f && std::floor(z0 + off) != std::floor(z1 + off)) {
      const auto c = grid(y);
      for (int x = 0; x < W_; ++x)
        v.put_px(x, y, c);
    }
  }

  // Lines towards the vanishing point, drawn as continuous lines
  for (int k = -GRID_LINES; k <= GRID_LINES; ++k) {
    const int x0 = cx + k * 2, y0 = hz_ + 1;
    const int x1 = cx + k * 14, y1 = H_ + 6;
    const int n = std::max(std::abs(x1 - x0), std::abs(y1 - y0));
    for (int i = 0; i <= n; ++i) {
      const int x = (int) std::lround(x0 + (float) (x1 - x0) * i / n);
      const int y = (int) std::lround(y0 + (float) (y1 - y0) * i / n);
      if (y > hz_ && y < H_)
        v.put_px(x, y, grid(y));
    }
  }

  // Glowing horizon line
  for (int x = 0; x < W_; ++x)
    v.put_px(x, hz_, pack_rgb(255, 120, 240));

  v.invalidate(canvas_);
}

}  // namespace lvgl_canvas_fx
}  // namespace esphome
