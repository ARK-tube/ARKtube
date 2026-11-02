# arktube-windows

A native Windows port of ARKtube: **one Win32 process, one WebView2
controller, no Electron, no bundled Chromium.**

This mirrors [`../arktube_linux`](../arktube_linux) module-for-module
(`src/platform`, `src/net`, `src/ui`, plus a top-level `main.cpp`
that's just the wiring between them) rather than being a separate
design -- see that directory's `README.md` for the architecture
rationale this port carries over unchanged, and
[`../docs/README.md`](../docs/README.md) for how the two platform
directories relate.

## Why WebView2

WebView2 wraps the Microsoft Edge/Chromium runtime already installed
(or auto-installed via Windows Update) on virtually every Windows 10/11
machine -- the direct Windows counterpart of WebKitGTK on Linux, which
`arktube_linux`'s own README describes as "the same engine GNOME Web /
Epiphany uses". Both let this app draw directly into a window it owns,
set a full-replacement user agent, and inject a document-start script
ahead of the page's own JS, with no reparented second process and no
bundled browser engine shipped in the repo.

## Status

🚧 **Early port, in progress**, started from the same slice
`arktube_linux` started from and carrying the same scope forward:

* [x] Launches youtube.com/tv in a native Win32 window via WebView2
* [x] TV/Leanback interface: full user-agent replacement
      (`src/ui/webview.cpp`'s `put_UserAgent()`) *and* the matching
      `navigator`/`screen` spoof, via the same
      `resources/js/user-script.js` shipped on Linux (see that file's
      header comment)
* [x] F11 fullscreen toggle, Escape to exit fullscreen
      (`src/ui/input.cpp`), intercepted ahead of the page via
      WebView2's `AcceleratorKeyPressed` event
      (`src/ui/webview.cpp`'s `SetAcceleratorKeyHandler()`)
* [x] Fullscreen state persisted across restarts, under
      `HKEY_CURRENT_USER\Software\ARKtube` (`src/platform/settings.cpp`)
* [x] Background connectivity check (raw non-blocking Winsock connect
      to `8.8.8.8:53`, retried on a timer) gating the WebView2
      navigation, with the same full-bleed "no internet" screen
      (`splash screen/no_internet.png`) shown while offline
* [x] Branded boot splash (`splash screen/boot-logo.png` + the same
      spinner artwork, pre-rasterized to PNG -- see
      `packaging/README.md`), dismissed on WebView2's
      `NavigationCompleted` (with the same 20s defensive fallback)

Not yet ported (same list as `arktube_linux`'s own, plus one
Windows-specific item):

* [ ] Tray icon / tray menu
* [ ] Immersive Mode (fullscreen lockdown + devtools guard)
* [ ] Gamepad/remote input beyond what `user-script.js`'s shared
      Gamepad-API polling already covers -- no evdev-style raw-input
      remap has been written for the IR/Bluetooth-remote-as-keyboard
      case `arktube_linux/src/ui/input.c`'s `arktube_remap_remote_keyval()`
      handles on Linux
* [ ] Installer / `.msix` packaging (see `packaging/README.md`) and CI
      (no `.github/workflows/arktube-windows.yml` yet)

## Building

**No CI yet** -- unlike `arktube_linux`, which has
`.github/workflows/arktube-linux.yml` building and uploading a `.deb`
on every push, this directory doesn't have a Windows equivalent set up
yet (tracked above). Everything below is for a local dev build, which
requires an actual Windows machine -- there's no cross-compiling this
from the Linux container this repo was cloned into.

### Requirements

* Windows 10 or 11
* CMake >= 3.21, a C++17 compiler (MSVC, via Visual Studio 2022's "Desktop
  development with C++" workload, is the target toolchain)
* The [WebView2 Runtime](https://developer.microsoft.com/microsoft-edge/webview2/)
  -- pre-installed on current Windows 10/11, so normally nothing to do
  here; the WebView2 *SDK* (headers/import lib) is fetched automatically
  at configure time, see `CMakeLists.txt`
* Internet access at configure time, for that SDK fetch (NuGet)

### Configure and build

```bat
cd arktube-windows
cmake -B build -S .
cmake --build build --config RelWithDebInfo
```

`cmake --build` copies `resources\` and `WebView2Loader.dll` next to
the built binary automatically, so it's runnable straight from the
build directory:

```bat
build\RelWithDebInfo\arktube_windows.exe
```

### Install

```bat
cmake --install build --prefix out
```

Installs the binary, `resources\`, and `WebView2Loader.dll` into a flat
`out\` directory -- see `src/platform/resource_paths.h` for why this
port only needs the one "next to the executable" resource lookup,
unlike the three-way build-tree/installed-prefix/cwd fallback
`arktube_linux/src/platform/resource_paths.c` needs on Linux.

## Layout

```
arktube-windows/
├── CMakeLists.txt
├── src/
│   ├── main.cpp                   # Win32 window + orchestration
│   ├── config.h                   # shared compile-time constants
│   ├── platform/
│   │   ├── resource_paths.*       # locate shipped files at runtime
│   │   └── settings.*             # persisted fullscreen pref (registry)
│   ├── net/
│   │   └── connectivity.*         # background Winsock connectivity poll
│   └── ui/
│       ├── webview.*              # WebView2 setup, script injection, UA
│       ├── overlay.*              # GDI+ splash / no-internet screens
│       └── input.*                # F11/Escape fullscreen toggle
├── resources/
│   ├── js/
│   │   └── user-script.js         # byte-identical to arktube_linux's copy
│   ├── icons/
│   │   ├── appIcon.png            # window/taskbar icon (shared art)
│   │   └── arktube-spinner.svg    # shared art; see packaging/README.md
│   └── splash screen/
│       ├── boot-logo.png          # shared art
│       └── no_internet.png        # shared art
└── packaging/
    └── README.md
```
