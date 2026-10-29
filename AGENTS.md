# Working agreement

- Maintain `MDK-ARM/bench12_buck_fast.uvprojx` as the single active test project.
  The user asked to stop creating a new project for each debugging iteration.
- Preserve history with Git commits/tags and concise test records. Existing
  historical project files are references; they share current source code.
- GitHub SSH remote: `git@github.com:meetyaothenstop/vscode.git` (`origin`).
  After each completed and verified new version or change, commit and push the
  current branch and any tags created for that version to GitHub. This is the
  user's standing authorization; do not wait for a separate upload request.
  Verify the remote branch points to the expected commit before reporting the
  upload complete. If network/authentication blocks a push, retain local commits
  and explicitly report that upload is still pending. Do not force-push.
- Keep README.md current with the active project and latest verified outcome.
- Keep rebuildable binaries and IDE UI state out of Git. Never claim board
  timing or electrical behavior is verified from host tests alone.
