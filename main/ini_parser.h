/**
 * @file ini_parser.h
 * @brief Minimal INI file parser for LittleFS config files.
 *
 * Parses INI files with sections, key=value pairs, and comments.
 * Uses a callback interface to deliver parsed entries.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum length of a section name. */
#define INI_SECTION_MAX  32

/** Maximum length of a key. */
#define INI_KEY_MAX      32

/** Maximum length of a value. */
#define INI_VALUE_MAX    96

/** Maximum line length in an INI file. */
#define INI_LINE_MAX     128

/**
 * @brief Callback invoked for each key=value pair.
 *
 * @param[in] section  Current section name (empty string if none).
 * @param[in] key      Key string (trimmed).
 * @param[in] value    Value string (trimmed).
 * @param[in] user     User context pointer.
 * @return true to continue parsing, false to stop.
 */
typedef bool (*ini_handler_t)(const char *section,
                              const char *key,
                              const char *value,
                              void *user);

/**
 * @brief Parse an INI file from the filesystem.
 *
 * Reads line-by-line, calls @p handler for each key=value pair.
 * Lines starting with '#' or ';' are comments. Sections are
 * enclosed in brackets: [section_name].
 *
 * @param[in] path     Absolute filesystem path.
 * @param[in] handler  Callback for each entry.
 * @param[in] user     User context passed to handler.
 * @return Number of entries parsed, or -1 on file open error.
 */
int ini_parse_file(const char *path,
                   ini_handler_t handler,
                   void *user);

/**
 * @brief Write a key=value pair to an INI file.
 *
 * Opens the file, finds the matching section and key, and
 * replaces the value. If the section or key does not exist,
 * appends them. Creates the file if it does not exist.
 *
 * @param[in] path     Absolute filesystem path.
 * @param[in] section  Section name (without brackets).
 * @param[in] key      Key to write.
 * @param[in] value    Value to write.
 * @return 0 on success, -1 on error.
 *
 * @sideeffects Writes to LittleFS.
 */
int ini_write_value(const char *path,
                    const char *section,
                    const char *key,
                    const char *value);

#ifdef __cplusplus
}
#endif
