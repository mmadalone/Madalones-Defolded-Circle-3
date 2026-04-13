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
    unfoldedcircle/r2-toolchain-qt-5.15.8-static:latest
```

Requires Docker. Output: `binaries/linux-arm64/release/remote-ui`

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
