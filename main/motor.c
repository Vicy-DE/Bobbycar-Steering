/**
 * @file motor.c
 * @brief Four bobbycar motor instances and control logic.
 *
 * Manages four motor_t objects representing the four wheels
 * of the bobbycar (FL, FR, RL, RR).  Provides clamped
 * set/get operations and bulk torque assignment.
 */

#include "motor.h"

#include <string.h>
#include "esp_log.h"

static const char *TAG = "motor";

/** The four motor objects (FL, FR, RL, RR). */
static motor_t s_motors[MOTOR_COUNT];

/** Human-readable motor names for logging. */
static const char *s_names[MOTOR_COUNT] = {
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
    if (val > lim) {
        return lim;
    }
    if (val < -lim) {
        return -lim;
    }
    return val;
}

/* --------------------------------------------------------- */
/*  Public API                                               */
/* --------------------------------------------------------- */

/* See motor.h for doc comments. */
void motor_init(void)
{
    memset(s_motors, 0, sizeof(s_motors));
    for (int i = 0; i < MOTOR_COUNT; i++) {
        s_motors[i].enabled   = false;
        s_motors[i].connected = false;
    }
    ESP_LOGI(TAG, "Motor init: %d motors (FL FR RL RR)",
             MOTOR_COUNT);
}

motor_t *motor_get(int idx)
{
    if (idx < 0 || idx >= MOTOR_COUNT) {
        return NULL;
    }
    return &s_motors[idx];
}

void motor_set_torque(int idx, int torque)
{
    if (idx < 0 || idx >= MOTOR_COUNT) {
        return;
    }
    s_motors[idx].torque = clamp_sym(torque, THROTTLE_MAX);
}

int motor_get_torque(int idx)
{
    if (idx < 0 || idx >= MOTOR_COUNT) {
        return 0;
    }
    return s_motors[idx].torque;
}

void motor_set_enabled(int idx, bool en)
{
    if (idx < 0 || idx >= MOTOR_COUNT) {
        return;
    }
    s_motors[idx].enabled = en;
    ESP_LOGI(TAG, "Motor %s %s",
             s_names[idx], en ? "enabled" : "disabled");
}

bool motor_get_enabled(int idx)
{
    if (idx < 0 || idx >= MOTOR_COUNT) {
        return false;
    }
    return s_motors[idx].enabled;
}

void motor_set_all_torque(const int torque[MOTOR_COUNT])
{
    for (int i = 0; i < MOTOR_COUNT; i++) {
        s_motors[i].torque = clamp_sym(
            torque[i], THROTTLE_MAX);
    }
}

int motor_get_throttle(void)
{
    int sum = 0;
    for (int i = 0; i < MOTOR_COUNT; i++) {
        sum += s_motors[i].torque;
    }
    return sum / MOTOR_COUNT;
}

void motor_set_throttle(int throttle)
{
    int clamped = clamp_sym(throttle, THROTTLE_MAX);
    for (int i = 0; i < MOTOR_COUNT; i++) {
        s_motors[i].torque = clamped;
    }
}
