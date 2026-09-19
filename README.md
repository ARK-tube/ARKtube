<div align="center">

<img src="https://raw.githubusercontent.com/ARK-tube/ARKtube/main/arktube_linux/resources/icons/appIcon.png" alt="ARKtube" width="120" height="120">

# ARKtube

**YouTube, as a native desktop app.**

Keep the UI. Keep the player. Change the shell.

</div>

---

## What we build

ARKtube wraps YouTube's own TV (Leanback) interface in a thin native shell, so it
behaves like an installed application instead of a browser tab. No redesign, no
replacement frontend, no bundled copy of the site. YouTube stays YouTube; we only
own the window around it.

```text
YouTube (youtube.com/tv)  +  GTK3 + WebKit2GTK  =  YouTube, installed.
```

## Projects

| Project | What it is | Where |
|---|---|---|
| **ARKtube for Linux** | Native GTK3 + WebKit2GTK client for `youtube.com/tv`: fullscreen persistence, gamepad and remote input, offline screen, boot splash, `.deb` builds from CI | [`main`](https://github.com/ARK-tube/ARKtube) |
| **Webtop** | A session layer that makes ARKtube selectable from the Ubuntu login screen, without starting the full GNOME Shell desktop | [`webtop`](https://github.com/ARK-tube/ARKtube/tree/webtop) |
| **Webtop, Sway edition** | The same idea on Sway: ARKtube is the only window the compositor ever shows, fullscreen and borderless | [`arktube-layer-shell`](https://github.com/ARK-tube/ARKtube/tree/arktube-layer-shell) |
| **ARKtube for Android** | A WebView shell around YouTube's mobile web UI | [`Android`](https://github.com/ARK-tube/ARKtube/tree/Android) |

## Design principles

- **Add the smallest layer that works.** If YouTube already solves a problem, we let it.
- **Own the window, not the page.** The native side handles window state, input mapping,
  and connectivity. The page is never re-implemented.
- **Stay small until the approach is proven.** Linux first; other platforms only after
  the Linux design holds up.

## Status

Early and in progress. The Linux app runs today; tray integration, Immersive Mode,
AppImage packaging, and non-Linux builds are not ported yet. The repository's
[roadmap](https://github.com/ARK-tube/ARKtube#status) tracks what is done and what is open.

## Try it

```bash
git clone https://github.com/ARK-tube/ARKtube.git
cd ARKtube/arktube_linux
cmake -B build -S . && cmake --build build
./build/arktube_linux
```

Requires CMake ≥ 3.16, a C11 compiler, `libgtk-3-dev`, and `libwebkit2gtk-4.1-dev`.
Full instructions are in the [repository README](https://github.com/ARK-tube/ARKtube#install).

---

<sub>ARKtube is an independent project. It is not affiliated with or endorsed by Google or YouTube.
YouTube is a trademark of Google LLC.</sub>


* **`arktube_linux/src/main.c`** — window/webview creation, user-agent
  and hardware-acceleration setup, fullscreen handling, the
  connectivity check, and the boot-splash / no-internet overlays
* **`arktube_linux/resources/js/user-script.js`** — injected at
  document-start into every frame of `youtube.com/tv`: the
  `navigator`/`screen` identity spoof, Home-key SPA navigation,
  gamepad/remote-to-keyboard remapping, and cursor auto-hide
* **`arktube_linux/packaging/arktube-linux.desktop`** — the installed
  `.desktop` launcher entry
* **`arktube_linux/CMakeLists.txt`** — build and install rules

### Initialization flow

1. `main()` creates the GTK window (maximized by default, or
   fullscreen if that was the state when the app last quit) and shows
   it immediately.
2. A background thread checks internet connectivity by attempting a
   raw TCP connection to `8.8.8.8:53`, independently of and before any
   WebView load.
3. While offline, a full-bleed "no internet" screen is shown in place
   of the browser; the check keeps retrying every few seconds.
4. Once connectivity is confirmed, the WebView is pointed at
   `https://www.youtube.com/tv#/` and a branded boot splash (logo +
   spinner) is shown on top of it.
5. `user-script.js` is injected at document-start, ahead of any of the
   page's own scripts, so the user-agent and `navigator`/`screen` spoof
   is in place before the page's bootstrap JS ever reads it.
6. The splash is dismissed the moment the WebView reports
   `WEBKIT_LOAD_FINISHED` (with a 20-second defensive ceiling in case
   that event never fires).

### Why the TV/Leanback interface

`youtube.com/tv` is designed for D-pad/remote navigation rather than a
mouse-and-keyboard desktop browser, which is what makes it a good fit
for a persistent, always-on-top-feeling desktop shell. Getting YouTube
to reliably serve that interface requires more than a User-Agent HTTP
header: `youtube.com/tv`'s own bootstrap JS also reads
`navigator.userAgent`, `navigator.platform`, `navigator.maxTouchPoints`
and `screen.*` directly, so `user-script.js` patches those on the
relevant prototypes before the page's own scripts run — see the
comments in that file for why each property is patched the way it is.

---

This project contains original code written for the desktop shell and
application layer.

YouTube is a trademark of Google LLC.

This project is independent and is not affiliated with or endorsed by
Google or YouTube.

See the repository license for the licensing terms of this project.
