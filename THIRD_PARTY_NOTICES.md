# Third-party notices

MONOLITH Toolbox's project-owned source and artwork are distributed under the MIT
license in LICENSE. Third-party names and trademarks remain with their owners.

## Runtime resources

Windows APIs and Windows Shell resources are supplied by Windows, not redistributed
as MONOLITH artwork. Application logos are extracted locally from installed software;
this repository does not include copies of VS Code, Blender, Git, Unreal, Godot,
Python or CUDA application binaries or logos.

## Build tools

The default portable GitHub package is built with MSVC and its static runtime.
Redistribution is subject to the Visual Studio/Windows SDK toolchain terms. These
toolchains are not included in the source repository.

Local MinGW builds may copy libgcc, libstdc++ and libwinpthread DLLs into the build
directory. Those generated files are excluded from source publication. Anyone who
distributes a MinGW binary package must include the applicable upstream copyright
and license notices, including the GCC Runtime Library Exception where applicable.

Sharp is an optional SVG-rendition development tool and is not linked into the
application. Sharp and its dependencies retain their own upstream licenses.
The repository includes the resulting project-owned UI artwork, not those tools.
