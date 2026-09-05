# Rep Counter

A C++20 console workout rep counter. It asks for a rep target and the wait time before each rep, displays the countdown, and plays the included MP3 notification sounds on Windows.

Current version: **v0.4-alpha**.

## Build and run

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\rep_counter.exe
```

Run the executable from the repository root so it can find the files in `sounds/`.
