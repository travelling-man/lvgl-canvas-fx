// SPDX-License-Identifier: MIT

#include "fx_fairy_lights.h"

namespace esphome {
namespace lvgl_canvas_fx {

using fxutil::CanvasView;
using fxutil::frand;
using fxutil::pack_rgb;

static constexpr float TWO_PI = 6.2831853f;
static const uint8_t BULB_COLORS[][3] = {
    {255, 40, 30}, {255, 180, 20}, {40, 120, 255}, {40, 220, 60}, {230, 60, 220},
};

void FxFairyLights::on_resize(const Rect &r) {
  FxBase::on_resize(r);
  W_ = r.w;
  H_ = r.h;
  wire_y_.clear();
  bulbs_.clear();
  snow_.clear();
  if (W_ <= 0 || H_ <= 0)
    return;

  const int loops = std::max(1, (W_ + LOOP_WIDTH / 2) / LOOP_WIDTH);
  const float sag = (float) H_ * SAG;
  wire_y_.assign((size_t) W_, 0);
  int color = 0;
  for (int l = 0; l < loops; ++l) {
    const int a = l * (W_ - 1) / loops;
    const int b = (l + 1) * (W_ - 1) / loops;
    // Each loop hangs as a parabola between its two fixing points on the top edge
    for (int x = a; x <= b; ++x) {
      const float u = (float) (x - a) / (float) (b - a);
      wire_y_[x] = (int16_t) std::lround(1.0f + sag * 4.0f * u * (1.0f - u));
    }
    for (int k = 0; k < BULBS_PER_LOOP; ++k) {
      const float u = (float) (k + 1) / (float) (BULBS_PER_LOOP + 1) * 1.2f - 0.1f;
      const int x = (int) std::lround((float) a + u * (float) (b - a));
      const auto &c = BULB_COLORS[color++ % 5];
      bulbs_.push_back(Bulb{x, 0, c[0], c[1], c[2]});
    }
  }
  for (auto &bl : bulbs_)
    bl.y = wire_y_[fxutil::clampi(bl.x, 0, W_ - 1)];

  snow_.resize((size_t) std::max(4, W_ * H_ / SNOW_DIV));
  for (auto &f : snow_) {
    f.x = frand(0.0f, (float) W_);
    f.y = frand(0.0f, (float) H_);
    f.speed = frand(1.5f, 3.5f);
  }
  t_ = 0.0f;
}

void FxFairyLights::step(float dt) {
  auto v = CanvasView::acquire(canvas_, area_);
  if (!v.ok() || v.w != W_ || v.h != H_ || bulbs_.empty())
    return;

  t_ += dt;
  if (t_ >= 3600.0f)
    t_ -= 3600.0f;

  v.clear_black();

  for (auto &f : snow_) {
    f.y += f.speed * dt;
    if (f.y >= (float) H_) {
      f.y = 0.0f;
      f.x = frand(0.0f, (float) W_);
    }
    v.put_px((int) f.x, (int) f.y, pack_rgb(55, 55, 70));
  }

  // Wire
  for (int x = 0; x < W_; ++x)
    v.put_px(x, wire_y_[x], pack_rgb(35, 45, 25));

  // Bulbs: the glow runs along the string, each bulb a little later than the previous one
  for (size_t i = 0; i < bulbs_.size(); ++i) {
    const auto &b = bulbs_[i];
    const float a = 0.35f + 0.65f * (0.5f + 0.5f * std::sin(t_ * CHASE_HZ * TWO_PI + (float) i * 1.9f));
    // Soft halo around the glass
    for (int dy = -1; dy <= 5; ++dy) {
      for (int dx = -3; dx <= 3; ++dx) {
        const float dd = (float) (dx * dx) + ((float) dy - 2.5f) * ((float) dy - 2.5f);
        if (dd <= 1.2f)
          continue;
        const float k = a * 0.5f / dd;
        v.put_px_add(b.x + dx, b.y + dy, (uint8_t) (b.r * k), (uint8_t) (b.g * k), (uint8_t) (b.b * k));
      }
    }
    const auto glass = pack_rgb((uint8_t) (b.r * a), (uint8_t) (b.g * a), (uint8_t) (b.b * a));
    const auto side = pack_rgb((uint8_t) (b.r * a * 0.8f), (uint8_t) (b.g * a * 0.8f), (uint8_t) (b.b * a * 0.8f));
    v.put_px(b.x, b.y, pack_rgb(110, 110, 90));  // cap
    v.put_px(b.x, b.y + 1, glass);
    v.put_px(b.x, b.y + 2, glass);
    v.put_px(b.x, b.y + 3, glass);
    v.put_px(b.x - 1, b.y + 2, side);
    v.put_px(b.x + 1, b.y + 2, side);
  }

  v.invalidate(canvas_);
}

}  // namespace lvgl_canvas_fx
}  // namespace esphome
