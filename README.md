# Percent Clicker (Geode mod for Geometry Dash 2.2081)

Presses keybinds / key combos at chosen level percentages and can re-press them when you die.
Rules are edited in-game from a button in the **pause menu**.

## Project layout
```
mod.json              mod metadata + settings
CMakeLists.txt        build script (C++23, Geode v5)
logo.png              mod list logo (336x336)
about.md / changelog.md
resources/            pause-menu button icon
src/main.cpp          hooks: PlayLayer, PauseLayer, toggle keybind
src/Runner.*          rule engine: percent checks, press/hold/release, death re-press
src/SystemInput.*     real OS key presses (Windows SendInput / macOS CoreGraphics)
src/Rules.*           rule data, save/load, key-name parser
src/RulesPopup.*      the rules editor popup
.github/workflows/    GitHub Actions build
```

## Build it yourself
1. Install the Geode CLI and SDK: https://docs.geode-sdk.org/getting-started/
2. Make sure `GEODE_SDK` points to your Geode SDK folder.
3. In this folder run `geode build`.
4. Drop the resulting `.geode` file into `<Geometry Dash>/geode/mods/`.

## Build without installing anything
Push this folder to a GitHub repo. The workflow builds `.geode` files — download them from **Actions → Build Output**.

## Notes
- **Real keyboard** mode works on Windows and macOS. On macOS you must allow Geometry Dash under Privacy & Security > Accessibility. On other platforms the mod falls back to in-game input.
- Use it responsibly: automated inputs may not be allowed for leaderboards, challenges or verification runs.
