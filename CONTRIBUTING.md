# Contributing

Thanks for improving Rep Counter.

## Development workflow

1. Create a branch from `Main`.
2. Configure with tests enabled: `cmake -S . -B build -DBUILD_TESTING=ON`.
3. Build with warnings enabled: `cmake --build build --config Release`.
4. Run `ctest --test-dir build -C Release --output-on-failure`.
5. Keep workout behavior in `WorkoutSession` where it can be tested independently of Win32.
6. Update `README.md` and `RELEASE_NOTES.md` when behavior changes.

Pull requests should explain the user-visible change, include tests for state or timer logic, and remain warning-free under MSVC `/W4`.

By contributing, you agree that your contribution is licensed under GPL-3.0-only.
