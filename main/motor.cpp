/**
 * @file motor.cpp
 * @brief Four bobbycar motor instances (C++ implementation).
 *
 * Implements the Motor and MotorController classes plus the
 * C-linkage wrapper functions so that plain-C translation
 * units can call the motor API.
 */

#include "motor.h"

#include <cstring>
#include "esp_log.h"

static const char *TAG = "motor";

/** Human-readable motor names for logging. */
static const char *k_names[MOTOR_COUNT] = {
    "FL", "FR", "RL", "RR"
};

/* --------------------------------------------------------- */
/*  Helpers                                                  */
/* --------------------------------------------------------- */

/**
 * @brief Clamp an integer to a symmetric range.
 *
 * @param[in] val  Input value.
 * @param[in] lim  Positive limit (symmetric: -lim..+lim).
 * @return Clamped value.
 */
static int clamp_sym(int val, int lim)
{
    if (val > lim)  return lim;
    if (val < -lim) return -lim;
    return val;
}

/* --------------------------------------------------------- */
/*  Motor class                                              */
/* --------------------------------------------------------- */

void Motor::set_torque(int t)
{
    torque_ = clamp_sym(t, THROTTLE_MAX);
}

void Motor::set_enabled(bool en)
{
    enabled_ = en;
}

/* --------------------------------------------------------- */
/*  MotorController class                                    */
/* --------------------------------------------------------- */

MotorController &MotorController::instance()
{
    static MotorController s_inst;
    return s_inst;
}

void MotorController::init()
{
    for (int i = 0; i < MOTOR_COUNT; i++) {
        motors_[i] = Motor();
    }
    ESP_LOGI(TAG, "Motor init: %d motors"
             " (FL FR RL RR)", MOTOR_COUNT);
}

Motor &MotorController::get(int idx)
{
    return motors_[idx];
}

const Motor &MotorController::get(int idx) const
{
    return motors_[idx];
}

void MotorController::set_all_torque(
    const int t[MOTOR_COUNT])
{
    for (int i = 0; i < MOTOR_COUNT; i++) {
        motors_[i].set_torque(t[i]);
    }
}

int MotorController::avg_throttle() const
{
    int sum = 0;
    for (int i = 0; i < MOTOR_COUNT; i++) {
        sum += motors_[i].torque();
    }
    return sum / MOTOR_COUNT;
}

void MotorController::set_throttle(int t)
{
    int clamped = clamp_sym(t, THROTTLE_MAX);
    for (int i = 0; i < MOTOR_COUNT; i++) {
        motors_[i].set_torque(clamped);
    }
}

const char *MotorController::name(int idx)
{
    if (idx < 0 || idx >= MOTOR_COUNT) return "??";
    return k_names[idx];
}

/* --------------------------------------------------------- */
/*  C-linkage wrappers                                       */
/* --------------------------------------------------------- */

extern "C" {

void motor_init(void)
{
    MotorController::instance().init();
}

int motor_get_torque(int idx)
{
    if (idx < 0 || idx >= MOTOR_COUNT) return 0;
    return MotorController::instance().get(idx).torque();
}

void motor_set_torque(int idx, int torque)
{
    if (idx < 0 || idx >= MOTOR_COUNT) return;
    MotorController::instance().get(idx).set_torque(torque);
}

bool motor_get_enabled(int idx)
{
    if (idx < 0 || idx >= MOTOR_COUNT) return false;
    return MotorController::instance().get(idx).enabled();
}

void motor_set_enabled(int idx, bool en)
{
    if (idx < 0 || idx >= MOTOR_COUNT) return;
    auto &m = MotorController::instance().get(idx);
    m.set_enabled(en);
    ESP_LOGI(TAG, "Motor %s %s",
             MotorController::name(idx),
             en ? "enabled" : "disabled");
}

void motor_set_all_torque(const int t[MOTOR_COUNT])
{
    MotorController::instance().set_all_torque(t);
}

int motor_get_throttle(void)
{
    return MotorController::instance().avg_throttle();
}

void motor_set_throttle(int throttle)
{
    MotorController::instance().set_throttle(throttle);
}

bool motor_get_state(int idx, motor_state_t *out)
{
    if (idx < 0 || idx >= MOTOR_COUNT || !out) {
        return false;
    }
    const Motor &m =
        MotorController::instance().get(idx);
    out->torque      = m.torque();
    out->speed_meas  = m.speed();
    out->bat_voltage = m.bat_voltage();
    out->board_temp  = m.board_temp();
    out->enabled     = m.enabled();
    out->connected   = m.connected();
    return true;
}

} /* extern "C" */
