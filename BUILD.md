# Build & Deploy

## Setup (one-time)

Copy `.env.local.example` to `.env.local` and fill in your UC3 host and PIN, then source it into your shell. All deploy commands below expect `UC3_HOST`, `UC3_USER`, and `UC3_PIN` in the environment:

```bash
set -a; source .env.local; set +a
```

## Cross-compile for UC Remote 3 (ARM64)

Run from the repo root. Requires Docker.

**Linux / macOS:**

```bash
docker run --rm --user=$(id -u):$(id -g) -v "$(pwd)":/sources \
    unfoldedcircle/r2-toolchain-qt-5.15.8-static@sha256:d4b1b81b4722586aa1bc9e6fc2d8ccf329872d71d6bbda40a40adb74060d31c6
```

**Windows + Git-Bash:** `$(pwd)` returns MSYS-style `/c/...` paths that Docker Desktop won't translate, and MSYS path conversion mangles `:/sources` if not disabled. Use `pwd -W` and `MSYS_NO_PATHCONV=1`:

```bash
MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd -W)":/sources \
    unfoldedcircle/r2-toolchain-qt-5.15.8-static@sha256:d4b1b81b4722586aa1bc9e6fc2d8ccf329872d71d6bbda40a40adb74060d31c6
```

`--user=$(id -u):$(id -g)` is harmless-but-ignored on Docker Desktop; kept on the Linux/macOS form for proper file ownership when running on a real Linux host.

**Release builds: pass the version.** The binary's version comes from `git describe`. On Windows / Docker Desktop, git rejects the bind-mounted repo (`detected dubious ownership`), so the version comes out empty, and the core then rejects the UI's `localization_languages` message at every start. Pass it as a qmake argument; `-e UC_BUILD_VERSION=…` does not reach qmake:

```bash
MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd -W)":/sources \
    unfoldedcircle/r2-toolchain-qt-5.15.8-static@sha256:d4b1b81b4722586aa1bc9e6fc2d8ccf329872d71d6bbda40a40adb74060d31c6 \
    remote-ui.pro "CONFIG+=static CONFIG+=release UC_BUILD_VERSION=v1.5.4"
```

CI builds don't need this: the container runs as the checkout's owner, and `git describe` finds the tag.

Output: `binaries/linux-arm64/release/remote-ui`

> **Toolchain pinning rationale.** The image is pinned by digest, not the `:latest` tag, so builds are reproducible from a tag check-out — `:latest` could be re-pushed by upstream and silently change the compiler / Qt minor / static-libs without a local indication. To rotate the pin (e.g. UC ships an updated toolchain you want to adopt), pull the new image then resolve the digest:
>
> ```bash
> docker pull unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest
> docker inspect unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest --format '{{index .RepoDigests 0}}'
> ```
>
> Replace the `@sha256:...` reference above and commit the bump as a `[chore] toolchain: bump to <new-digest>` commit so the move is auditable.

## Package & Deploy

```bash
cp binaries/linux-arm64/release/remote-ui deploy/bin/
cd deploy && tar -czf ../matrix-charging-screen.tar.gz release.json bin/ config/
curl --location "http://${UC3_HOST}/api/system/install/ui?void_warranty=yes" \
    --form "file=@../matrix-charging-screen.tar.gz" \
    -u "web-configurator:${UC3_PIN}" --max-time 120
```

UI restarts automatically after install.

## Revert to stock UI

```bash
curl -X PUT "http://${UC3_HOST}/api/system/install/ui?enable=false" -u "web-configurator:${UC3_PIN}"
```

## Desktop simulator

Upstream's static Linux x64 toolchain image builds the simulator without a Qt installation
([docs/static-compile.md](docs/static-compile.md)):

```bash
make linux-x64        # -> binaries/linux-x64/release/remote-ui
```

On Windows / Git-Bash, run the image directly (the version must be a qmake argument, as for the device build):

```bash
MSYS_NO_PATHCONV=1 docker run --rm -v "$(pwd -W)":/sources \
    unfoldedcircle/remote-ui-toolchain-qt-5.15.19-static-x64 \
    remote-ui.pro "CONFIG+=static CONFIG+=release UC_BUILD_VERSION=vX.Y.Z"
```

Start it with `UC_MODEL=DEV` against Unfolded Circle's [core simulator](https://github.com/unfoldedcircle/core-simulator).
DEV mode: the arrow keys are the d-pad, Return is OK, Escape is BACK, Home is HOME, F12 fakes docking and undocking.
qmake runs lupdate, which rewrites every tracked `resources/translations/*.ts`: keep `en_US.ts`, revert the others
(ADR 0006).

A dynamic desktop build with a local Qt 5.15 kit still works (`qmake && make`); on macOS the screensaver renders black
(OpenGL 2.1 limitation of Qt 5.15 there).

## Run tests

The suites need Qt 5.15 with Quick Controls 2, Multimedia, SVG and WebSockets (on Ubuntu: `qtquickcontrols2-5-dev
qtmultimedia5-dev libqt5svg5-dev libqt5websockets5-dev` and the matching `qml-module-*` packages). CI runs the same
commands (`.github/workflows/test.yml`):

```bash
(cd test/hardware && qmake hardware_test.pro && make -j4)            # keep awake, phantom wake, entity controller, power
(cd test/matrixrain && qmake matrixrain_test.pro && make -j4 && ./test_matrixrain)
(cd test/qml && qmake matrixrain_qml_test.pro && make -j4 && xvfb-run ./test_qml)
(cd test/integration && qmake matrixrain_integration_test.pro && make -j4 && xvfb-run ./test_integration)
mkdir -p test/build && cd test/build && cmake .. && cmake --build . -j4   # upstream's unit tests: run each test* binary
```

Before a qmake rebuild of a test, delete stale `*.moc`, `Makefile*` and `.qmake.stash` files in its folder: a stale
`.moc` hides new test slots.

## Enable Logdy (on-device log viewer)

```bash
curl --request PUT "http://${UC3_HOST}/api/system/logs/web" \
    --header 'Content-Type: application/json' \
    --user "web-configurator:${UC3_PIN}" \
    --data '{"enabled": true}'
```

View at `http://${UC3_HOST}/log/`. Disable when done (uses ~170 MB from integration memory pool):

```bash
curl --request PUT "http://${UC3_HOST}/api/system/logs/web" \
    --header 'Content-Type: application/json' \
    --user "web-configurator:${UC3_PIN}" \
    --data '{"enabled": false, "autostart": false}'
```
