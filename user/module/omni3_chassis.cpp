#include "omni3_chassis.hpp"

#include "FreeRTOS.h" // IWYU pragma: keep (configASSERT)


// 运动学系数：θ = 30°/150°/270°，实测正转方向 d_i = (-sinθ_i, +cosθ_i)
// （方位角见 Docs/omni3/omni3_layout.py；2026-10-29 上板实测后整体取反过一次）
//   M1: d = (-0.500, +0.866)   M2: d = (-0.500, -0.866)   M3: d = (+1.000, 0.000)
constexpr float WHEEL_A[3] = {-0.5f, -0.5f, 1.0f};            // v_x 系数
constexpr float WHEEL_B[3] = {0.8660254f, -0.8660254f, 0.0f}; // v_y 系数


// ---------------- 公共接口 ----------------

/**
 * @brief 校验配置：三个电机必须绑上、R 必须实测过
 */
void Omni3Chassis::init()
{
  configASSERT(_config.motor[0] != nullptr); // M1 右前没绑：解出来的速度无处下发
  configASSERT(_config.motor[1] != nullptr); // M2 左前没绑
  configASSERT(_config.motor[2] != nullptr); // M3 正后没绑

  // R = 0 时自转项恒为 0，上电后 ω 给了也不转，属于必填参数
  configASSERT(_config.wheel_radius_cm > 0.0f);

  stop();
}

/** @brief 底盘速度 → 三轮目标速度 */
void Omni3Chassis::drive(float vx_cm_s, float vy_cm_s, float wz_rad_s)
{
  // 先限自转：三轮共用的 -R·ω 一旦过大，会把三轮同时顶到限幅，平移就没有余量了
  if (wz_rad_s > _config.w_max_rad_s)
    wz_rad_s = _config.w_max_rad_s;
  else if (wz_rad_s < -_config.w_max_rad_s)
    wz_rad_s = -_config.w_max_rad_s;

  const float rot = _config.wheel_radius_cm * wz_rad_s; // 三轮相同，见文件头说明

  for (uint8_t i = 0; i < 3U; ++i)
  {
    float v = WHEEL_A[i] * vx_cm_s + WHEEL_B[i] * vy_cm_s + rot;

    // 平移与自转叠加后可能超出单轮可用速度（如斜向平移 + 自转），按等比例优先保方向
    if (v > _config.v_max_cm_s)
      v = _config.v_max_cm_s;
    else if (v < -_config.v_max_cm_s)
      v = -_config.v_max_cm_s;

    _target[i] = v;
    _config.motor[i]->set_speed(v);
  }
}

/**
 * @brief 三个轮子真正停机
 *
 * @note 不能写成 drive(0,0,0)：那只是把目标设为 0，DeviceMotor 仍处于 SPEED 模式，
 *       速度环会跟着编码器的 ±1 抖动继续输出微小占空比，H 桥以 1 kHz 响个不停。
 *       下面调的是 DeviceMotor::stop()：输出归 0、模式回 STOP、积分清零。
 */
void Omni3Chassis::stop()
{
  for (uint8_t i = 0; i < 3U; ++i)
  {
    _target[i] = 0.0f;
    _config.motor[i]->stop();
  }
}

// ----------------
