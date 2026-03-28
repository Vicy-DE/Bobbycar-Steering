# Todo: Console, Motors, Steering Algorithm

**Created:** 2026-03-28
**Requirement:** [Req #11-14 — Console, XMODEM, Motors, Steering](../Requirements/requirements.md)
**Status:** Complete

---

## Tasks

- [x] Read 1180 console requirements (XMODEM, help structure)
- [x] Read bobbycar-project console code (command_interpreter, console_task)
- [x] Read bobbycar-project motor/steering code (math_functions, config)
- [x] Design console architecture (1180 help + bobbycar dispatch)
- [x] Create motor.h / motor.c — 4 motor objects
- [x] Create steering_algo.h / steering_algo.c — Ackermann algorithm
- [x] Create console.h / console.c — UART task + dispatch
- [x] Create console_cmds.h / console_cmds.c — motor/steering/system cmds
- [x] Create xmodem.h / xmodem.c — XMODEM-CRC protocol
- [x] Create console_cmds_fs.c — filesystem + XMODEM commands
- [x] Integrate into main.c (motor_init, console_init)
- [x] Update CMakeLists.txt with new source files
- [x] Verify all files < 400 lines, all lines < 100 chars
- [x] Update requirements.md
- [x] Build passes with feature integrated (`idf.py build`)
- [x] Flashed and verified on hardware (`idf.py flash monitor`)
- [x] CHANGE_LOG.md updated
- [x] PROJECT_DOC.md updated
- [x] requirements.md updated (if scope changed)

---

## Notes

- Software design: bobbycar-project dispatch pattern (exec → table lookup → handler)
- Help format: 1180-style (name + description columns)
- XMODEM uses getchar/putchar for I/O — works with USB-CDC stdin/stdout
- Motor objects are data-only for now (no serial hoverboard comm yet)
- Steering algo uses all bobbycar geometry constants (cm units)
