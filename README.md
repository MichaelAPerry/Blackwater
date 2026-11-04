# BLACKWATER

A generative fish music maker for the M-VAVE FM-1, under user-led development.
Four tracks use editable six-operator FM voices. Placing hooks enables generative
music; automatic or timed catches vary each track's theme and record fish sizes.

The first C application runs through the existing **fm1-base desktop simulator**,
including genuine firmware audio, controls, display and storage paths. Generation is
currently simple random pitch/rhythm mutation of short patterns, not finished musical
behavior. The tracks are Bluegill, Largemouth Bass, Brown Trout and Catfish; species
art/sound design, browser/device validation and the Jeremy Wade clip remain unfinished.

```sh
./tools/setup.sh
. build/venv/bin/activate
./tests/run_tests.sh
```

- [Build instructions, controls, tests and limitations](docs/DEVELOPMENT.md)
- [Project status and approved requirements](PROJECT.md)
- [Technical audit](docs/TECHNICAL_AUDIT.md)
- [Contributor instructions](AGENTS.md)
- [Third-party notices](THIRD_PARTY.md)

No replacement emulator, independent JavaScript DSP, or deployed simulator is supplied.
The original GPL license text is preserved; upstream retains its own license notices.
