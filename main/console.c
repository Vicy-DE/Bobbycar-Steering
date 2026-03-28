/**
 * @file console.c
 * @brief UART console task and command dispatch.
 *
 * Implements the console infrastructure: a FreeRTOS task that
 * reads lines from stdin (USB-CDC), parses them into argc/argv,
 * and dispatches to matching handlers.  Uses the 1180 help
 * format and the bobbycar exec() dispatch pattern.
 */

#include "console.h"
#include "ble_console.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "console";

/* ------------------------------------------------------ */
/*  Command registry                                      */
/* ------------------------------------------------------ */

static shell_cmd_t s_cmds[CONSOLE_MAX_COMMANDS];
static int         s_cmd_count = 0;

/* ------------------------------------------------------ */
/*  Built-in: help                                        */
/* ------------------------------------------------------ */

/**
 * @brief Print all registered commands (1180 format).
 *
 * @param[in] argc  Argument count (unused).
 * @param[in] argv  Argument vector (unused).
 *
 * @sideeffects Writes to stdout.
 */
static void cmd_help(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    printf("Available commands:\r\n");
    for (int i = 0; i < s_cmd_count; i++) {
        printf("  %-12s %s\r\n",
               s_cmds[i].name,
               s_cmds[i].desc);
    }
}

/* ------------------------------------------------------ */
/*  Parsing                                               */
/* ------------------------------------------------------ */

/**
 * @brief Split a line into argc/argv tokens.
 *
 * Modifies the input buffer in-place (inserts NULs).
 *
 * @param[in,out] line  Mutable command string.
 * @param[out]    argv  Array of token pointers.
 * @param[in]     max   Capacity of @p argv.
 * @return Number of tokens found.
 */
static int parse_line(char *line,
                      char *argv[],
                      int max)
{
    int argc = 0;
    char *p = line;

    while (*p && argc < max) {
        /* Skip whitespace. */
        while (*p && (*p == ' ' || *p == '\t')) {
            p++;
        }
        if (*p == '\0' || *p == '\n' || *p == '\r') {
            break;
        }
        argv[argc++] = p;
        /* Advance to next whitespace. */
        while (*p && *p != ' ' && *p != '\t'
               && *p != '\n' && *p != '\r') {
            p++;
        }
        if (*p) {
            *p++ = '\0';
        }
    }
    return argc;
}

/* ------------------------------------------------------ */
/*  Dispatch                                              */
/* ------------------------------------------------------ */

/* See console.h for doc comment. */
bool console_exec(const char *line)
{
    char buf[CONSOLE_LINE_MAX];
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    char *argv[CONSOLE_ARGC_MAX];
    int argc = parse_line(buf, argv, CONSOLE_ARGC_MAX);
    if (argc == 0) {
        return false;
    }

    /* Convert command name to lower case. */
    for (char *c = argv[0]; *c; c++) {
        *c = (char)tolower((unsigned char)*c);
    }

    for (int i = 0; i < s_cmd_count; i++) {
        if (strcmp(s_cmds[i].name, argv[0]) == 0) {
            s_cmds[i].handler(argc, argv);
            return true;
        }
    }

    console_printf("Unknown command: %s\r\n", argv[0]);
    return false;
}

/* ------------------------------------------------------ */
/*  Registration                                          */
/* ------------------------------------------------------ */

/* See console.h for doc comment. */
void console_register_commands(const shell_cmd_t *cmds,
                               int count)
{
    for (int i = 0; i < count; i++) {
        if (s_cmd_count >= CONSOLE_MAX_COMMANDS) {
            ESP_LOGE(TAG, "Command table full");
            return;
        }
        s_cmds[s_cmd_count++] = cmds[i];
    }
}

/* ------------------------------------------------------ */
/*  Console task                                          */
/* ------------------------------------------------------ */

/**
 * @brief FreeRTOS task reading lines from UART/stdin.
 *
 * Reads one line at a time via fgets, echoes it, then
 * dispatches through console_exec().
 *
 * @param[in] arg  Unused.
 *
 * @sideeffects Reads from stdin, writes to stdout.
 */
static void console_task(void *arg)
{
    (void)arg;
    char line[CONSOLE_LINE_MAX];

    ESP_LOGI(TAG, "Console ready (type 'help')");
    printf("\r\n> ");
    fflush(stdout);

    while (1) {
        if (fgets(line, sizeof(line), stdin) != NULL) {
            /* Strip trailing newline. */
            size_t len = strlen(line);
            while (len > 0
                   && (line[len - 1] == '\n'
                       || line[len - 1] == '\r')) {
                line[--len] = '\0';
            }
            if (len > 0) {
                console_exec(line);
            }
            printf("> ");
            fflush(stdout);
        } else {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}

/* ------------------------------------------------------ */
/*  Init                                                  */
/* ------------------------------------------------------ */

/* See console.h for doc comment. */
void console_init(void)
{
    /* Register the built-in help command. */
    static const shell_cmd_t help_cmd = {
        "help", "Show this help", cmd_help
    };
    console_register_commands(&help_cmd, 1);

    xTaskCreate(console_task, "console",
                4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "Console task started");
}
