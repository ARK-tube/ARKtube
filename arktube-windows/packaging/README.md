# arktube-windows packaging

## Spinner artwork

`resources/icons/arktube-spinner.svg` is copied straight from
[`../../arktube_linux/resources/icons/arktube-spinner.svg`](../../arktube_linux/resources/icons/arktube-spinner.svg)
and is **not** shipped as a loadable resource on this platform: GDI+
(used by `src/ui/overlay.cpp` to draw the splash/no-internet screens,
since there's no GTK-style widget tree here to compose with
`gdk_pixbuf_new_from_file()` the way
[`arktube_linux/src/ui/spinner.c`](../../arktube_linux/src/ui/spinner.c)
does) has no built-in SVG decoder the way gdk-pixbuf's librsvg-backed
loader does on Linux.

Instead, `resources/icons/arktube-spinner.png` -- a one-time,
pre-rasterized PNG of the same artwork at
`ARKTUBE_SPLASH_SPINNER_SIZE` (90x90, see `../src/config.h`) -- needs
to be generated once and checked in, rather than produced by every
build the way `../CMakeLists.txt`'s `POST_BUILD` step handles the rest
of `resources/`. Any SVG rasterizer that can target an exact pixel size
works, e.g.:

```sh
# Inkscape
inkscape resources/icons/arktube-spinner.svg \
    --export-type=png --export-width=90 --export-height=90 \
    --export-filename=resources/icons/arktube-spinner.png

# or resvg
resvg -w 90 -h 90 resources/icons/arktube-spinner.svg resources/icons/arktube-spinner.png
```

If `resources/icons/arktube-spinner.png` is missing, the app still
runs: `ArktubeOverlay::Create()` in `../src/ui/overlay.cpp` degrades
the same way `arktube_create_splash_overlay()` does on Linux when its
own spinner can't be loaded -- the splash screen still shows, just
without the spinner.

## Installer

Not yet built. The CI workflow that will package `arktube_windows.exe`
+ `resources\` + `WebView2Loader.dll` (see `../CMakeLists.txt`'s
`install()` rules) into an actual installer or `.msix` is tracked as
follow-up work, the same status the Linux build's AppImage packaging is
in (see [`../../arktube_linux/README.md`](../../arktube_linux/README.md)'s
"Not yet ported" list). `.github/workflows/arktube-linux.yml` is the
existing CI reference to model an `arktube-windows.yml` equivalent on.
