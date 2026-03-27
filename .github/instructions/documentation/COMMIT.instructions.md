---
applyTo: "**/*.c,**/*.h,**/*.S,**/CMakeLists.txt,**/*.bat,**/*.ps1,**/*.py,**/*.sh"
---

# Commit — Instructions for Copilot

## When to execute

**Final step of every task — after all of the following are confirmed:**

1. Build passes (`idf.py build` — no errors, no warnings).
2. Tests pass (see [TEST_DOC](TEST_DOC.instructions.md)).
3. Change log updated (`Documentation/CHANGE_LOG.md`).
4. Project doc updated (`Documentation/PROJECT_DOC.md`).
5. Test report saved (`Documentation/Tests/<YYYY-MM-DD>_<feature>.md`).

**NEVER push the commit.** Stage and commit only.

---

## Step 1 — Stage all relevant files

Stage only files that belong to the current task.
Do **not** stage unrelated modifications, generated build artefacts, or binary outputs.

```powershell
# Stage specific files (preferred — be explicit)
git add <file1> <file2> ...

# Or stage all tracked changes if the entire working tree belongs to this task
git add -u
```

Files to **always exclude** from staging:
- `build/` (all build outputs)
- `managed_components/` (ESP-IDF managed components cache)
- `sdkconfig.old`
- `*.elf`, `*.bin`, `*.hex`, `*.map`

---

## Step 2 — Write the commit message

Use the **Conventional Commits** format:

```
<type>(<scope>): <short imperative summary>

<body — explain the what and the why, not the how>

<footer — references, breaking changes>
```

### Type

| Type | When to use |
|---|---|
| `feat` | New feature or capability |
| `fix` | Bug fix |
| `refactor` | Code restructuring without behaviour change |
| `test` | Adding or updating tests |
| `docs` | Documentation only |
| `chore` | Build, tooling, config changes |
| `perf` | Performance improvement |

### Scope (use the affected module)

`main` · `steering` · `display` · `lvgl` · `components` · `cmake` · `target` · `docs`

### Summary line rules
- Imperative mood, present tense: "add", "fix", "remove" — not "added" or "fixes".
- No trailing period.
- Max 72 characters.

### Body rules
- Separate from the summary with a blank line.
- Explain **what** changed and **why** (the motivation, the bug being fixed, the requirement being met).
- Wrap at 72 characters per line.
- Reference relevant requirement IDs, issue numbers, or test case IDs where applicable.

### Footer rules
- `BREAKING CHANGE:` if the change alters a public interface or partition layout.
- `Refs: #<issue>` or `Refs: Requirements/<req>.md` where applicable.
- **Never** include `git push` commands or suggestions.

---

## Step 3 — Commit

```powershell
git commit -m "<summary line>" -m "<body paragraph 1>" -m "<body paragraph 2 if needed>"
```

---

## Rules

- **MUST** verify all prerequisites (build, tests, docs) before committing.
- **MUST** use Conventional Commits format.
- **MUST NOT** commit build artefacts or generated binaries.
- **MUST NOT** run `git push` at any point — local commit only.
- **MUST NOT** amend commits that have already been pushed (even if `--force` would work).
- **SHOULD** keep each commit focused on a single logical change.
- **SHOULD** include the test report file path in the commit body.
