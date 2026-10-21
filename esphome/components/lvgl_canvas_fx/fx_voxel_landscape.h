// SPDX-License-Identifier: MIT
//
// Endless flight over a generated island landscape (lakes, beaches, meadows,
// rock and snowy peaks) at dusk, rendered with the classic "voxel space"
// column renderer: side lighting on the slopes and haze in the distance.

#pragma once
#include "fx_base.h"
#include "fx_util.h"
#include <vector>

namespace esphome {
namespace lvgl_canvas_fx {

class FxVoxelLandscape : public FxBase {
 public:
  void on_resize(const Rect &r) override;
  void step(float dt) override;

 private:
  // ---- tuning ----
  static constexpr int MAP_BITS = 7;          // height map is 2^MAP_BITS squared (wraps around)
  static constexpr float SPEED = 14.0f;       // flight speed, map cells per second
  static constexpr float HEIGHT = 40.0f;      // vertical scale of the terrain
  static constexpr float CLEARANCE = 12.0f;   // how high the camera flies above the ground
  static constexpr float FOV_SCALE = 20.0f;   // projection scale (bigger = steeper)
  static constexpr float FAR = 110.0f;        // view distance
  static constexpr float WATER = 0.18f;       // sea level (0..1)
  static constexpr float HORIZON = 0.34f;     // horizon row, fraction of the height

  static constexpr int N = 1 << MAP_BITS;
  static constexpr int MASK = N - 1;

  int W_{0}, H_{0};
  float t_{0};
  float cam_x_{60}, cam_y_{40}, cam_h_{0};
  std::vector<uint8_t> map_;     // terrain height 0..255
  std::vector<uint8_t> ybuf_;    // per column: topmost row drawn so far

  float h_at_(float x, float y) const { return map_[((int) y & MASK) * N + ((int) x & MASK)] / 255.0f; }
  void build_map_();
};

}  // namespace lvgl_canvas_fx
}  // namespace esphome
