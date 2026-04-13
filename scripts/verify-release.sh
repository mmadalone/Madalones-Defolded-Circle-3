#!/usr/bin/env bash
# Copyright (c) 2026 madalone. Verify a downloaded UC Remote 3 release.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Checks:
#   1. SHA256 checksum matches the published hashes file
#   2. GPG signature matches the project's public key (if signed)
#
# Usage:
#   ./scripts/verify-release.sh <tarball> [<hashes-file>] [<signature-file>]
#
# Defaults:
#   hashes-file:    <tarball-dir>/remote-ui.hash
#   signature-file: <tarball>.asc   (skipped if missing — verify-release exits 0)
#
# Example:
#   ./scripts/verify-release.sh \
#       remote-ui-v1.2.2-UCR2-static.tar.gz \
#       remote-ui.hash \
#       remote-ui-v1.2.2-UCR2-static.tar.gz.asc
#
# Exit codes:
#   0  all checks passed (or GPG skipped because no sig file)
#   1  SHA256 mismatch
#   2  GPG signature failed or public key not imported
#   3  usage error

set -u

TARBALL="${1:-}"
HASHES="${2:-}"
SIG="${3:-}"

log() { echo "[verify] $*" >&2; }
err() { echo "[verify][ERROR] $*" >&2; }

if [ -z "$TARBALL" ]; then
    err "usage: $0 <tarball> [<hashes-file>] [<signature-file>]"
    exit 3
fi

if [ ! -f "$TARBALL" ]; then
    err "tarball not found: $TARBALL"
    exit 3
fi

# --- SHA256 check ---
if [ -z "$HASHES" ]; then
    HASHES="$(dirname "$TARBALL")/remote-ui.hash"
fi

if [ ! -f "$HASHES" ]; then
    err "hashes file not found: $HASHES"
    err "  (pass as 2nd arg or place remote-ui.hash next to the tarball)"
    exit 3
fi

log "SHA256 check against $HASHES"
BASENAME=$(basename "$TARBALL")
# Hashes file format (from build.yml):
#   sha256  <hash>  <filename>
EXPECTED=$(awk -v fn="$BASENAME" '$1 == "sha256" && $3 == fn { print $2 }' "$HASHES")

if [ -z "$EXPECTED" ]; then
    err "no SHA256 entry for $BASENAME in $HASHES"
    exit 1
fi

# shasum on macOS, sha256sum on Linux — prefer shasum for portability
if command -v shasum >/dev/null 2>&1; then
    ACTUAL=$(shasum -a 256 "$TARBALL" | awk '{print $1}')
elif command -v sha256sum >/dev/null 2>&1; then
    ACTUAL=$(sha256sum "$TARBALL" | awk '{print $1}')
else
    err "neither shasum nor sha256sum available — install coreutils"
    exit 3
fi

if [ "$EXPECTED" != "$ACTUAL" ]; then
    err "SHA256 MISMATCH"
    err "  expected: $EXPECTED"
    err "  actual:   $ACTUAL"
    exit 1
fi
log "✓ SHA256 matches ($ACTUAL)"

# --- GPG signature check ---
if [ -z "$SIG" ]; then
    SIG="${TARBALL}.asc"
fi

if [ ! -f "$SIG" ]; then
    log "no signature file at $SIG — skipping GPG check"
    log "(for signed releases, pass the .asc file as 3rd arg)"
    exit 0
fi

if ! command -v gpg >/dev/null 2>&1; then
    err "gpg not installed — cannot verify $SIG"
    err "  install: brew install gnupg (macOS) / apt install gnupg (Linux)"
    exit 2
fi

log "GPG signature check against $SIG"
if ! gpg --verify "$SIG" "$TARBALL" 2>&1 | tee /tmp/verify-release-gpg.log; then
    err "GPG signature verification FAILED"
    err "  if this is 'No public key' — import the release key:"
    err "    gpg --import docs/release-pubkey.asc"
    err "  otherwise the artifact is tampered or the key is wrong"
    exit 2
fi

log "✓ GPG signature valid"
log "all checks passed"
exit 0
