// SPDX-License-Identifier: MIT

#include "fx_voxel_landscape.h"

namespace esphome {
namespace lvgl_canvas_fx {

using fxutil::CanvasView;
using fxutil::frand;
using fxutil::pack_rgb;

void FxVoxelLandscape::build_map_() {
  // Value noise, four octaves, wrapping seamlessly at the map edge
  std::vector<float> acc((size_t) N * N, 0.0f);
  static const int FREQ[4] = {8, 16, 32, 64};
  static const float AMP[4] = {1.0f, 0.45f, 0.2f, 0.08f};
  for (int o = 0; o < 4; ++o) {
    const int f = FREQ[o];
    std::vector<float> g((size_t) f * f);
    for (auto &v : g)
      v = frand(0.0f, 1.0f);
    for (int y = 0; y < N; ++y) {
      const float gy = (float) y * f / N;
      const int iy = (int) gy;
      float fy = gy - iy;
      fy = fy * fy * (3.0f - 2.0f * fy);
      for (int x = 0; x < N; ++x) {
        const float gx = (float) x * f / N;
        const int ix = (int) gx;
        float fx = gx - ix;
        fx = fx * fx * (3.0f - 2.0f * fx);
        const float a0 = g[(iy % f) * f + ix % f], a1 = g[(iy % f) * f + (ix + 1) % f];
        const float b0 = g[((iy + 1) % f) * f + ix % f], b1 = g[((iy + 1) % f) * f + (ix + 1) % f];
        acc[(size_t) y * N + x] += AMP[o] * ((a0 * (1 - fx) + a1 * fx) * (1 - fy) + (b0 * (1 - fx) + b1 * fx) * fy);
      }
    }
  }
  float mn = 1e9f, mx = -1e9f;
  for (float v : acc) {
    mn = std::min(mn, v);
    mx = std::max(mx, v);
  }
  map_.resize((size_t) N * N);
  for (size_t i = 0; i < acc.size(); ++i) {
    const float n = (acc[i] - mn) / std::max(1e-6f, mx - mn);
    map_[i] = (uint8_t) (255.0f * std::pow(n, 1.6f));  // flatter valleys, sharper peaks
  }
}

void FxVoxelLandscape::on_resize(const Rect &r) {
  FxBase::on_resize(r);
  W_ = r.w;
  H_ = r.h;
  if (W_ <= 0 || H_ <= 0)
    return;
  if (map_.empty())
    build_map_();
  ybuf_.assign((size_t) W_, 0);
  cam_h_ = std::max(h_at_(cam_x_, cam_y_), WATER) * HEIGHT + CLEARANCE;
}

static void terrain_color(float h, float shade, uint8_t &r, uint8_t &g, uint8_t &b) {
  float cr, cg, cb;
  if (h < 0.18f) {
    cr = 20; cg = 60; cb = 140; shade = 1.0f;  // water
  } else if (h < 0.21f) {
    cr = 170; cg = 160; cb = 100;  // beach
  } else if (h < 0.45f) {
    cr = 40; cg = 110.0f + 70.0f * (h - 0.21f) / 0.24f; cb = 40;  // meadow
  } else if (h < 0.66f) {
    cr = 105; cg = 90; cb = 75;  // rock
  } else {
    cr = 230; cg = 230; cb = 240;  // snow
  }
  r = (uint8_t) std::min(255.0f, cr * shade);
  g = (uint8_t) std::min(255.0f, cg * shade);
  b = (uint8_t) std::min(255.0f, cb * shade);
}

void FxVoxelLandscape::step(float dt) {
  auto v = CanvasView::acquire(canvas_, area_);
  if (!v.ok() || v.w != W_ || v.h != H_ || map_.empty())
    return;

  t_ += dt;
  if (t_ >= 3600.0f)
    t_ -= 3600.0f;

  // Camera: flies forward, slowly swings left and right
  const float ang = 0.6f + 0.35f * std::sin(t_ * 0.11f) + 0.15f * std::sin(t_ * 0.3f);
  const float sa = std::sin(ang), ca = std::cos(ang);
  cam_x_ += -(sa + ca) * 0.7071f * SPEED * dt;
  cam_y_ += -(ca - sa) * 0.7071f * SPEED * dt;
  if (cam_x_ < 0) cam_x_ += N;
  if (cam_y_ < 0) cam_y_ += N;
  if (cam_x_ >= N) cam_x_ -= N;
  if (cam_y_ >= N) cam_y_ -= N;
  // Fly over what lies ahead, smoothly
  float ground = WATER;
  for (int k = 0; k <= 12; k += 4)
    ground = std::max(ground, h_at_(cam_x_ - (sa + ca) * 0.7071f * k, cam_y_ - (ca - sa) * 0.7071f * k));
  const float target = ground * HEIGHT + CLEARANCE;
  cam_h_ += (target - cam_h_) * std::min(1.0f, dt * 1.5f);

  const int horizon = (int) ((float) H_ * HORIZON);

  // Dusk sky and a low sun
  for (int y = 0; y < H_; ++y) {
    const float u = (float) y / (float) H_;
    const auto c = pack_rgb((uint8_t) (40 + 150 * u), (uint8_t) (20 + 60 * u), (uint8_t) (80 - 20 * u));
    for (int x = 0; x < W_; ++x)
      v.put_px(x, y, c);
  }
  const int sx = W_ * 3 / 4 - 2, sy = horizon + 1;
  for (int y = sy - 4; y <= sy; ++y)
    for (int x = sx - 4; x <= sx + 4; ++x)
      if ((x - sx) * (x - sx) + (y - sy) * (y - sy) <= 12)
        v.put_px(x, y, pack_rgb(255, 200, 90));

  // Voxel space: march from near to far, draw each column's visible spans
  std::fill(ybuf_.begin(), ybuf_.end(), (uint8_t) H_);
  float z = 1.0f, dz = 0.3f;
  while (z < FAR) {
    float plx = (-ca - sa) * z + cam_x_, ply = (sa - ca) * z + cam_y_;
    const float prx = (ca - sa) * z + cam_x_, pry = (-sa - ca) * z + cam_y_;
    const float dx = (prx - plx) / (float) W_, dy = (pry - ply) / (float) W_;
    float fog = z / FAR;
    fog = std::min(1.0f, std::pow(fog, 1.3f));
    for (int i = 0; i < W_; ++i) {
      const float hh = h_at_(plx, ply);
      const float hv = std::max(hh, WATER);
      int top = (int) ((cam_h_ - hv * HEIGHT) / z * FOV_SCALE + (float) horizon);
      if (top < 0)
        top = 0;
      if (top < ybuf_[i]) {
        // light from the left: brighter where the ground rises towards +x
        const float hl = h_at_(plx - 1.0f, ply);
        const float shade = std::max(0.55f, std::min(1.25f, 1.0f + (hh - hl) * 12.0f));
        uint8_t r, g, b;
        terrain_color(hh, shade, r, g, b);
        r = (uint8_t) (r * (1 - fog) + 150 * fog);
        g = (uint8_t) (g * (1 - fog) + 70 * fog);
        b = (uint8_t) (b * (1 - fog) + 100 * fog);
        const auto c = pack_rgb(r, g, b);
        for (int y = top; y < ybuf_[i]; ++y)
          v.put_px(i, y, c);
        ybuf_[i] = (uint8_t) top;
      }
      plx += dx;
      ply += dy;
    }
    z += dz;
    dz += 0.03f;
  }

  v.invalidate(canvas_);
}

}  // namespace lvgl_canvas_fx
}  // namespace esphome
