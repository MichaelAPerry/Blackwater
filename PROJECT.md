# BLACKWATER project record

## Approved requirements and user decisions

- Original M-VAVE FM-1 application inspired by aquatic mysteries, freshwater creatures,
  fishing expeditions, and the investigative atmosphere of River Monsters.
- The user is product owner, creative director, and final decision-maker.
- Use established FM-1 firmware and simulator infrastructure, with genuine application
  code and audio paths. Preserve practical physical-device compatibility.
- Simulator-first development. No replacement emulator or disconnected browser demo.
- Initial assignment is a technical audit and upstream simulator baseline, followed by
  a foundation recommendation for approval. This supersedes the earlier broad request
  to build an initial codebase.

## Proposed, awaiting approval

- Adopt a pinned `charlesvestal/fm1-base` revision and its pinned `felucca-bsp`, with a
  separate BLACKWATER C application and the existing host/browser simulator.
- No synthesizer engines, sequencing, fishing mechanics, environmental simulation,
  generative behavior, or application interface are approved. These remain possibilities,
  not a roadmap.

## Completed

- Audited the initially license-only checkout and four requested upstream repositories.
- Built and exercised the upstream fm1-base template desktop simulator in a separate
  research checkout; this is not a BLACKWATER implementation.
- Recorded source references, reproducible baseline commands, constraints, and results
  in `docs/TECHNICAL_AUDIT.md`.

## Current tasks and pending decisions

- Obtain foundation approval before importing framework or template code.
- User to define the first application behavior and minimum acceptable experience.
- After approval: integrate the selected foundation, implement the approved scope,
  validate host/browser/device builds, and document reproducible setup.
- GitHub Pages publication is pending separate user approval and a working BLACKWATER
  emulator; no deployment has been performed.

## Known bugs and limitations

- No BLACKWATER application exists yet, so no application behavior is verified.
- Browser toolchain installation encountered an HTTP 403 network-policy denial at
  `storage.googleapis.com`; browser build and interaction testing remain unverified.
- No physical hardware is attached or tested; desktop speed is not a device CPU budget.
- The unmodified fm1-base platform suite has reproducible USB screen-mirroring
  failures here (`mirror`, `mirror-viewer`); 56 named groups passed. Root cause is
  unconfirmed. Firmware update-path and `.rfm1` checks were skipped.
- No foundation has been adopted and no license change has been made.
