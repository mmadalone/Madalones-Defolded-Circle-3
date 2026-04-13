# Madalone's Defolded Circle 3 — Custom Screensaver Firmware

Custom firmware fork of [`unfoldedcircle/remote-ui`](https://github.com/unfoldedcircle/remote-ui) for the **Unfolded Circle Remote 3**, replacing the stock analog-clock charging screen with a GPU-accelerated screensaver system.

Five themes, five screen-off animation styles, full DPAD/touch interaction, zero Home Assistant dependency, clean fall-back to stock UI on install failure.

![Matrix rain screensaver](docs/screenshots/matrix-color-01.jpeg)

---

## Features

**Themes (v1.2.2):**
- **Matrix rain** — 3-layer depth with per-cell residual glow, 108 tunable properties, 6 color modes (green / blue / red / amber / white / purple + rainbow / rainbow-gradient / neon), 4 charsets (ASCII / binary / digits / katakana), full glitch engine (flash, stutter, reverse, direction trails, chaos bursts), coprime-gravity auto-rotation with DPAD/touch direction control, message overlay with pulse/flash effects
- **Starfield** — warp-speed star tunnel with depth, trail length, color gradients (rainbow / neon)
- **Minimal** — large digital clock + locale-aware date, font + size + color configurable, optional 24h mode
- **Analog** — scaled-up analog clock with gradient hands + shutoff animation
- **TV Static** — CRT noise + scanlines + chroma + channel-flash bursts with GLSL shader

**Overlays (on every theme):**
- Clock (position: top / center / bottom, 24h toggle, color, font, docked-only mode)
- Battery level (icon + percent with 5-tier color coding, size slider, "Fully charged" translated)

**Screen-off animations:** fade / flash / vignette / wipe / sleepwave / genie / pixelate / dissolve / theme-native (per-theme shutdown animation, e.g. Analog's sweep-and-fall, TV Static's collapse)

**Interactive (Matrix theme):**
- DPAD 8-way direction with smooth gravity-lerp (no respawn on direction change)
- Touch zones: 4-corner direction, double-tap to close, long-press to slow
- Tap effects: burst, flash, scramble, spawn, square burst, ripple, wipe (all togglable, optional randomize)

---

## Install

See **[SCREENSAVER-README.md](SCREENSAVER-README.md)** for the full install flow including device setup, PIN, and revert procedure.

Quick version for anyone who's already set up:

```bash
# 1. Download the latest release tarball + checksum
curl -L -O https://github.com/mmadalone/Madalones-Defolded-Circle-3/releases/download/v1.2.2/remote-ui-v1.2.2-UCR2-static.tar.gz
curl -L -O https://github.com/mmadalone/Madalones-Defolded-Circle-3/releases/download/v1.2.2/remote-ui.hash

# 2. Verify integrity (SHA256 + GPG if signed — see docs/RELEASE_SIGNING.md)
./scripts/verify-release.sh remote-ui-v1.2.2-UCR2-static.tar.gz remote-ui.hash

# 3. Install on your device (replace with your UC3 host and web-configurator PIN)
curl --location "http://${UC3_HOST}/api/system/install/ui?void_warranty=yes" \
    --form "file=@remote-ui-v1.2.2-UCR2-static.tar.gz" \
    -u "web-configurator:${UC3_PIN}"
```

The device reboots the UI process (~10 s). If anything breaks, revert to stock:

```bash
curl -X PUT "http://${UC3_HOST}/api/system/install/ui?enable=false" \
    -u "web-configurator:${UC3_PIN}"
```

**⚠ This voids your warranty.** The custom install endpoint requires the `?void_warranty=yes` query string. Upstream UC will not support the device while custom firmware is active.

For safer rehearsals, the repo ships a mock UC3 HTTP endpoint at [`scripts/mock-uc3-api.py`](scripts/mock-uc3-api.py) so you can exercise [`scripts/deploy-canary.sh`](scripts/deploy-canary.sh) locally before pointing it at a real device.

---

## Build from source

Requirements:
- Docker (for ARM64 cross-compile via UC's static Qt toolchain)
- Qt 5.15.2+ on the host for local desktop builds (optional — only needed for the macOS / Linux simulator)

```bash
# Clone
git clone https://github.com/mmadalone/Madalones-Defolded-Circle-3.git
cd Madalones-Defolded-Circle-3
git submodule update --init --recursive

# Cross-compile for UC3 (ARM64)
docker run --rm --user=$(id -u):$(id -g) \
    -v "$(pwd)":/sources \
    unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest

# Output: binaries/linux-arm64/release/remote-ui
```

Desktop-side build (requires Qt 5.15 Creator + UC's Core Simulator):

```bash
qmake && make -j$(sysctl -n hw.ncpu)
UC_MODEL=DEV ./binaries/osx-*/release/Remote\ UI
```

See [`BUILD.md`](BUILD.md) for the full dev flow including `.env.local` setup for deploy credentials, the Docker VNC screensaver preview (`test/matrixrain_preview/`), and on-device `logdy` log viewer.

---

## Architecture

| Layer | Where | What |
|---|---|---|
| **C++ renderers** (`src/ui/`) | `matrixrain.cpp` (QQuickItem + QSGGeometryNode + texture atlas), `rainsimulation.cpp` (simulation logic), `glyphatlas.cpp` (GPU glyph atlas) | GPU pipeline, simulation tick, per-frame vertex emission on the Qt scene-graph render thread |
| **Config bridge** (`src/ui/`) | `screensaverconfig.cpp` + `screensaverconfig_macros.h` | 108 Q_PROPERTY-backed settings via `SCRN_BOOL/INT/STRING` macros, QSettings-backed, survives popup destruction on undock |
| **QML themes** (`src/qml/components/themes/`) | `MatrixTheme.qml`, `StarfieldTheme.qml`, `MinimalTheme.qml`, `AnalogTheme.qml`, `TvStaticTheme.qml` | Thin QML wrappers implementing a common theme-native screen-off protocol (see `BaseTheme.qml` for the contract) |
| **Settings UI** (`src/qml/settings/settings/chargingscreen/`) | 13 sub-pages | DPAD-navigable theme picker + per-theme config sliders/toggles |
| **CI** (`.github/workflows/`) | `build.yml` (cross-compile + release + GPG signing), `tidy.yml` (clang-tidy baseline), `test.yml` (QtTest suite + 5 theme tests), `code_guidelines.yml` (cpplint) | Every push is built, lint-checked, and release-signed (when tagged) |

See [`STYLE_GUIDE.md`](STYLE_GUIDE.md) for the full architectural conventions and [`docs/CUSTOM_FILES.md`](docs/CUSTOM_FILES.md) for the custom-vs-upstream file manifest.

---

## Release verification

All tagged releases (`v1.2.2+`) ship with a SHA256 hash file; releases built after the maintainer configures the GPG signing secret will also ship with detached `.asc` signatures.

```bash
# Import the project release key (one-time)
gpg --import docs/release-pubkey.asc

# Verify a download
./scripts/verify-release.sh \
    remote-ui-v1.2.2-UCR2-static.tar.gz \
    remote-ui.hash \
    remote-ui-v1.2.2-UCR2-static.tar.gz.asc
```

Key details + rotation procedure: [`docs/RELEASE_SIGNING.md`](docs/RELEASE_SIGNING.md).

---

## Upstream relationship

This is a **fork** of [`unfoldedcircle/remote-ui`](https://github.com/unfoldedcircle/remote-ui), tracked via the `upstream` git remote. All custom work lives on `main`; upstream commits are merged manually per the playbook in [`docs/UPSTREAM_MERGE.md`](docs/UPSTREAM_MERGE.md). Current fork base: **`v0.71.1`**.

If you're looking for the stock UC Remote 3 firmware source, go to the upstream repo. This fork is specifically for running the custom screensaver on a UC3.

---

## Contributing

[`CONTRIBUTING.md`](CONTRIBUTING.md) covers the workflow, code style (`cpplint.sh` + `.clang-format`), the local pre-commit hook (`.githooks/pre-commit`), and the PR process.

Custom modifications should follow the mod pattern documented in [`STYLE_GUIDE.md`](STYLE_GUIDE.md) §1 (anatomy: C++ renderer + `SCRN_*` config bridge + QML wrapper + settings sub-page + qrc registration).

---

## Version history

See [`SCREENSAVER-README.md`](SCREENSAVER-README.md) for the full release log and [`CHANGELOG.md`](CHANGELOG.md) for upstream UC changes.

**v1.2.2** (2026-04-13) — 5 screensaver bug fixes, thermal sim-pause, DPAD respawn fix, strict warning flags, clang-tidy CI, i18n baseline, macro cleanup, release signing + canary deploy scripts, upstream merge rehearsal, SBOM, a11y audit checklist.

**v1.2.1** — drop `displayOff` gate from matrix running binding (fixes wake-black race).
**v1.2.0** — runtime slider fix, tap master toggle.
**Earlier** — theme selector, GradientText, Analog theme, TV Static theme, screen-off animation system.

---

## License

**GPL-3.0-or-later** — see [`LICENSE`](LICENSE). Custom firmware modifications are under the same license as the upstream project. Third-party dependencies listed in [`sbom.cdx.json`](sbom.cdx.json) and [`licenses/remote-ui_licenses.md`](licenses/remote-ui_licenses.md).

---

## Not affiliated with Unfolded Circle

This is an independent community mod. Unfolded Circle does not endorse or support installs of this firmware. For the official product, see [unfoldedcircle.com](https://unfoldedcircle.com).
