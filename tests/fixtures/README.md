# v0.3 save compatibility fixture

`v03.backup` was produced by the unmodified BLACKWATER application at
`fa39bf3960773304dbf2e91014ef039e1475e5f8`, compiled against its pinned fm1-base host simulator.
It contains synthetic gameplay, no user data or credentials.

The session has current-layer volume 52, operator 1 level 73, reverb send 32,
key D (after personal-best key changes), scale Dorian, automatic fishing, and a populated catch log. User patch 1
contains the level-73 patch. `tests/session_compatibility.txt` restores it through
the platform's actual backup path, checks those values, loads the user patch,
saves, and reboots.
