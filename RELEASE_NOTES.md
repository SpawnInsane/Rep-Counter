# Rep Counter v0.4-alpha

> First alpha release of the native C++ desktop app for Windows.

## Highlights

- Rebuilt Rep Counter from Python as a C++20 application.
- Added a native Windows desktop interface with a dark theme.
- Added fields for rep targets and rest time between reps.
- Added live countdown, rep progress, start, and complete-rep controls.
- Added support for resizing and maximizing the app window; controls adapt to the available space.
- Preserved the included MP3 cues for workout notifications.

## Build

The project now uses CMake and builds with the Visual Studio C++ toolchain:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\rep_counter.exe
```

## Known limitations

- This alpha release targets Windows.
- The app expects the `sounds/` directory to be available when it runs so it can play notification cues.
- UI behavior and styling may change before a stable release.

## Feedback

Please report bugs or UI feedback through GitHub Issues.
