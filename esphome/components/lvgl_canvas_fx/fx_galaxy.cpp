// SPDX-License-Identifier: MIT

#include "fx_galaxy.h"

namespace esphome {
namespace lvgl_canvas_fx {

using fxutil::CanvasView;
using fxutil::clampf;
using fxutil::frand;
using fxutil::irand;
using fxutil::lerpf;

static constexpr float TWO_PI = 6.2831853f;

void FxGalaxy::on_resize(const Rect &r) {
  FxBase::on_resize(r);
  W_ = r.w;
  H_ = r.h;
  if (W_ <= 0 || H_ <= 0) {
    phase_.clear();
    return;
  }
  const size_t n = (size_t) W_ * (size_t) H_;
  phase_.assign(n, 0.0f);
  disk_.assign(n, 0.0f);
  core_.assign(n, 0.0f);
  col_.assign(n * 3, 0);

  ct_ = std::cos(TILT);
  st_ = std::sin(TILT);
  // Largest disc that fits the canvas once it is squashed
  radius_ = std::min((float) W_ * 0.48f, (float) H_ * 0.5f / SQUASH);
  const float cx = (float) (W_ - 1) * 0.5f, cy = (float) (H_ - 1) * 0.5f;
  const float R = radius_;

  for (int y = 0; y < H_; ++y) {
    for (int x = 0; x < W_; ++x) {
      const size_t i = (size_t) y * W_ + x;
      const float dx = (float) x - cx, dy = (float) y - cy;
      // Screen -> galaxy plane
      const float u = dx * ct_ + dy * st_;
      const float v = (-dx * st_ + dy * ct_) / SQUASH;
      const float rr = std::sqrt(u * u + v * v);
      // Logarithmic spiral arms: brightness peaks where this phase == ARMS*rot
      phase_[i] = (float) ARMS * std::atan2(v, u) + WIND * std::log(rr * 30.0f / R + 1.0f);
      const float edge = clampf((R * 1.03f - rr) / (R * 0.13f), 0.0f, 1.0f);
      disk_[i] = std::exp(-rr / (R * 0.43f)) * edge;
      core_[i] = std::exp(-rr / (R * 0.087f));
      // Arm colour: pink near the core, violet, then blue at the rim
      const float q = clampf(rr / (R * 0.93f), 0.0f, 1.0f);
      float cr, cg, cb;
      if (q < 0.5f) {
        const float f = q * 2.0f;
        cr = lerpf(255.0f, 130.0f, f);
        cg = lerpf(110.0f, 80.0f, f);
        cb = lerpf(190.0f, 255.0f, f);
      } else {
        const float f = (q - 0.5f) * 2.0f;
        cr = lerpf(130.0f, 50.0f, f);
        cg = lerpf(80.0f, 70.0f, f);
        cb = lerpf(255.0f, 200.0f, f);
      }
      col_[i * 3] = (uint8_t) cr;
      col_[i * 3 + 1] = (uint8_t) cg;
      col_[i * 3 + 2] = (uint8_t) cb;
    }
  }

  arm_stars_.resize((size_t) std::max(12, W_ * H_ / ARM_STAR_DIV));
  for (auto &s : arm_stars_) {
    s.r = frand(R * 0.10f, R * 0.93f);
    const float arm = (float) irand(0, ARMS - 1) * TWO_PI / (float) ARMS;
    // Sit on an arm, with some scatter
    s.a = arm - (WIND / (float) ARMS) * std::log(s.r * 30.0f / R + 1.0f) + frand(-0.9f, 0.9f) / (float) ARMS;
    s.speed = frand(1.0f, 4.0f);
    s.phase = frand(0.0f, TWO_PI);
  }
  back_stars_.resize((size_t) std::max(4, W_ * H_ / BACK_STAR_DIV));
  for (auto &s : back_stars_) {
    s.x = (uint16_t) irand(0, W_ - 1);
    s.y = (uint16_t) irand(0, H_ - 1);
    s.speed = frand(0.5f, 2.5f);
    s.phase = frand(0.0f, TWO_PI);
  }
  rot_ = 0.0f;
  t_ = 0.0f;
}

void FxGalaxy::step(float dt) {
  auto v = CanvasView::acquire(canvas_, area_);
  if (!v.ok() || v.w != W_ || v.h != H_ || phase_.empty())
    return;

  rot_ += dt * TWO_PI / PERIOD_S;
  if (rot_ >= TWO_PI)
    rot_ -= TWO_PI;
  t_ += dt;
  if (t_ >= 3600.0f)
    t_ -= 3600.0f;

  // Disc with spiral arms and a bright core
  const float rot2 = (float) ARMS * rot_;
  for (int y = 0; y < H_; ++y) {
    for (int x = 0; x < W_; ++x) {
      const size_t i = (size_t) y * W_ + x;
      float a = 0.5f + 0.5f * std::cos(phase_[i] - rot2);
      a = a * a * a;
      const float ia = a * disk_[i] * 1.7f + disk_[i] * 0.12f;
      const float c = core_[i];
      const float r = ia * col_[i * 3] + c * 255.0f;
      const float g = ia * col_[i * 3 + 1] + c * 235.0f;
      const float b = ia * col_[i * 3 + 2] + c * 215.0f;
      v.put_px(x, y, fxutil::pack_rgb((uint8_t) std::min(r, 255.0f), (uint8_t) std::min(g, 255.0f),
                                      (uint8_t) std::min(b, 255.0f)));
    }
  }

  // Stars in the arms: rotate with the disc and twinkle
  const float cx = (float) (W_ - 1) * 0.5f, cy = (float) (H_ - 1) * 0.5f;
  for (const auto &s : arm_stars_) {
    const float th = s.a + rot_;
    const float u = s.r * std::cos(th);
    const float w = s.r * std::sin(th) * SQUASH;
    const int px = (int) std::lround(u * ct_ - w * st_ + cx);
    const int py = (int) std::lround(u * st_ + w * ct_ + cy);
    const float b = 60.0f + 110.0f * (0.5f + 0.5f * std::sin(t_ * s.speed + s.phase));
    v.put_px_add(px, py, (uint8_t) (b * 0.9f), (uint8_t) (b * 0.9f), (uint8_t) b);
  }

  // Background stars: fixed, twinkling
  for (const auto &s : back_stars_) {
    const uint8_t b = (uint8_t) (30.0f + 90.0f * (0.5f + 0.5f * std::sin(t_ * s.speed + s.phase)));
    v.put_px_add(s.x, s.y, b, b, b);
  }

  v.invalidate(canvas_);
}

}  // namespace lvgl_canvas_fx
}  // namespace esphome
