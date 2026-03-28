/**
 * @file console.h
 * @brief UART console shell with 1180-style help structure.
 *
 * Provides a command-dispatch framework modelled after the
 * bobbycar-project command_interpreter, using the 1180 help
 * format (name + description in the command table).
 *
 * Commands are registered via console_register_commands().
 */

#pragma once

#include <stdbool.h>

/** Maximum registered commands across all groups. */
#define CONSOLE_MAX_COMMANDS  48

/** Maximum length of one input line. */
#define CONSOLE_LINE_MAX      128

/** Maximum arguments per command (including cmd name). */
#define CONSOLE_ARGC_MAX      8

/**
 * @brief One shell command entry (1180-style).
 *
 * The help command iterates this table to print aligned
 * name + description pairs.
 */
typedef struct {
    const char *name;   /**< Command keyword.            */
    const char *desc;   /**< Usage + short description.  */
    void (*handler)(int argc, char *argv[]);
} shell_cmd_t;

/**
 * @brief Initialise the console and start the UART task.
 *
 * Creates a FreeRTOS task that reads lines from stdin and
 * dispatches them through the registered command table.
 *
 * @sideeffects Starts a FreeRTOS task, reads from UART.
 */
void console_init(void);

/**
 * @brief Register a group of commands.
 *
 * Multiple groups can be registered.  Duplicate names are
 * not checked — the first match wins.
 *
 * @param[in] cmds   Array of shell_cmd_t entries.
 * @param[in] count  Number of entries in @p cmds.
 *
 * @sideeffects Extends the internal command table.
 */
void console_register_commands(const shell_cmd_t *cmds,
                               int count);

/**
 * @brief Execute a single command line (for tests).
 *
 * Parses the line, looks up the command, and calls the
 * handler.  Returns true if a valid command was found.
 *
 * @param[in] line  NUL-terminated command string.
 * @return true if command found and executed.
 */
bool console_exec(const char *line);
