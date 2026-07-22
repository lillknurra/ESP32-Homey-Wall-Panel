# Development Workflow

1. Audit branch, local HEAD, remote HEAD, worktree, and documented baseline.
2. Define one patch purpose, non-goals, expected files, validation, rollback, and commit message.
3. Implement locally using deterministic scripts when practical.
4. Run static checks and scope-appropriate validation.
5. Inspect the exact diff.
6. Stage exact files.
7. Commit and push only after accepted validation.
8. Verify remote commit and open a draft PR.
9. Lock durable documentation from accepted evidence. The intended stable
   post-merge state must identify `main` as the stable branch and must not
   invent or authorize a subsequent patch.
10. After merge, verify the merged `main` ref. A bounded finalization patch may
    record the preceding merge, but it must be self-finalizing and must not
    require another state-lock patch solely to record its own merge.
11. Start later work only after a separate patch scope decision.
