# Rep Counter

A native Windows desktop workout rep counter with a dark-mode interface. Set a rep target and rest time, track each completed rep, and receive the included MP3 notification when it is time to begin the next one.

Current version: **v0.4-alpha**.

## Build and run

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\rep_counter.exe
```

Run the executable from the repository root so it can find the files in `sounds/`.
