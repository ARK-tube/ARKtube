#ifndef ARKTUBE_PLATFORM_RESOURCE_PATHS_H
#define ARKTUBE_PLATFORM_RESOURCE_PATHS_H

#include <optional>
#include <string>

/* Locate a file this app ships (icons, the injected JS, the splash
   artwork) at runtime, relative to the running executable -- the
   Windows counterpart of
   ../../../arktube_linux/src/platform/resource_paths.h.

   There is deliberately only one lookup location here, unlike the
   three-way fallback (build tree / installed prefix / CWD) the Linux
   port needs: a Windows install (MSIX, or a plain xcopy/installer
   layout, see ../../packaging/) always ships resources\ as a sibling
   of the .exe, and the CMake post-build step in ../../CMakeLists.txt
   copies resources\ next to the binary in the build tree the same way,
   so "next to the executable" already covers both cases -- there's no
   separate FHS-style share\ prefix to fall back to on this platform.

   Returns std::nullopt if the file doesn't exist there. */
std::optional<std::wstring> ArktubeFindResource(const std::wstring &relative);

#endif // ARKTUBE_PLATFORM_RESOURCE_PATHS_H
