/**
 * @file ini_parser.c
 * @brief Minimal INI file parser implementation.
 *
 * Parses INI files with [sections], key=value pairs, and
 * # or ; comment lines.  Rewrites files for value updates.
 */

#include "ini_parser.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* -------------------------------------------------------- */
/*  Helpers                                                 */
/* -------------------------------------------------------- */

/**
 * @brief Trim leading and trailing whitespace in-place.
 *
 * @param[in,out] s  String to trim.
 * @return Pointer to trimmed start within @p s.
 */
static char *trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) {
        s++;
    }
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end-- = '\0';
    }
    return s;
}

/* -------------------------------------------------------- */
/*  Parser                                                  */
/* -------------------------------------------------------- */

/* See ini_parser.h for doc comment. */
int ini_parse_file(const char *path,
                   ini_handler_t handler,
                   void *user)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }

    char line[INI_LINE_MAX];
    char section[INI_SECTION_MAX] = "";
    int count = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        /* Strip newline. */
        size_t len = strlen(line);
        while (len > 0
               && (line[len - 1] == '\n'
                   || line[len - 1] == '\r')) {
            line[--len] = '\0';
        }

        char *p = trim(line);

        /* Skip empty lines and comments. */
        if (*p == '\0' || *p == '#' || *p == ';') {
            continue;
        }

        /* Section header. */
        if (*p == '[') {
            char *end = strchr(p, ']');
            if (end) {
                *end = '\0';
                strncpy(section, p + 1,
                        INI_SECTION_MAX - 1);
                section[INI_SECTION_MAX - 1] = '\0';
            }
            continue;
        }

        /* Key=value pair. */
        char *eq = strchr(p, '=');
        if (!eq) {
            continue;
        }

        *eq = '\0';
        char *key = trim(p);
        char *val = trim(eq + 1);

        if (*key == '\0') {
            continue;
        }

        count++;
        if (!handler(section, key, val, user)) {
            break;
        }
    }

    fclose(f);
    return count;
}

/* -------------------------------------------------------- */
/*  Writer                                                  */
/* -------------------------------------------------------- */

/* See ini_parser.h for doc comment. */
int ini_write_value(const char *path,
                    const char *section,
                    const char *key,
                    const char *value)
{
    /*
     * Strategy: read existing file into a line buffer,
     * replace the matching key in the matching section,
     * or append section+key if not found.  Then rewrite.
     */
    #define MAX_LINES 64
    static char lines[MAX_LINES][INI_LINE_MAX];
    int n_lines = 0;
    bool found = false;

    /* Read existing file (if it exists). */
    FILE *f = fopen(path, "r");
    if (f) {
        while (n_lines < MAX_LINES
               && fgets(lines[n_lines],
                        INI_LINE_MAX, f) != NULL) {
            n_lines++;
        }
        fclose(f);
    }

    /* Search for matching section + key. */
    char cur_sec[INI_SECTION_MAX] = "";
    int insert_line = -1;  /* Where to insert if key missing. */

    for (int i = 0; i < n_lines; i++) {
        char tmp[INI_LINE_MAX];
        strncpy(tmp, lines[i], INI_LINE_MAX - 1);
        tmp[INI_LINE_MAX - 1] = '\0';

        /* Strip newline for parsing. */
        size_t len = strlen(tmp);
        while (len > 0
               && (tmp[len - 1] == '\n'
                   || tmp[len - 1] == '\r')) {
            tmp[--len] = '\0';
        }

        char *p = trim(tmp);

        if (*p == '[') {
            char *end = strchr(p, ']');
            if (end) {
                *end = '\0';
                strncpy(cur_sec, p + 1,
                        INI_SECTION_MAX - 1);
                cur_sec[INI_SECTION_MAX - 1] = '\0';
            }
            /* Mark end of matching section for insert. */
            if (strcmp(cur_sec, section) == 0) {
                insert_line = i + 1;
            }
            continue;
        }

        /* Check if in matching section. */
        if (strcmp(cur_sec, section) != 0) {
            continue;
        }

        /* Track last line in section for insertion. */
        insert_line = i + 1;

        /* Check key match. */
        char *eq = strchr(p, '=');
        if (!eq) {
            continue;
        }
        *eq = '\0';
        char *k = trim(p);
        if (strcmp(k, key) == 0) {
            /* Replace this line. */
            snprintf(lines[i], INI_LINE_MAX,
                     "%s=%s\n", key, value);
            found = true;
            break;
        }
    }

    if (!found) {
        /* Need to add the entry. */
        if (insert_line < 0) {
            /* Section does not exist — append it. */
            if (n_lines < MAX_LINES) {
                snprintf(lines[n_lines], INI_LINE_MAX,
                         "\n[%s]\n", section);
                n_lines++;
            }
            if (n_lines < MAX_LINES) {
                snprintf(lines[n_lines], INI_LINE_MAX,
                         "%s=%s\n", key, value);
                n_lines++;
            }
        } else {
            /* Insert after last line in section. */
            if (n_lines < MAX_LINES) {
                /* Shift lines down. */
                for (int i = n_lines; i > insert_line; i--) {
                    memcpy(lines[i], lines[i - 1],
                           INI_LINE_MAX);
                }
                snprintf(lines[insert_line], INI_LINE_MAX,
                         "%s=%s\n", key, value);
                n_lines++;
            }
        }
    }

    /* Write back. */
    f = fopen(path, "w");
    if (!f) {
        return -1;
    }
    for (int i = 0; i < n_lines; i++) {
        fputs(lines[i], f);
    }
    fclose(f);
    return 0;
    #undef MAX_LINES
}
