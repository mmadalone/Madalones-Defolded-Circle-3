# Build & Deploy

## Setup (one-time)

Copy `.env.local.example` to `.env.local` and fill in your UC3 host and PIN, then source it into your shell. All deploy commands below expect `UC3_HOST`, `UC3_USER`, and `UC3_PIN` in the environment:

```bash
set -a; source .env.local; set +a
```

## Cross-compile for UC Remote 3 (ARM64)

```bash
cd "/Users/madalone/_Claude Projects/UC-Remote-UI"
docker run --rm --user=$(id -u):$(id -g) -v "$(pwd)":/sources \
    unfoldedcircle/r2-toolchain-qt-5.15.8-static@sha256:d4b1b81b4722586aa1bc9e6fc2d8ccf329872d71d6bbda40a40adb74060d31c6
```

Requires Docker. Output: `binaries/linux-arm64/release/remote-ui`

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

## Desktop build (macOS)

```bash
qmake && make -j$(sysctl -n hw.ncpu)
UC_MODEL=DEV ./binaries/osx-*/release/Remote\ UI
```

## Run tests

```bash
cd test/matrixrain
qmake matrixrain_test.pro && make -j4
./test_matrixrain.app/Contents/MacOS/test_matrixrain
```

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
