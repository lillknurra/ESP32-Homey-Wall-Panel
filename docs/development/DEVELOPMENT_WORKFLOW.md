# Development Workflow

1. Audit branch, local HEAD, remote HEAD, worktree, and documented baseline.
2. Define one patch purpose, non-goals, expected files, validation, rollback, and commit message.
3. Implement locally using deterministic scripts when practical.
4. Run static checks and scope-appropriate validation.
5. Inspect the exact diff.
6. Stage exact files.
7. Commit and push only after accepted validation.
8. Verify remote commit and open a draft PR.
9. Lock documentation from accepted evidence.
10. Complete only after merge and merged-baseline verification.
