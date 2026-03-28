/**
 * @file console_cmds.c
 * @brief Motor, steering, and system console commands.
 *
 * Implements the bobbycar-style get/set commands for motor
 * torque, steering angle, PID gains, input source, driving
 * mode, and system utilities (echo, ver, reset, resetbt).
 */

#include "console_cmds.h"
#include "console.h"
#include "motor.h"
#include "steering_algo.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_system.h"

/* -------------------------------------------------------- */
/*  Driving state                                           */
/* -------------------------------------------------------- */

static float   s_des_steering = 0.0f;
static float   s_cur_steering = 0.0f;
static int     s_throttle     = 0;
static int     s_input_src    = 0;
static int     s_mode         = 0;

/* PID gains */
static float s_pid_kp = 1.0f;
static float s_pid_ki = 0.0f;
static float s_pid_kd = 0.0f;
static float s_pid_out = 0.0f;

static const char *s_inputs[] = {
    "ADC", "CONSOLE", "GAMEPAD", "RC"
};
#define INPUT_COUNT 4

/* -------------------------------------------------------- */
/*  Helpers                                                 */
/* -------------------------------------------------------- */

/**
 * @brief Parse argv[1] as int.
 *
 * @param[in]  argc  Argument count.
 * @param[in]  argv  Argument vector.
 * @param[out] out   Parsed value.
 * @return true on success.
 */
static bool parse_int(int argc, char *argv[], int *out)
{
    if (argc < 2) {
        return false;
    }
    char *end = NULL;
    long val = strtol(argv[1], &end, 10);
    if (end == argv[1]) {
        return false;
    }
    *out = (int)val;
    return true;
}

/**
 * @brief Parse argv[1] as float.
 *
 * @param[in]  argc  Argument count.
 * @param[in]  argv  Argument vector.
 * @param[out] out   Parsed value.
 * @return true on success.
 */
static bool parse_float(int argc, char *argv[], float *out)
{
    if (argc < 2) {
        return false;
    }
    char *end = NULL;
    float val = strtof(argv[1], &end);
    if (end == argv[1]) {
        return false;
    }
    *out = val;
    return true;
}

/* -------------------------------------------------------- */
/*  Steering commands                                       */
/* -------------------------------------------------------- */

/**
 * @brief Set desired steering angle (degrees).
 *
 * @sideeffects Modifies s_des_steering, re-computes torques.
 */
static void cmd_sets(int argc, char *argv[])
{
    float tmp;
    if (!parse_float(argc, argv, &tmp)) {
        printf("Usage: sets <angle_deg>\r\n");
        return;
    }
    if (fabsf(tmp) > 45.0f) {
        printf("Error: angle out of range\r\n");
        return;
    }
    s_des_steering = deg2rad(tmp);
    int torques[MOTOR_COUNT];
    calc_torque_per_wheel(s_throttle,
        s_des_steering, (int)s_pid_out, torques);
    motor_set_all_torque(torques);
    printf("setS:%.4f\r\n", s_des_steering);
}

/**
 * @brief Get current steering angle (degrees).
 *
 * @sideeffects Writes to stdout.
 */
static void cmd_gets(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("S:%.4f\r\n", rad2deg(s_cur_steering));
}

/**
 * @brief Get desired steering angle (degrees).
 *
 * @sideeffects Writes to stdout.
 */
static void cmd_getds(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("dS:%.4f\r\n", rad2deg(s_des_steering));
}

/* -------------------------------------------------------- */
/*  Throttle commands                                       */
/* -------------------------------------------------------- */

/**
 * @brief Set throttle and re-compute wheel torques.
 *
 * @sideeffects Modifies motor torques.
 */
static void cmd_sett(int argc, char *argv[])
{
    int tmp;
    if (!parse_int(argc, argv, &tmp)) {
        printf("Usage: sett <throttle>\r\n");
        return;
    }
    s_throttle = tmp;
    int torques[MOTOR_COUNT];
    calc_torque_per_wheel(s_throttle,
        s_des_steering, (int)s_pid_out, torques);
    motor_set_all_torque(torques);
    printf("setT:%d\r\n", s_throttle);
}

/**
 * @brief Get current throttle.
 *
 * @sideeffects Writes to stdout.
 */
static void cmd_gett(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("T:%d\r\n", motor_get_throttle());
}

/* -------------------------------------------------------- */
/*  Motor status                                            */
/* -------------------------------------------------------- */

/**
 * @brief Print all motor torques.
 *
 * @sideeffects Writes to stdout.
 */
static void cmd_getb(int argc, char *argv[])
{
    (void)argc; (void)argv;
    const char *names[] = {"FL","FR","RL","RR"};
    for (int i = 0; i < MOTOR_COUNT; i++) {
        motor_t *m = motor_get(i);
        if (!m) {
            continue;
        }
        printf("%s: T=%d spd=%d en=%d\r\n",
               names[i], m->torque,
               m->speed_meas, m->enabled);
    }
}

/* -------------------------------------------------------- */
/*  PID commands                                            */
/* -------------------------------------------------------- */

/** @brief Set PID Kp. @sideeffects Modifies s_pid_kp. */
static void cmd_setkp(int argc, char *argv[])
{
    float tmp;
    if (!parse_float(argc, argv, &tmp)) {
        printf("Usage: setkp <value>\r\n");
        return;
    }
    s_pid_kp = tmp;
    printf("setKp:%.4f\r\n", s_pid_kp);
}
/** @brief Set PID Ki. @sideeffects Modifies s_pid_ki. */
static void cmd_setki(int argc, char *argv[])
{
    float tmp;
    if (!parse_float(argc, argv, &tmp)) {
        printf("Usage: setki <value>\r\n");
        return;
    }
    s_pid_ki = tmp;
    printf("setKi:%.4f\r\n", s_pid_ki);
}
/** @brief Set PID Kd. @sideeffects Modifies s_pid_kd. */
static void cmd_setkd(int argc, char *argv[])
{
    float tmp;
    if (!parse_float(argc, argv, &tmp)) {
        printf("Usage: setkd <value>\r\n");
        return;
    }
    s_pid_kd = tmp;
    printf("setKd:%.4f\r\n", s_pid_kd);
}
/** @brief Get Kp. @sideeffects Writes to stdout. */
static void cmd_getkp(int argc, char *argv[])
{ (void)argc; (void)argv; printf("Kp:%.4f\r\n", s_pid_kp); }
/** @brief Get Ki. @sideeffects Writes to stdout. */
static void cmd_getki(int argc, char *argv[])
{ (void)argc; (void)argv; printf("Ki:%.4f\r\n", s_pid_ki); }
/** @brief Get Kd. @sideeffects Writes to stdout. */
static void cmd_getkd(int argc, char *argv[])
{ (void)argc; (void)argv; printf("Kd:%.4f\r\n", s_pid_kd); }
/** @brief Get PID output. @sideeffects Writes to stdout. */
static void cmd_getpo(int argc, char *argv[])
{ (void)argc; (void)argv; printf("PID_Out:%.4f\r\n", s_pid_out); }

/* -------------------------------------------------------- */
/*  Input source / mode                                     */
/* -------------------------------------------------------- */

/**
 * @brief Set input source by name.
 *
 * @sideeffects Modifies s_input_src.
 */
static void cmd_seti(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: seti <ADC|CONSOLE|GAMEPAD|RC>\r\n");
        return;
    }
    for (int i = 0; i < INPUT_COUNT; i++) {
        if (strcasecmp(argv[1], s_inputs[i]) == 0) {
            s_input_src = i;
            printf("I:%s\r\n", s_inputs[i]);
            return;
        }
    }
    printf("Unknown input: %s\r\n", argv[1]);
}

/** @brief Get input source. @sideeffects Writes to stdout. */
static void cmd_geti(int argc, char *argv[])
{ (void)argc; (void)argv; printf("I:%s\r\n", s_inputs[s_input_src]); }
/** @brief Set driving mode. @sideeffects Modifies s_mode. */
static void cmd_setm(int argc, char *argv[])
{
    int tmp;
    if (!parse_int(argc, argv, &tmp)) {
        printf("Usage: setm <mode>\r\n");
        return;
    }
    s_mode = tmp;
    printf("setM:%d\r\n", s_mode);
}
/** @brief Get driving mode. @sideeffects Writes to stdout. */
static void cmd_getm(int argc, char *argv[])
{ (void)argc; (void)argv; printf("getM:%d\r\n", s_mode); }

/* -------------------------------------------------------- */
/*  System commands                                         */
/* -------------------------------------------------------- */

/**
 * @brief Echo text back.
 *
 * @sideeffects Writes to stdout.
 */
static void cmd_echo(int argc, char *argv[])
{
    for (int i = 1; i < argc; i++) {
        printf("%s%s", argv[i], i < argc - 1 ? " " : "");
    }
    printf("\r\n");
}

/**
 * @brief Print firmware version.
 *
 * @sideeffects Writes to stdout.
 */
static void cmd_ver(int argc, char *argv[])
{
    (void)argc; (void)argv;
    esp_chip_info_t ci;
    esp_chip_info(&ci);
    printf("Bobbycar-Steering v0.5\r\n");
    printf("IDF: %s\r\n", esp_get_idf_version());
    printf("Cores: %d  Rev: %d\r\n", ci.cores, ci.revision);
}

/**
 * @brief System reset.
 *
 * @sideeffects Resets the MCU.
 */
static void cmd_reset(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("Resetting...\r\n");
    fflush(stdout);
    esp_restart();
}

/* -------------------------------------------------------- */
/*  Registration                                            */
/* -------------------------------------------------------- */

/** Command table. */
static const shell_cmd_t s_motor_cmds[] = {
    { "echo",  "<text>       Echo text",
      cmd_echo },
    { "ver",   "             Firmware version",
      cmd_ver },
    { "reset", "             System reset",
      cmd_reset },
    { "sets",  "<angle>      Set steering (deg)",
      cmd_sets },
    { "gets",  "             Get steering angle",
      cmd_gets },
    { "getds", "             Get desired steering",
      cmd_getds },
    { "sett",  "<value>      Set throttle",
      cmd_sett },
    { "gett",  "             Get throttle",
      cmd_gett },
    { "getb",  "             Get motor status",
      cmd_getb },
    { "seti",  "<src>        Set input source",
      cmd_seti },
    { "geti",  "             Get input source",
      cmd_geti },
    { "setm",  "<mode>       Set driving mode",
      cmd_setm },
    { "getm",  "             Get driving mode",
      cmd_getm },
    { "setkp", "<val>        Set PID Kp",
      cmd_setkp },
    { "setki", "<val>        Set PID Ki",
      cmd_setki },
    { "setkd", "<val>        Set PID Kd",
      cmd_setkd },
    { "getkp", "             Get PID Kp",
      cmd_getkp },
    { "getki", "             Get PID Ki",
      cmd_getki },
    { "getkd", "             Get PID Kd",
      cmd_getkd },
    { "getpo", "             Get PID output",
      cmd_getpo },
};

#define CMD_COUNT \
    (sizeof(s_motor_cmds) / sizeof(s_motor_cmds[0]))

/* See console_cmds.h for doc comment. */
void console_cmds_register(void)
{
    console_register_commands(s_motor_cmds, CMD_COUNT);
}
