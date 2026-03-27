---
applyTo: "**/*.c,**/*.h,**/*.S,**/CMakeLists.txt"
---

# Todo Documentation — Instructions for Copilot

## When to execute

**On every feature request — after updating `requirements.md`, before writing code.**
Also **after every implementation step** — tick completed items immediately.

## Target file

`Documentation/ToDo/<feature-name>.md`
The file is named after the feature using `kebab-case` (e.g., `steering-control.md`, `display-driver.md`).
Create the file when the feature is first requested. Update it after every implementation step.

---

## File format

```markdown
# Todo: <Feature Name>

**Created:** YYYY-MM-DD
**Requirement:** [Req #N — <Feature Name>](../Requirements/requirements.md)
**Status:** In Progress / Done

---

## Tasks

- [ ] <Top-level task 1>
  - [ ] <Sub-task 1.1>
  - [ ] <Sub-task 1.2>
- [ ] <Top-level task 2>
- [ ] Build passes with feature integrated (`idf.py build`)
- [ ] Flashed and verified on hardware (`idf.py flash monitor`)
- [ ] CHANGE_LOG.md updated
- [ ] requirements.md updated (if scope changed)

---

## Notes

<Optional: blockers, open questions, platform-specific quirks, deferred items.>
```

---

## Rules

- **MUST** create the todo file when a feature is requested, immediately after updating `requirements.md`.
- **MUST** tick (`- [x]`) each task as soon as that implementation step is complete — do not batch-tick at the end.
- **MUST** include the final standard tasks (build, flash/verify, CHANGE_LOG update) in every file.
- **MUST** update the `**Status:**` line to `Done` once all checkboxes are ticked.
- **MUST NOT** delete todo files when done — they serve as implementation history.
- **SHOULD** break each top-level task into sub-tasks when it involves more than one file or function.
