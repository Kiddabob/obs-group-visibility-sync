# Group Visibility Sync for OBS Studio

Restore live show/hide updates for sources inside shared groups when using
Studio Mode and **Duplicate Scene** in OBS Studio 33.

**Early test release:** the Windows x64 build has been tested with OBS 33.0.1
in a portable installation using synthetic scenes. It has not yet been tested
with every real-world scene collection or plugin combination. Test it with
your own scenes before relying on it during a broadcast.

## Why use it?

OBS 33 makes a separate copy of each group when it duplicates a scene for
Studio Mode. This keeps group edits in Preview, but changes workflows that used
the same group as shared controls in several layouts. This plugin can mirror
the visibility of matching items inside those groups to Program. See the
[OBS change](https://github.com/obsproject/obs-studio/pull/11092) for context.

The plugin is **disabled by default**. Enable or disable it via **Tools → Sync
grouped visibility to Program**. The setting is remembered between launches.
When enabled, an eye-button change inside a matching group can immediately
affect what viewers see, even if Preview and Program are different scenes.

## Download and install (Windows x64, OBS 33)

1. Download [group-visibility-sync-0.1.0-windows-x64.zip](downloads/group-visibility-sync-0.1.0-windows-x64.zip?raw=1).
2. Close OBS. Extract the ZIP contents into `C:\ProgramData\obs-studio`.
   The DLL should be at
   `C:\ProgramData\obs-studio\plugins\group-visibility-sync\group-visibility-sync.dll`,
   with a `data` folder beside it. Administrator permission may be required.
3. Open OBS, then enable **Tools → Sync grouped visibility to Program**.

For portable OBS 33, extract the ZIP contents into the portable OBS root
instead. The DLL will be in `plugins\group-visibility-sync` under that root.
Portable OBS does not use the ProgramData plugin folder.

To remove the plugin, close OBS and delete the `group-visibility-sync` folder
from the appropriate `plugins` directory. Its saved on/off setting, if any,
is in OBS's `plugin_config/group-visibility-sync` directory.

## Scope and limitations

- It mirrors visibility changes **inside** matching groups only; it does not
  mirror the group's own eye button or other top-level scene items.
- It matches group names and the underlying child source. If the same source
  occurs more than once inside a matching group, it skips that group instead
  of guessing. Duplicating the underlying sources as well as the scene can
  prevent a match.
- Renaming a group after Program's copy was made prevents a match until the
  next transition. Nested groups and complex plugin combinations need more
  testing.
- It does not change positions, sizes, filters, or other source settings.

## Build from source

The current build targets Windows x64 and requires CMake, a C++17 compiler,
Qt 6.11 development files matching OBS, and OBS/libobs plus frontend API
development files. Point `CMAKE_PREFIX_PATH` at the Qt and OBS SDK prefixes,
then run:

```powershell
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH="C:/path/to/qt;C:/path/to/obs-sdk"
cmake --build build --config Release
```

If the OBS development files do not provide CMake package definitions, supply
`OBS_SOURCE_DIR`, `OBS_GENERATED_INCLUDE_DIR`, `OBS_LIBOBS_LIBRARY`, and
`OBS_FRONTEND_LIBRARY` to CMake instead. `BUILD_GROUP_SYNC_TESTS=ON` builds
the synthetic core test; running it requires `OBS_PROBE_DATA` and
`OBS_PROBE_GFX` to identify the OBS data folder and graphics module, with
OBS's runtime DLLs on `PATH`.

## License

GPL-2.0-or-later; see [LICENSE](LICENSE).
