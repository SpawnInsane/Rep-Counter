# Rep Counter v0.5.0-alpha

## Highlights

- Fixed countdown rounding so rests display their full duration and never finish early.
- Changed rest input from minutes to seconds and added 30/60/90-second presets.
- Made the first rep immediate by default; an optional checkbox restores an initial rest.
- Added pause/resume, skip-rest, cancel, and new-workout controls.
- Added selectable synthesized alerts, mute, volume, and a workout-complete cue.
- Added keyboard navigation, shortcuts, visible focus indicators, and Per-Monitor V2 DPI support.
- Added an original application icon and Windows version metadata.
- Removed runtime MP3 files and their fragile working-directory dependency.
- Extracted the workout state machine and added automated tests.
- Added Windows build/test/package CI and tightened security workflow permissions.
- Added portable ZIP packaging through CPack.

## Build

```powershell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Known limitations

- Windows is currently the only supported platform.
- Settings and workout history are not yet persisted between launches.
- The application is not currently code-signed, so downloaded builds may trigger a Windows reputation warning.
