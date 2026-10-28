# Cliphist

A small Qt Quick clipboard-history manager for Ubuntu (X11).

Copy text or images anywhere — they're remembered. Press **Ctrl+Shift+V** and a
translucent, animated list of your 10 most recent items appears next to the
cursor. Click one (or use ↑/↓ + Enter) and it's pasted into the app you were in.
[Screencast from 2026-10-28 16-48-49.webm](https://github.com/user-attachments/assets/52eca614-81b4-45e5-b762-bbe87af55191)

## Requirements

- Qt 6.10+ (targets `/home/ayoub/Qt/6.10.2/gcc_64`)
- X11 session
- g++, CMake, Ninja (the Qt install bundles CMake + Ninja under `~/Qt/Tools/`)

### Build dependencies

The app needs X11 + XTest + OpenGL **dev headers**. Two options:

**Option A — install system packages (recommended, one-time):**

```bash
sudo apt install -y libx11-dev libxtst-dev libxi-dev libxext-dev \
                    libgl-dev libglx-dev libegl-dev libgles-dev
```

**Option B — no sudo (already done on this machine):**

The headers were downloaded and extracted to `~/.local/x11dev` (plus linker
symlinks to the system runtime libs). The build automatically uses that prefix
when it exists. If you later install the system packages, you can delete
`~/.local/x11dev` and rebuild to use the system headers instead.

## Build

```bash
export PATH=$HOME/Qt/Tools/CMake/bin:$HOME/Qt/Tools/Ninja:$PATH
cd ~/cliphist
cmake -B build -G Ninja -DCMAKE_PREFIX_PATH=$HOME/Qt/6.10.2/gcc_64
cmake --build build
```

## Run

```bash
~/cliphist/build/cliphist
```

Runs in the background. History persists across restarts in
`~/.local/share/cliphist/` (`history.json` + per-entry `mime_*.bin` and
`thumb_*.png`).

## Controls

| Input | Action |
|---|---|
| `Ctrl+Shift+V` (anywhere) | Toggle the clipboard popup |
| `↑` / `↓` | Move selection |
| `Enter` | Paste selected item into the previous app |
| `Esc` | Dismiss |
| Click an item | Paste it |

## Notes

- **System tray:** the tray icon (Quit / Pause watching / Clear history)
  requires the GNOME "AppIndicator and KStatusNotifierItem Support" extension
  to be visible. The hotkey works regardless; quit with `pkill cliphist` if
  there is no tray.
- **X11 only.** On Wayland, global hotkeys and synthetic paste need a different
  (portal/extension) approach.
- Images are stored as PNG thumbnails; the original clipboard bytes are
  replayed faithfully on paste (text, HTML, URLs, and images all work).
