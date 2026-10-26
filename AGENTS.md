# Working agreement

- Maintain `MDK-ARM/bench12_buck_fast.uvprojx` as the single active test project.
  The user asked to stop creating a new project for each debugging iteration.
- Preserve history with Git commits/tags and concise test records. Existing
  historical project files are references; they share current source code.
- Keep README.md current with the active project and latest verified outcome.
- Keep rebuildable binaries and IDE UI state out of Git. Never claim board
  timing or electrical behavior is verified from host tests alone.
