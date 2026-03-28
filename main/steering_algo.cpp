/**
 * @file steering_algo.cpp
 * @brief Ackermann steering torque distribution algorithm.
 *
 * Ported from the bobbycar-project math_functions.c.
 * Computes per-wheel torque using Ackermann geometry so that
 * the inner wheels turn slower than the outer wheels during
 * a turn.  Compiled as C++ but exposes extern "C" API.
 */

#include "steering_algo.h"
#include "motor.h"

#include <cmath>

/* --------------------------------------------------------- */
/*  Helpers                                                  */
/* --------------------------------------------------------- */

/**
 * @brief Return sign of a float as int.
 *
 * @param[in] x  Input value.
 * @return 1, -1, or 0.
 */
static int signf(float x)
{
    if (x > 0.0f) return 1;
    if (x < 0.0f) return -1;
    return 0;
}

/**
 * @brief Square a float.
 *
 * @param[in] x  Input value.
 * @return x * x.
 */
static float sq(float x)
{
    return x * x;
}

/**
 * @brief Clamp torque to motor limits.
 *
 * @param[in] val  Input torque.
 * @return Clamped value.
 */
static int clamp_torque(int val)
{
    if (val > THROTTLE_MAX)  return THROTTLE_MAX;
    if (val < -THROTTLE_MAX) return -THROTTLE_MAX;
    return val;
}

/* --------------------------------------------------------- */
/*  Public API (extern "C")                                  */
/* --------------------------------------------------------- */

extern "C" {

float rad2deg(float rad)
{
    return rad * (180.0f / static_cast<float>(M_PI));
}

float deg2rad(float deg)
{
    return deg * (static_cast<float>(M_PI) / 180.0f);
}

void calc_torque_per_wheel(int throttle,
                           float alpha_steer,
                           int torque_regulated,
                           int *torque)
{
    if (alpha_steer == 0.0f) {
        torque[0] = throttle;
        torque[1] = throttle;
        torque[2] = throttle;
        torque[3] = throttle;
    } else {
        float V[4];
        float steer_abs = std::fabsf(alpha_steer);
        int   steer_sgn = signf(alpha_steer);
        float v_bw = L_WHEELBASE
            / std::tanf(steer_abs);

        /* Rear axle — simple width ratio. */
        V[2] = (v_bw + L_WIDTH / 2.0f
                * steer_sgn) / v_bw;
        V[3] = (v_bw - L_WIDTH / 2.0f
                * steer_sgn) / v_bw;

        /* Front axle — Ackermann geometry. */
        float inner_fw = v_bw
            + (L_STEERING_WIDTH / 2.0f) * steer_sgn;
        float outer_fw = v_bw
            - (L_STEERING_WIDTH / 2.0f) * steer_sgn;
        V[0] = (std::sqrtf(sq(inner_fw)
            + sq(L_WHEELBASE))
            + L_STEERING_TO_WHEEL) / v_bw;
        V[1] = (std::sqrtf(sq(outer_fw)
            + sq(L_WHEELBASE))
            + L_STEERING_TO_WHEEL) / v_bw;

        /* Normalize so total torque is preserved. */
        float corr = 4.0f
            / (V[0] + V[1] + V[2] + V[3]);
        for (int i = 0; i < 4; i++) {
            torque[i] = static_cast<int>(
                std::roundf(
                    static_cast<float>(throttle)
                    * V[i] * corr));
        }
    }

    /* Apply PID correction to front differential. */
    torque[0] += torque_regulated;
    torque[1] -= torque_regulated;

    /* Clamp all outputs. */
    for (int i = 0; i < 4; i++) {
        torque[i] = clamp_torque(torque[i]);
    }
}

} /* extern "C" */
