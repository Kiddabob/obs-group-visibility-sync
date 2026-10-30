# Group Visibility Sync

An independent, third-party OBS Studio plugin that restores live show/hide
updates for sources inside shared groups or folders in OBS Studio 33.

If a source's eye button changes **Preview** but not the live **Program**
output when using Studio Mode and **Duplicate Scene**, Group Visibility Sync
can mirror that visibility change to the matching shared group in Program.
It works when Preview and Program show the same scene or different scenes.

**Early test release:** the Windows x64 build has been tested with OBS 33.0.1
in a portable installation using synthetic scenes and reported working in the
project owner's OBS setup. It has not yet been tested with every scene
collection or plugin combination. Test it with your own scenes before relying
on it during a broadcast.

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

## Where to find the toggle

1. In OBS Studio, click **Tools** in the menu bar at the top of the window.
2. Click **Sync grouped visibility to Program**. A check mark beside the item
   means it is enabled. Click it again to turn it off.

The option starts off. When it is on, showing or hiding an item inside a
matching group can change the live Program output immediately.

The switch beside the plugin under **Tools → Plug-in Manager → Installed**
enables the plugin itself; the separate **Tools** menu item above turns its
visibility syncing on or off.

## Download and install (Windows x64, OBS 33)

1. Close OBS and download the [Windows installer](https://github.com/Kiddabob/obs-group-visibility-sync/releases/download/v0.1.0-beta.1/GroupVisibilitySync-0.1.0-beta.1-Setup.exe).
2. Run the installer. It places the plugin in OBS 33's standard plugin folder
   and adds an uninstall entry to Windows Installed apps.
3. Open OBS, then enable **Tools → Sync grouped visibility to Program**.

This early test installer is not code-signed, so Windows may show a warning
before it runs. The release includes SHA-256 checksums for the downloads.

Alternatively, download the [manual ZIP](downloads/group-visibility-sync-0.1.0-windows-x64.zip?raw=1),
close OBS and extract its contents into `C:\ProgramData\obs-studio`. The DLL
should be at `C:\ProgramData\obs-studio\plugins\group-visibility-sync\group-visibility-sync.dll`,
with a `data` folder beside it. Administrator permission may be required.

For portable OBS 33, extract the ZIP contents into the portable OBS root
instead. The DLL will be in `plugins\group-visibility-sync` under that root.
Portable OBS does not use the ProgramData plugin folder.

To uninstall the installer version, close OBS, open **Windows Settings → Apps →
Installed apps**, find **Group Visibility Sync for OBS Studio**, and choose
**Uninstall**. For a manual ZIP install, close OBS and delete the
`group-visibility-sync` folder from the appropriate `plugins` directory. The
uninstaller preserves the saved on/off setting in OBS's
`plugin_config/group-visibility-sync` directory.

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

## Common questions

### Why do sources in a group update Preview but not Program?

In OBS Studio 33, Studio Mode's Duplicate Scene setting gives Program a
separate copy of each group. This is useful for preparing changes privately,
but affects setups that use groups as shared folders across multiple scenes.
Enable this plugin to send matching grouped source-visibility changes to the
live Program output as well.

### Why is the option missing from the Tools menu?

Make sure **Group Visibility Sync** is enabled under **Tools → Plug-in Manager
→ Installed**. The menu item appears after OBS has loaded the plugin; an
already-running OBS 33 session does not load newly installed plugins
automatically. Check that the plugin DLL is directly inside the
`group-visibility-sync` folder in the appropriate `plugins` directory.
Third-party plugins do not load in OBS Safe Mode. The current download is for
Windows x64 and was tested with OBS 33.0.1.

The Plug-in Manager's **Browse** and **Updates** sections are disabled in OBS
33.0.1. Download and update this plugin from its GitHub release page instead.

### Does it sync the group's own eye button or its position?

No. It mirrors show/hide changes to matching sources **inside** a group. It
does not sync the group's own visibility, source positions, sizes, filters or
other settings.

### Does it work on macOS or Linux?

The available installer and ZIP are for Windows x64 only. macOS and Linux
builds have not been provided or tested.

For bug reports and compatibility feedback, please use the
[GitHub issue tracker](https://github.com/Kiddabob/obs-group-visibility-sync/issues).

## Build from source

The current build targets Windows x64 and requires CMake, a C++17 compiler,
Qt 6.11 development files matching OBS, and OBS/libobs plus frontend API
development files. Point `CMAKE_PREFIX_PATH` at the Qt and OBS SDK prefixes,
then run:

```powershell
cmake -S . -B local-build -A x64 -DCMAKE_PREFIX_PATH="C:/path/to/qt;C:/path/to/obs-sdk"
cmake --build local-build --config Release
```

If the OBS development files do not provide CMake package definitions, supply
`OBS_SOURCE_DIR`, `OBS_GENERATED_INCLUDE_DIR`, `OBS_LIBOBS_LIBRARY`, and
`OBS_FRONTEND_LIBRARY` to CMake instead. `BUILD_GROUP_SYNC_TESTS=ON` builds
the synthetic core test; running it requires `OBS_PROBE_DATA` and
`OBS_PROBE_GFX` to identify the OBS data folder and graphics module, with
OBS's runtime DLLs on `PATH`.

To build the Windows installer from the compiled DLL, install Inno Setup 6
and run `ISCC.exe installer/group-visibility-sync.iss`. The compiler writes
the installer into `downloads`.

## License

GPL-2.0-or-later; see [LICENSE](LICENSE).

This is an independent project and is not affiliated with the OBS Project.
The plugin was developed with AI assistance from OpenAI Codex; the owner has
reported it working in their OBS setup, and the plugin and installer were also
tested in isolated environments.
