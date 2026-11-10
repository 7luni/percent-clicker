# Percent Clicker (Geode mod for Geometry Dash 2.2081)

Presses keybinds / key combos at chosen level percentages and can re-press them when you die.
Rules are edited in-game from a button in the **pause menu**.

## Project layout
```
mod.json              mod metadata + settings
CMakeLists.txt        build script (C++23, Geode v5)
logo.png              mod list logo (336x336)
about.md / changelog.md
resources/            pause-menu button icon (SD/HD/UHD)
src/main.cpp          hooks: PlayLayer, PauseLayer, toggle keybind
src/Runner.*          rule engine: percent checks, press/hold/release, death re-press
src/Rules.*           rule data, save/load, key-name parser
src/RulesPopup.*      the rules editor popup
.github/workflows/    GitHub Actions build (Windows, macOS, Android)
```

## Build it yourself
1. Install the Geode CLI and SDK: https://docs.geode-sdk.org/getting-started/
2. Make sure `GEODE_SDK` points to your Geode SDK folder.
3. In this folder:
   ```
   geode build
   ```
   (or `cmake -B build && cmake --build build --config RelWithDebInfo`)
4. The result is `yourname.percent-clicker.geode`. Drop it into `<Geometry Dash>/geode/mods/` (or use the Geode installer's mod folder) and start the game.

## Build without installing anything
Push this folder to a GitHub repo. The included workflow builds Windows, macOS and Android `.geode` files — download them from the **Actions → Build Output** artifact.

## Before you publish
- Change `"id"` and `"developer"` in `mod.json` (the id format is `yourname.percent-clicker`) and the name in `LICENSE`.

## Key syntax
Join keys with `+`: `ctrl+alt+k`, `shift+f5`, `space`. Modifiers: `ctrl shift alt cmd`. Also `a-z 0-9 f1-f12 num0-num9 up down left right enter tab esc backspace delete insert home end pageup pagedown capslock` and `, . / ; ' [ ] \ - = ``.

## Notes
- If the compiler complains about `dispatchKeyboardMSG`, the code already auto-selects the 3 or 4 argument version; if a future Geode changes it again, edit `sendKey` in `src/Runner.cpp`.
- Injected keys go through the normal key system, so avoid using the same combo as the "Toggle Keybind" setting (F8 by default).
- Use it responsibly: automated inputs may not be allowed for leaderboards, challenges or verification runs.
