# MONOLITH Toolbox

Native Windows toolbox prototype built with C++20, Win32, Direct2D and DirectWrite.

**Status:** prototype. Builds are currently unsigned. SignPath Foundation approval
has not been obtained; this repository does not claim to provide signed releases.

## Features

- Dark interface with application accent colors, SVG control artwork, hover,
  pressed and disabled states, and a custom draggable title bar.
- Original MONOLITH PNG logo with dynamic monitor-DPI rendering.
- Application icons read locally from installed programs and Windows resources.
- Project templates with type selection and drag ordering.
- Local workspace creation with a project name, directory and starter files.
- A sample application catalogue with category filters and scrolling.

## Current limitations

The Installed and Available views use sample names and versions. Launch and Install
are previews; search, sorting, package downloads and updates are not implemented.
Icon lookup does not imply that application discovery or launching is implemented.
Every project template currently generates the same basic CMake/C++ starter.
The `.mlw` workspace file is a textual placeholder, and tile order is not persisted.
Glass shading is simulated; live desktop blur is not implemented.

## Build on Windows

Requirements: CMake 3.21+, a C++20 compiler, and Windows SDK headers/libraries.
Use MSVC/Visual Studio Build Tools or MinGW-w64. WSL/Linux builds are not supported.

### CLion / MinGW

Open this directory in CLion, choose its Windows MinGW toolchain, and run the
`MONOLITH_Toolbox` target. Assets and required MinGW runtime DLLs are copied next
to the executable. Windows Smart App Control can block locally built unsigned files.

### MSVC release

From a Visual Studio developer shell:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

The executable is `build/Release/MONOLITH_Toolbox.exe`; keep its `assets/` directory
beside it. The MSVC build links its C++ runtime statically. The Windows workflow
builds from repository sources on a GitHub-hosted runner and uploads an unsigned
portable package as a workflow artifact. This is not a published release or a signature.

## Artwork

The original logo is `assets/monolith.png`. UI controls have editable SVG sources
in `assets/ui/`, with transparent PNG renditions for the native WIC renderer.
To regenerate UI renditions, install Sharp in a development environment and run
`node scripts/generate_ui_icons.cjs`. Node.js and Sharp are not needed to build or
run the desktop application. App logos are loaded from the user's installed apps;
third-party application icons are not bundled in this repository.

## Portable use and removal

Extract a release package to a directory you own and run its executable. The
prototype installs no services, startup entries or system-wide registry settings.
Close the application and remove its extracted directory to uninstall it. Workspaces
created by the user remain in the location selected by the user.

## Code signing policy

Signing is **planned, not active**. See [Code signing policy](docs/CODE_SIGNING.md)
and [SignPath onboarding](docs/SIGNPATH.md). Approval from SignPath Foundation,
service configuration and manual release approval are still required.

## Privacy and licensing

See [Privacy policy](PRIVACY.md), [MIT license](LICENSE), and
[third-party notices](THIRD_PARTY_NOTICES.md).
