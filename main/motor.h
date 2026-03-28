/**
 * @file motor.h
 * @brief Bobbycar motor objects — four wheel drive.
 *
 * Defines the motor data structure and API for four independent
 * wheel motors (front-left, front-right, rear-left, rear-right)
 * with bobbycar-specific parameters.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Motor indices */
#define MOTOR_FL    0   /**< Front-left wheel.  */
#define MOTOR_FR    1   /**< Front-right wheel. */
#define MOTOR_RL    2   /**< Rear-left wheel.   */
#define MOTOR_RR    3   /**< Rear-right wheel.  */
#define MOTOR_COUNT 4   /**< Total motor count. */

/* Torque limits (unitless, hoverboard protocol scale) */
#define THROTTLE_MAX         1000
#define THROTTLE_REVERSE_MAX (THROTTLE_MAX * 3 / 10)

/* Hoverboard serial protocol constants */
#define HOVER_SERIAL_BAUD    57600
#define HOVER_START_FRAME    0x7A7A
#define HOVER_SEND_INTERVAL  20   /**< ms between commands. */

/**
 * @brief One motor / wheel state.
 */
typedef struct {
    int      torque;       /**< Commanded: -1000..+1000. */
    int      speed_meas;   /**< Measured speed (ticks).  */
    int16_t  bat_voltage;  /**< Battery (0.01 V units).  */
    int16_t  board_temp;   /**< Board temp (0.1 C).      */
    bool     enabled;      /**< Motor enable flag.        */
    bool     connected;    /**< Board comm active flag.   */
} motor_t;

/**
 * @brief Initialise all four motor objects to defaults.
 *
 * @sideeffects Zeros static motor array.
 */
void motor_init(void);

/**
 * @brief Get pointer to a motor object.
 *
 * @param[in] idx  Motor index (MOTOR_FL .. MOTOR_RR).
 * @return Pointer to the motor struct, NULL on bad index.
 */
motor_t *motor_get(int idx);

/**
 * @brief Set torque for one motor (clamped).
 *
 * @param[in] idx     Motor index.
 * @param[in] torque  Value in -THROTTLE_MAX..+THROTTLE_MAX.
 *
 * @sideeffects Modifies static motor torque value.
 */
void motor_set_torque(int idx, int torque);

/**
 * @brief Get current torque for one motor.
 *
 * @param[in] idx  Motor index.
 * @return Torque value, 0 on bad index.
 */
int motor_get_torque(int idx);

/**
 * @brief Set enable flag for one motor.
 *
 * @param[in] idx  Motor index.
 * @param[in] en   true to enable, false to disable.
 *
 * @sideeffects Modifies static motor struct.
 */
void motor_set_enabled(int idx, bool en);

/**
 * @brief Check if a motor is enabled.
 *
 * @param[in] idx  Motor index.
 * @return true if enabled, false otherwise.
 */
bool motor_get_enabled(int idx);

/**
 * @brief Set torque for all four motors at once.
 *
 * @param[in] torque  Array of MOTOR_COUNT values.
 *
 * @sideeffects Modifies all static motor torques.
 */
void motor_set_all_torque(const int torque[MOTOR_COUNT]);

/**
 * @brief Get combined throttle value.
 *
 * @return Average of all four motor torques.
 */
int motor_get_throttle(void);

/**
 * @brief Set throttle for all motors uniformly.
 *
 * @param[in] throttle  Value in -THROTTLE_MAX..+THROTTLE_MAX.
 *
 * @sideeffects Modifies all motor torques.
 */
void motor_set_throttle(int throttle);
