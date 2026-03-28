/**
 * @file console_cmds_fs.c
 * @brief Filesystem and XMODEM console commands.
 *
 * Implements 1180-style file commands operating on LittleFS:
 * ls, cd, pwd, rm, mkdir, show, recv, send, format.
 * Uses XMODEM-CRC for file transfer over UART.
 */

#include "console_cmds.h"
#include "console.h"
#include "xmodem.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
#include <errno.h>
#include "esp_log.h"
#include "esp_littlefs.h"

static const char *TAG __attribute__((unused))
    = "fs_cmd";

/* Base path of the mounted LittleFS partition. */
#define FS_ROOT "/littlefs"

/** Current working directory (absolute path). */
static char s_cwd[128] = FS_ROOT;

/** File receive buffer (8 KB). */
#define XMODEM_BUF_SIZE  (8 * 1024)
static uint8_t s_xm_buf[XMODEM_BUF_SIZE];

/* ------------------------------------------------------ */
/*  Helpers                                               */
/* ------------------------------------------------------ */

/**
 * @brief Resolve a user path to an absolute path.
 *
 * If @p arg starts with '/' it is taken as absolute under
 * FS_ROOT, otherwise it is appended to s_cwd.
 *
 * @param[in]  arg   User-supplied path (may be NULL).
 * @param[out] out   Buffer for resolved path.
 * @param[in]  size  Buffer size.
 * @return true on success.
 */
static bool resolve_path(const char *arg,
                         char *out, size_t size)
{
    if (arg == NULL || arg[0] == '\0') {
        strncpy(out, s_cwd, size - 1);
        out[size - 1] = '\0';
        return true;
    }
    if (arg[0] == '/') {
        int n = snprintf(out, size, "%s%s",
                         FS_ROOT, arg);
        return n > 0 && (size_t)n < size;
    }
    int n = snprintf(out, size, "%s/%s", s_cwd, arg);
    return n > 0 && (size_t)n < size;
}

/**
 * @brief Validate path stays within FS_ROOT.
 *
 * @param[in] path  Absolute path.
 * @return true if safe.
 */
static bool path_is_safe(const char *path)
{
    if (strstr(path, "..") != NULL) {
        return false;
    }
    if (strncmp(path, FS_ROOT,
                strlen(FS_ROOT)) != 0) {
        return false;
    }
    return true;
}

/* ------------------------------------------------------ */
/*  File commands                                         */
/* ------------------------------------------------------ */

/**
 * @brief List directory contents.
 *
 * @sideeffects Reads directory, writes to stdout.
 */
static void cmd_ls(int argc, char *argv[])
{
    char path[128];
    resolve_path(argc > 1 ? argv[1] : NULL,
                 path, sizeof(path));
    if (!path_is_safe(path)) {
        printf("Error: invalid path\r\n");
        return;
    }

    DIR *d = opendir(path);
    if (!d) {
        printf("Cannot open: %s\r\n", path);
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        char full[384];
        snprintf(full, sizeof(full),
                 "%s/%s", path, ent->d_name);
        struct stat st;
        if (stat(full, &st) == 0 && S_ISDIR(st.st_mode)) {
            printf("  [DIR]  %s\r\n", ent->d_name);
        } else {
            long sz = 0;
            if (stat(full, &st) == 0) {
                sz = (long)st.st_size;
            }
            printf("  %6ld %s\r\n", sz, ent->d_name);
        }
    }
    closedir(d);
}

/**
 * @brief Change working directory.
 *
 * @sideeffects Modifies s_cwd.
 */
static void cmd_cd(int argc, char *argv[])
{
    if (argc < 2) {
        strncpy(s_cwd, FS_ROOT, sizeof(s_cwd));
        printf("%s\r\n", s_cwd);
        return;
    }
    char path[128];
    resolve_path(argv[1], path, sizeof(path));
    if (!path_is_safe(path)) {
        printf("Error: invalid path\r\n");
        return;
    }
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        printf("Not a directory: %s\r\n", path);
        return;
    }
    strncpy(s_cwd, path, sizeof(s_cwd) - 1);
    s_cwd[sizeof(s_cwd) - 1] = '\0';
    printf("%s\r\n", s_cwd);
}

/**
 * @brief Print working directory.
 *
 * @sideeffects Writes to stdout.
 */
static void cmd_pwd(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("%s\r\n", s_cwd);
}

/**
 * @brief Remove a file.
 *
 * @sideeffects Deletes a file from LittleFS.
 */
static void cmd_rm(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: rm <path>\r\n");
        return;
    }
    char path[128];
    resolve_path(argv[1], path, sizeof(path));
    if (!path_is_safe(path)) {
        printf("Error: invalid path\r\n");
        return;
    }
    if (unlink(path) != 0) {
        printf("Failed: %s\r\n", strerror(errno));
    } else {
        printf("Removed: %s\r\n", path);
    }
}

/**
 * @brief Create a directory.
 *
 * @sideeffects Creates a directory on LittleFS.
 */
static void cmd_mkdir(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: mkdir <path>\r\n");
        return;
    }
    char path[128];
    resolve_path(argv[1], path, sizeof(path));
    if (!path_is_safe(path)) {
        printf("Error: invalid path\r\n");
        return;
    }
    if (mkdir(path, 0755) != 0) {
        printf("Failed: %s\r\n", strerror(errno));
    } else {
        printf("Created: %s\r\n", path);
    }
}

/**
 * @brief Display file contents (like cat).
 *
 * @sideeffects Writes file contents to stdout.
 */
static void cmd_show(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: show <path>\r\n");
        return;
    }
    char path[128];
    resolve_path(argv[1], path, sizeof(path));
    if (!path_is_safe(path)) {
        printf("Error: invalid path\r\n");
        return;
    }
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Cannot open: %s\r\n", path);
        return;
    }
    char buf[128];
    while (fgets(buf, sizeof(buf), f) != NULL) {
        printf("%s", buf);
    }
    printf("\r\n");
    fclose(f);
}

/**
 * @brief Receive a file via XMODEM-CRC.
 *
 * @sideeffects Reads from UART, writes to LittleFS.
 */
static void cmd_recv(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: recv <path>\r\n");
        return;
    }
    char path[128];
    resolve_path(argv[1], path, sizeof(path));
    if (!path_is_safe(path)) {
        printf("Error: invalid path\r\n");
        return;
    }
    printf("Ready to receive '%s' via XModem-CRC."
           " Start transfer now...\r\n", argv[1]);

    long result = xmodem_receive(
        s_xm_buf, XMODEM_BUF_SIZE);
    if (result < 0) {
        printf("\r\nXModem error: %ld\r\n", result);
        return;
    }

    FILE *f = fopen(path, "wb");
    if (!f) {
        printf("Cannot create: %s\r\n", path);
        return;
    }
    fwrite(s_xm_buf, 1, (size_t)result, f);
    fclose(f);
    printf("\r\nReceived %ld bytes -> '%s'\r\n",
           result, argv[1]);
}

/**
 * @brief Send a file via XMODEM-CRC.
 *
 * @sideeffects Reads from LittleFS, writes to UART.
 */
static void cmd_send(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: send <path>\r\n");
        return;
    }
    char path[128];
    resolve_path(argv[1], path, sizeof(path));
    if (!path_is_safe(path)) {
        printf("Error: invalid path\r\n");
        return;
    }
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("Cannot open: %s\r\n", path);
        return;
    }
    size_t n = fread(s_xm_buf, 1, XMODEM_BUF_SIZE, f);
    fclose(f);

    printf("Sending '%s' (%u bytes) via XModem-CRC."
           " Start receive now...\r\n",
           argv[1], (unsigned)n);

    long result = xmodem_send(s_xm_buf, n);
    if (result >= 0) {
        printf("\r\nSent %ld bytes\r\n", result);
    } else {
        printf("\r\nXModem error: %ld\r\n", result);
    }
}

/**
 * @brief Format the LittleFS partition.
 *
 * @sideeffects Erases and re-formats LittleFS.
 */
static void cmd_format(int argc, char *argv[])
{
    (void)argc; (void)argv;
    printf("Formatting LittleFS...\r\n");
    esp_err_t ret = esp_littlefs_format("littlefs");
    if (ret == ESP_OK) {
        printf("Format complete\r\n");
    } else {
        printf("Format failed: %s\r\n",
               esp_err_to_name(ret));
    }
}

/* ------------------------------------------------------ */
/*  Registration                                          */
/* ------------------------------------------------------ */

static const shell_cmd_t s_fs_cmds[] = {
    { "ls",     "[path]       List directory",
      cmd_ls },
    { "cd",     "[path]       Change directory",
      cmd_cd },
    { "pwd",    "             Print working dir",
      cmd_pwd },
    { "rm",     "<path>       Delete file",
      cmd_rm },
    { "mkdir",  "<path>       Create directory",
      cmd_mkdir },
    { "show",   "<path>       Show file contents",
      cmd_show },
    { "recv",   "<path>       Receive via XModem",
      cmd_recv },
    { "send",   "<path>       Send via XModem",
      cmd_send },
    { "format", "             Format filesystem",
      cmd_format },
};

#define FS_CMD_COUNT \
    (sizeof(s_fs_cmds) / sizeof(s_fs_cmds[0]))

/* See console_cmds.h for doc comment. */
void console_cmds_fs_register(void)
{
    console_register_commands(s_fs_cmds, FS_CMD_COUNT);
}
