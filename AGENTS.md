# BLACKWATER development rules

Read `PROJECT.md`, `docs/TECHNICAL_AUDIT.md`, and recent Git history before work.

The user owns product direction and approves features, interface behavior, priorities,
and major architecture choices. Research and recommendations do not authorize adoption
or implementation. Keep approved requirements separate from proposals in `PROJECT.md`.
Make routine engineering decisions independently within approved scope.

Use established M-VAVE FM-1 infrastructure and its existing simulator. Application code,
controls, display, storage, and audio must use the real firmware architecture. Do not
create a replacement emulator, disconnected web demo, or JavaScript sound engine.
Preserve working components and upstream license notices.

For approved features: implement, compile affected targets, run meaningful tests,
exercise the existing simulator, investigate failures, update the project record, and
commit working changes. Distinguish compilation, host simulation, WebAssembly testing,
browser interaction, and physical-device testing. Never claim unperformed verification.

Use the existing checkout: cloud tasks are already isolated; do not create a worktree
unless the user requests one. Do not publish or deploy without authorization.
