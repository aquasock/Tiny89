# Workflow run workflow_bad4044e

- status: Failed
- goal: Build a Tiny89-owned AE350 platform whose video path shows the calculator's 160x100 LCD at 7x integer scale (1120x700, square pixels, centred in 1280x720) with four-level grayscale, forked from TinyNES at a pinned commit, built with the open flow and verified; deployment to the Tang waits for the user's explicit OK.
- started_at_ms: 1792071180373
- completed_at_ms: 1792071493548
- child_agents: 2
- error: script error: Error: Required Workflow result unavailable: lcd-gray
    at <anonymous> (eval_script:13:180)
    at forEach (native)
    at <anonymous> (eval_script:13:86)


## Progress

- phase: workflow: Build a Tiny89-owned AE350 platform whose video path shows the calculator's 160x100 LCD at 7x integer scale (1120x700, square pixels, centred in 1280x720) with four-level grayscale, forked from TinyNES at a pinned commit, built with the open flow and verified; deployment to the Tang waits for the user's explicit OK.
- phase: phase-firmware-and-fork
- log: parallel(): dropped a failed slot as null (kind=agent, slot 1): task(): subagent failed: Sub-agent delivery verification failed: Compared with the workspace at spawn: declared but unchanged: []; observed changes without a change declaration: ["docs/tiny89-soc.md"]. Inspect the worker receipt.
- log: parallel(): dropped a failed slot as null (kind=budget, slot 0): task(): budget exhausted: sub-agent budget exhausted
- log: parallel(): every slot failed (2 of 2); no work survived this fan-out
- phase: phase-video
