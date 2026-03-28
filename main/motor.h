/**
 * @file motor.h
 * @brief Bobbycar motor objects — four wheel drive.
 *
 * Defines a Motor C++ class for individual wheel motors
 * and a MotorController that manages all four wheels
 * (FL, FR, RL, RR).  Provides a C-linkage API for use
 * from plain C translation units.
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

#ifdef __cplusplus

/**
 * @brief One motor / wheel object.
 *
 * Wraps per-wheel state and provides clamped getters and
 * setters.  Construct via MotorController.
 */
class Motor {
public:
    Motor() = default;

    /** @brief Get commanded torque. */
    int  torque()     const { return torque_; }
    /** @brief Get measured speed. */
    int  speed()      const { return speed_meas_; }
    /** @brief Get battery voltage (0.01 V). */
    int16_t bat_voltage() const { return bat_v_; }
    /** @brief Get board temperature (0.1 C). */
    int16_t board_temp()  const { return board_t_; }
    /** @brief Check if motor is enabled. */
    bool enabled()    const { return enabled_; }
    /** @brief Check if board comm is active. */
    bool connected()  const { return connected_; }

    /**
     * @brief Set torque (clamped to ±THROTTLE_MAX).
     * @param[in] t  Desired torque.
     * @sideeffects Modifies internal torque value.
     */
    void set_torque(int t);

    /**
     * @brief Set measured speed.
     * @param[in] s  Measured speed ticks.
     * @sideeffects Modifies internal speed value.
     */
    void set_speed(int s) { speed_meas_ = s; }

    /**
     * @brief Set enable flag.
     * @param[in] en  true to enable.
     * @sideeffects Modifies enable flag, logs.
     */
    void set_enabled(bool en);

    /**
     * @brief Set connected flag.
     * @param[in] c  true if board comm active.
     * @sideeffects Modifies connected flag.
     */
    void set_connected(bool c) { connected_ = c; }

private:
    int      torque_     = 0;
    int      speed_meas_ = 0;
    int16_t  bat_v_      = 0;
    int16_t  board_t_    = 0;
    bool     enabled_    = false;
    bool     connected_  = false;
};

/**
 * @brief Manager for the four bobbycar wheel motors.
 */
class MotorController {
public:
    /** @brief Get singleton instance. */
    static MotorController &instance();

    /**
     * @brief Initialise all motors to defaults.
     * @sideeffects Resets motor objects, logs.
     */
    void init();

    /**
     * @brief Get motor by index.
     * @param[in] idx  0..MOTOR_COUNT-1.
     * @return Reference to the Motor object.
     */
    Motor       &get(int idx);
    const Motor &get(int idx) const;

    /**
     * @brief Set torque on all four motors.
     * @param[in] t  Array of MOTOR_COUNT values.
     * @sideeffects Modifies all motor torques.
     */
    void set_all_torque(const int t[MOTOR_COUNT]);

    /**
     * @brief Average torque across all motors.
     * @return Average throttle value.
     */
    int avg_throttle() const;

    /**
     * @brief Set uniform throttle on all motors.
     * @param[in] t  Throttle value.
     * @sideeffects Modifies all motor torques.
     */
    void set_throttle(int t);

    /** @brief Human-readable name for motor index. */
    static const char *name(int idx);

private:
    MotorController() = default;
    Motor motors_[MOTOR_COUNT];
};

#endif /* __cplusplus */

/* --------------------------------------------------------- */
/*  C-linkage API (callable from .c files)                   */
/* --------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

void    motor_init(void);
int     motor_get_torque(int idx);
void    motor_set_torque(int idx, int torque);
bool    motor_get_enabled(int idx);
void    motor_set_enabled(int idx, bool en);
void    motor_set_all_torque(const int t[MOTOR_COUNT]);
int     motor_get_throttle(void);
void    motor_set_throttle(int throttle);

/**
 * @brief Motor state snapshot for C callers.
 */
typedef struct {
    int      torque;
    int      speed_meas;
    int16_t  bat_voltage;
    int16_t  board_temp;
    bool     enabled;
    bool     connected;
} motor_state_t;

/**
 * @brief Get a snapshot of one motor's state.
 * @param[in]  idx  Motor index.
 * @param[out] out  Filled on success.
 * @return true if idx valid.
 */
bool motor_get_state(int idx, motor_state_t *out);

#ifdef __cplusplus
}
#endif
