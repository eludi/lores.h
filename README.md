# lores

A small, single-header terminal game framework for Linux and Windows.
Define `LORES_H_IMPLEMENTATION` in one C or C++ translation unit.

## Building the examples

Run `make` on Linux or in a Windows MinGW/MSYS2 shell. Linux audio builds
require ALSA development headers and libraries. Run `make mingw` on Linux
to cross-compile using the `x86_64-w64-mingw32-` GCC/G++ toolchain; override
`CROSS` to select another toolchain.

`make AUDIO=0` builds without audio headers or libraries. It also works with
`make mingw AUDIO=0`. Executables are written to `build/linux` or
`build/windows`, with a `-no-audio` suffix for audio-free builds.
`make clean` removes the selected configuration's executables.

Run `make test` on Linux for sanitizer regressions and PTY input tests.
These tests require GCC/G++, ALSA development files, Bash, and Python 3.

## Terminal lifecycle

```c
initscr();
atexit(shutdownscr);
echooff();
cursoroff();

/* Application loop. */

shutdownscr();
```

`initscr()` saves the original terminal state. `shutdownscr()` restores it
and is safe to call repeatedly. Registering it with `atexit()` also covers
normal early returns and `exit()` calls. Signal handling remains with the
application.

Linux restores the original input settings, resets colors to terminal
defaults, and restores cursor visibility on terminals supporting DEC
private-mode save/restore; other terminals receive a show-cursor fallback.
Windows restores console modes, text attributes, cursor state, and output
code page. A later `initscr()` starts a new session.

## Keyboard input

Use `echooff()` before a polling loop, `kbhit()` to check for input, and
`getkey()` to read a key. Linux input recognizes CSI and SS3 arrow, navigation,
and function-key sequences, including bytes arriving in separate chunks.
Modified sequences return the base key; modifier state is not exposed.

A standalone Escape waits up to 50 ms for the next byte. Override
`LORES_KEY_TIMEOUT_MS` before including `lores.h` to change this inter-byte
timeout. Incomplete or unsupported sequences return `KEY_UNKNOWN`. An Escape
followed by an ordinary character returns Escape and preserves the character
for the next read. Do not mix these functions with stdio reads from stdin.

## Optional audio

Define `LORES_NO_AUDIO` before including the implementation, or compile with
`-DLORES_NO_AUDIO`, to make `sound()` a silent no-op on either platform.
Linux builds then need neither ALSA, libm, nor pthread audio dependencies.

With audio enabled, an unavailable audio device disables subsequent sound
attempts and emits one warning on Linux. Recoverable playback errors are
retried before disabling sound.
