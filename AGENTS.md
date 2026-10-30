# Working agreement

- Maintain the existing `MDK-ARM/bidreaction.uvprojx` as the main application
  project, per the user's integration request (2026-10-29). Preserve forward
  Boost and both reverse modes alongside optimized forward Buck.
- Keep `MDK-ARM/bench12_buck_fast.uvprojx` as the sole restricted bench reference.
  Do not create a new project for each debugging iteration.
- The user operates firmware downloads. Prepare/build/verify and hand off the
  firmware; do not control the computer or board to flash unless asked again.
- Main currently uses explicitly authorized ADC software-protection testing;
  never mark hardware OCP wiring or readiness verified without evidence.
- Preserve history with Git commits/tags and concise test records. Existing
  historical project files are references; they share current source code.
- GitHub SSH remote: `git@github.com:meetyaothenstop/vscode.git` (`origin`).
  After each completed and verified new version or change, commit and push the
  current branch and any tags created for that version to GitHub. This is the
  user's standing authorization; do not wait for a separate upload request.
  Verify the remote branch points to the expected commit before reporting the
  upload complete. If network/authentication blocks a push, retain local commits
  and explicitly report that upload is still pending. Do not force-push.
  If SSH authentication is unavailable, the same repository's HTTPS URL may be
  used with existing Git credentials for the push; retain the SSH origin and
  report the transport used. Never disable host verification to make a push work.
- Keep README.md current with the active project and latest verified outcome.
- Keep rebuildable binaries and IDE UI state out of Git. Never claim board
  timing or electrical behavior is verified from host tests alone.
