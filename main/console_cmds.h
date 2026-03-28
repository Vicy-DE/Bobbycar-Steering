/**
 * @file console_cmds.h
 * @brief Console command registration prototypes.
 *
 * Declares the registration functions that each command
 * group file provides.  Called from main during init.
 */

#pragma once

/**
 * @brief Register motor/steering/system commands.
 *
 * @sideeffects Adds entries to the console command table.
 */
void console_cmds_register(void);

/**
 * @brief Register filesystem and XMODEM commands.
 *
 * @sideeffects Adds entries to the console command table.
 */
void console_cmds_fs_register(void);
