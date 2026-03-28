/**
 * @file steering_algo.h
 * @brief Ackermann steering algorithm for four-wheel torque.
 *
 * Computes individual wheel torques based on a steering angle
 * using the Ackermann geometry model from the bobbycar project.
 * Dimensions are in centimetres, angles in radians.
 */

#pragma once

/* Bobbycar chassis geometry (cm) */
#define L_WHEELBASE         35.0f
#define L_WIDTH             30.0f
#define L_STEERING_WIDTH    20.0f
#define L_STEERING_TO_WHEEL  5.0f

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert radians to degrees.
 *
 * @param[in] rad  Angle in radians.
 * @return Angle in degrees.
 */
float rad2deg(float rad);

/**
 * @brief Convert degrees to radians.
 *
 * @param[in] deg  Angle in degrees.
 * @return Angle in radians.
 */
float deg2rad(float deg);

/**
 * @brief Compute per-wheel torque from Ackermann geometry.
 *
 * Output order: torque[0]=FL, torque[1]=FR,
 *               torque[2]=RL, torque[3]=RR.
 *
 * @param[in]  throttle         Base throttle (-1000..+1000).
 * @param[in]  alpha_steer      Steering angle (radians).
 * @param[in]  torque_regulated  PID correction applied to
 *                               front axle differential.
 * @param[out] torque           Array of 4 output torques.
 */
void calc_torque_per_wheel(int throttle,
                           float alpha_steer,
                           int torque_regulated,
                           int *torque);

#ifdef __cplusplus
}
#endif
