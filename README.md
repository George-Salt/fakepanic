<div align="center">

# Fake Panic

**Your video. A fake kernel panic. Thousands of little dots.**

[![Release](https://img.shields.io/github/v/release/George-Salt/fakepanic?style=flat-square&color=2563eb)](https://github.com/George-Salt/fakepanic/releases/latest)
[![CI](https://github.com/George-Salt/fakepanic/actions/workflows/build.yml/badge.svg)](https://github.com/George-Salt/fakepanic/actions/workflows/build.yml)
[![Downloads](https://img.shields.io/github/downloads/George-Salt/fakepanic/total?style=flat-square&color=7c3aed)](https://github.com/George-Salt/fakepanic/releases)
[![MIT](https://img.shields.io/badge/code-MIT-22c55e?style=flat-square)](LICENSE)
![Linux](https://img.shields.io/badge/platform-Linux-f59e0b?style=flat-square&logo=linux&logoColor=white)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599c?style=flat-square&logo=cplusplus)

**English · [Русский](README.ru.md)**

[![Watch Fake Panic](docs/media/preview.gif)](https://github.com/George-Salt/fakepanic/raw/refs/heads/main/docs/media/preview.mp4)

[▶ Watch the 14-second presentation](https://github.com/George-Salt/fakepanic/raw/refs/heads/main/docs/media/preview.mp4) · [Download & install](https://github.com/George-Salt/fakepanic/releases/latest) · [Customize](#customize)

</div>

Fake Panic (`fakepanic`) plays local videos inside a fullscreen, blue Linux panic screen.
The image becomes a monochrome grid of Braille-style dots, with ASCII Tux and fictional
error messages around it. **It runs in user space; it does not crash your kernel. Press Esc to exit.**

The presentation captures the actual renderer in an isolated X11 display: Bad Apple in its native 4:3 frame, with the default blue and custom mint colors.
The supplied [Bad Apple video](examples/bad-apple.mp4) lets you reproduce the effect.
Video playback supports X11 and Wayland through SDL2. An experimental game mode captures
Lutris or Prism Launcher through headless Gamescope and PipeWire, independently of the host desktop.

## Install

### Download a release

Open [Releases](https://github.com/George-Salt/fakepanic/releases/latest).
Choose the **Linux x86_64 `.tar.gz`** for a local installation, or the **`.deb`** for Ubuntu/Debian.
The binaries are built on Ubuntu 22.04; the archive uses system libraries and is not an AppImage.
Other architectures can build from source.

Install runtime dependencies first:

```bash
# Ubuntu / Debian
sudo apt install libsdl2-2.0-0 libsdl2-ttf-2.0-0 ffmpeg fonts-dejavu-core python3

# Arch / CachyOS
sudo pacman -S --needed sdl2 sdl2_ttf ffmpeg ttf-dejavu python
```

For the archive, run from the directory containing the downloaded file:

```bash
sha256sum -c SHA256SUMS --ignore-missing  # download SHA256SUMS too
mkdir -p fakepanic-release
tar -xzf fakepanic-1.0.2-linux-x86_64.tar.gz -C fakepanic-release
cd fakepanic-release/fakepanic-1.0.2-linux-x86_64
./install.sh
~/.local/bin/fakepanic --version
```

The installer writes to `~/.local/bin` and `~/.local/share/fakepanic`; no root required.
Add `~/.local/bin` to your `PATH` if necessary; see [shells and desktops](#shells-and-desktops).
Set `env FAKEPANIC_PREFIX=/your/prefix ./install.sh` for a different location.
You can also run `./bin/fakepanic` directly from the extracted archive.

For a `.deb`, let APT resolve dependencies:

```bash
sudo apt install ./fakepanic-1.0.2-linux-x86_64.deb
fakepanic --version
```

### Build from source

```bash
# Ubuntu / Debian
sudo apt install build-essential cmake pkg-config libsdl2-dev libsdl2-ttf-dev ffmpeg fonts-dejavu-core python3

# Arch / CachyOS
sudo pacman -S --needed base-devel cmake pkgconf sdl2 sdl2_ttf ffmpeg ttf-dejavu python

git clone https://github.com/George-Salt/fakepanic.git
cd fakepanic
./install.sh
```

Manual build and tests:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
# Optional installation:
cmake --install build --prefix "$HOME/.local"
```

## Play a video

From the repository root:

```bash
fakepanic --video examples/bad-apple.mp4
fakepanic --video "/path/to/my video.mp4" --mute
fakepanic --video examples/bad-apple.mp4 --lang ru
```

The release packages also install the sample video:

```bash
# Local archive/source installation
fakepanic --video ~/.local/share/fakepanic/examples/bad-apple.mp4
# System .deb installation
fakepanic --video /usr/share/fakepanic/examples/bad-apple.mp4
```

`fakepanic FILE` is shorthand for `fakepanic --video FILE`.
Playback exits at the end of the video. Esc closes the effect immediately.
FFmpeg decodes the image; FFplay handles audio separately. Audio synchronization is approximate.
No network access is needed during playback.

## Customize

![Colors](https://img.shields.io/badge/colors-RRGGBB-2563eb?style=flat-square)
![FPS](https://img.shields.io/badge/FPS-1–120-7c3aed?style=flat-square)
![Languages](https://img.shields.io/badge/language-EN%20%2F%20RU-22c55e?style=flat-square)
![Window](https://img.shields.io/badge/window-fullscreen%20%2F%20windowed-f59e0b?style=flat-square)

| Option | Default | Effect |
| --- | --- | --- |
| `--lang en\|ru` | `en` (regardless of system locale) | Help and console messages; the panic screen always stays English |
| `--background RRGGBB` | `0000AA` | Background color; `#RRGGBB` also works when quoted |
| `--foreground RRGGBB` | `FFFFFF` | Text, border and dots |
| `--threshold 0…255` | `128` | Lower values produce more lit dots; ordered dithering preserves detail |
| `--fps 1…120` | `24` | Video conversion and playback frame limit |
| `--windowed` | Fullscreen | Open a fixed-size window |
| `--size WIDTHxHEIGHT` | `1280x720` | Window size, from `640x480` to `7680x4320` |
| `--reason TEXT` | Fictional fatal exception | Custom error line, including Russian text |
| `--font FILE` | DejaVu Sans Mono | Custom monospace TTF font |
| `--mute` | Sound on | Disable FFplay audio |
| `--duration SECONDS` | Whole video | Stop after 1–86400 seconds, including startup time |
| `--stats` | Off | Print frame count and average playback FPS to stderr |

```bash
# Mint on a dark background
fakepanic --video examples/bad-apple.mp4 --background 111827 --foreground A7F3D0

# Windowed, silent, with a custom error
fakepanic --video examples/bad-apple.mp4 --windowed --size 1280x720 \
  --mute --fps 30 --reason 'Kernel panic - not syncing: Bad Apple!!'

fakepanic --help
fakepanic --lang ru --help
```

The frame width follows the video’s display aspect ratio while its height stays fixed for the current window size. Bad Apple uses a 4:3 frame. Width is capped with a 20-pixel margin on either side of the window; very wide videos are fitted inside that limit without cropping. Sample aspect ratio and rotation metadata are respected. Dots are drawn directly, so the effect
does not depend on Braille font glyphs. The dot grid and text are rebuilt when the window or fullscreen resolution changes. The panic screen always uses English; `--lang` only affects help and console messages.
Console output defaults to English, including the launcher and installer. Use `--lang ru` for Russian messages, without duplicated translations. Third-party tools may print their own messages in English.

## Shells and desktops

The executable and installer work from **Bash, Zsh and Fish**, and from other shells
which can execute programs. Your terminal emulator is your choice. Scripts use their
own Bash shebang; you do not need to make Bash your login shell. Run `./install.sh`,
not `source install.sh` or `fish install.sh`.

| Shell | Add the default installation to PATH |
| --- | --- |
| Bash | `export PATH="$HOME/.local/bin:$PATH"` (persist in `~/.bashrc`) |
| Zsh | `export PATH="$HOME/.local/bin:$PATH"` (persist in `~/.zshrc`) |
| Fish | `fish_add_path ~/.local/bin` |
| Any shell | Use `~/.local/bin/fakepanic` directly |

Video playback uses SDL2's X11 or Wayland backend. The game capture backend uses
Gamescope's own headless output and PipeWire stream; it makes **no Hyprland IPC calls**
and does not require a host virtual monitor or a desktop screen-capture portal.
This removes the compositor dependency for KDE Plasma, GNOME, Sway, Xfce and other
Linux desktops. The backend is tested without `DISPLAY`, `WAYLAND_DISPLAY` or Hyprland
environment variables; individual desktop/driver combinations still need compatible
SDL2, Gamescope, Vulkan and PipeWire installations.

## Experimental game mode

Requires **Gamescope with the headless backend and PipeWire support** (tested: 3.16),
a running PipeWire service, GStreamer tools, the `pipewiresrc`, `videoscale`, `videoconvert`
and `fdsink` plugins, `jq`, Python 3, `libX11`, `libXtst`, and the selected launcher.
Video mode does not need these game-specific dependencies.

```bash
# Arch / CachyOS: extra dependencies
sudo pacman -S --needed gamescope pipewire gst-plugin-pipewire gstreamer gst-plugins-base jq libx11 libxtst

# Debian / Ubuntu: GStreamer and PipeWire dependencies
sudo apt install pipewire gstreamer1.0-tools gstreamer1.0-pipewire gstreamer1.0-plugins-base jq libx11-6 libxtst6
# Install a compatible Gamescope from your distribution, plus Lutris and/or Prism Launcher.

fakepanic --lutris 42 --stats
fakepanic --prism "My instance" --stats
fakepanic --prism "My instance" --capture-size 960x540 --fps 30
```

The Lutris ID is your local game ID; the Prism name is your existing instance name.
Fake Panic starts its own headless Gamescope process, locates the PipeWire video node
belonging to that process, and converts its frames directly to the renderer's tiny
grayscale grid. It never captures your desktop. `--capture-size WIDTHxHEIGHT` chooses
the virtual output size (320×240 to 3840×2160). The game itself uses a 1280×720 logical
screen for consistent input coordinates. Exit stops the capture and Gamescope processes
it started. Some launchers can keep the game running separately; close it normally if needed.

**Limitations:** game mode is experimental and has capture latency. Basic keyboard,
mouse buttons, wheel and absolute cursor positioning are forwarded through XTEST.
**Relative/raw mouse look is not supported**, so Minecraft camera control is limited.
Esc exits Fake Panic; F1 sends Escape to the game. Launchers which reuse an existing
process may not stay inside Gamescope. Nested display detection can fail in such cases.
Set `FAKEPANIC_GAME_DISPLAY=:N` only when you know the correct nested display.
The desktop Xwayland display is refused by default. The game must support X11/Xwayland;
the host desktop can use either X11 or Wayland. Gamescope requires a compatible Vulkan driver.

The automated suite covers renderer/launcher validation, Bash/Zsh/Fish launches and
stream shutdown. `tests/game-smoke.sh` additionally checks the real headless capture
path with synthetic content; commercial games and personal Prism instances are not CI fixtures.
See [validation scope](docs/VALIDATION.md).

## Troubleshooting

| Symptom | What to check |
| --- | --- |
| `fakepanic: command not found` | Add `~/.local/bin` to `PATH`, or use the full executable path |
| Renderer/library missing | Install the runtime dependencies; rebuild for your distribution if its ABI differs |
| Font error or missing Russian letters | Install DejaVu Sans Mono or use `--font /path/to/font.ttf` |
| Video will not decode | Run `ffmpeg -v error -i FILE -f null -`; read stderr from Fake Panic |
| No audio | Check FFplay and audio output; try `ffplay -nodisp -autoexit FILE` |
| Waiting for game capture | Check PipeWire, Gamescope and the GStreamer plugins; inspect `~/.local/state/fakepanic/` (or `$XDG_STATE_HOME/fakepanic/`) |
| Game input unavailable | Check the nested X11 display and XTEST; relative mouse look is unsupported |

Missing video frames or startup failure produce a nonzero exit status. A stream that
has not produced its first frame times out after 20 seconds. Ctrl+C / SIGTERM clean up children.

## How it works

```text
Video → FFmpeg → grayscale samples → ordered dots → SDL2
Game  → headless Gamescope → PipeWire → GStreamer grayscale → SDL2
Input ← nested Gamescope X11 session ← XTEST bridge ← SDL2 events
```

The renderer caches static text, batches dot drawing, and uses a bounded queue of at most
three video frames. Live capture keeps only the latest queued frame to reduce latency.
The UI stays responsive while decoding waits for data. Rendering happens when a new frame arrives.
This is an effect for local playback and experimentation, not a low-latency gaming frontend.

## Remove

For the default local installation:

```bash
rm -f ~/.local/bin/fakepanic
rm -rf ~/.local/share/fakepanic ~/.local/share/licenses/fakepanic
# Optional: remove game logs
rm -rf ~/.local/state/fakepanic
```

For the Debian package: `sudo apt remove fakepanic`. Custom prefixes use the same layout.

## Project

[Contributing / Участие](CONTRIBUTING.md) · [Security / Безопасность](SECURITY.md) · [Changelog](CHANGELOG.md)

Code: [MIT](LICENSE). The included Bad Apple video and derived recordings are third-party
media, excluded from MIT; see [media attribution](examples/README.md).
The simulated reboot/error messages are part of the visual effect. Use it on your own
screen or with the owner's knowledge.
