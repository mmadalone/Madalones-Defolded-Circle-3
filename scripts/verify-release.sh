#!/usr/bin/env bash
# Copyright (c) 2026 madalone. Verify a downloaded UC Remote 3 release.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Checks:
#   1. the SHA256 checksum matches the published hash file (the download is intact)
#   2. the detached signature was made by the project's release key (the file is ours)
#
# Usage:
#   ./scripts/verify-release.sh [--checksum-only] [--key <public-key-file>] \
#       <tarball> [<hashes-file>] [<signature-file>]
#
# Defaults:
#   hashes-file:     <tarball-dir>/remote-ui.hash
#   signature-file:  <tarball>.asc
#   public key:      docs/release-pubkey.asc of this checkout (--key names another copy of it)
#
# Example:
#   ./scripts/verify-release.sh \
#       remote-ui-v1.6.5-UCR3-static.tar.gz \
#       remote-ui.hash \
#       remote-ui-v1.6.5-UCR3-static.tar.gz.asc
#
# A release is signed, so a missing signature is a failure (up to v1.6.5 this script skipped the check and
# reported success). Development Builds (the "latest" pre-release) are not signed: check those with
# --checksum-only, which says plainly that nothing shows who made the file.
#
# The signature is checked with gpgv against the release key alone, in a temporary folder: your own keyring is
# neither read nor changed, and a signature by any other key fails, whatever keys you trust. The key that
# signed must belong to the primary key whose fingerprint is pinned below (docs/RELEASE_SIGNING.md prints it,
# and tells how to change it when the key is rotated).
#
# Exit codes:
#   0  all checks passed (with --checksum-only: the checksum matches, the signature was not checked)
#   1  SHA256 mismatch, or no line for the tarball in the hash file
#   2  no signature, a signature that does not verify, a signature by another key or by a revoked key,
#      the key file missing or unreadable, or gpgv/gpg missing
#   3  usage error: a missing file or argument, or no sha256 tool

set -u

# The primary key of docs/release-pubkey.asc ("madalone UC Remote 3 Release"). Releases are signed by its signing
# subkey; gpgv reports the primary key as the last field of its VALIDSIG line.
RELEASE_KEY_FINGERPRINT="8A6F881E750D0CE1D22AD58B3172A28DABF07621"

log() { echo "[verify] $*" >&2; }
err() { echo "[verify][ERROR] $*" >&2; }
usage() {
    err "usage: $0 [--checksum-only] [--key <public-key-file>] <tarball> [<hashes-file>] [<signature-file>]"
}

CHECKSUM_ONLY=0
KEY=""
POSITIONAL=()
while [ $# -gt 0 ]; do
    case "$1" in
        --checksum-only) CHECKSUM_ONLY=1 ;;
        --key)
            if [ $# -lt 2 ]; then usage; exit 3; fi
            KEY="$2"; shift ;;
        -h|--help) usage; exit 3 ;;
        --) shift; while [ $# -gt 0 ]; do POSITIONAL+=("$1"); shift; done; break ;;
        -*) err "unknown option: $1"; usage; exit 3 ;;
        *) POSITIONAL+=("$1") ;;
    esac
    shift
done

TARBALL="${POSITIONAL[0]:-}"
HASHES="${POSITIONAL[1]:-}"
SIG="${POSITIONAL[2]:-}"

if [ -z "$TARBALL" ] || [ "${#POSITIONAL[@]}" -gt 3 ]; then
    usage
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

# shasum on macOS, sha256sum on Linux — prefer shasum for portability. The file goes in on standard input: given a
# name with a backslash in it (a Windows path), both tools put a backslash in front of the hash, and the comparison
# below then failed for a good download (up to v1.6.5).
if command -v shasum >/dev/null 2>&1; then
    ACTUAL=$(shasum -a 256 < "$TARBALL" | awk '{print $1}')
elif command -v sha256sum >/dev/null 2>&1; then
    ACTUAL=$(sha256sum < "$TARBALL" | awk '{print $1}')
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

# --- Signature check ---
if [ "$CHECKSUM_ONLY" -eq 1 ]; then
    log "signature NOT checked (--checksum-only): the download matches the hash file,"
    log "but nothing shows who made it. Use this for Development Builds only."
    exit 0
fi

if [ -z "$SIG" ]; then
    SIG="${TARBALL}.asc"
fi

if [ ! -f "$SIG" ]; then
    err "no signature file at $SIG"
    err "  Releases are signed: download ${BASENAME}.asc from the same release page."
    err "  Development Builds (\"latest\") are not signed: check those with --checksum-only."
    exit 2
fi

if [ -z "$KEY" ]; then
    KEY="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/../docs/release-pubkey.asc"
fi
if [ ! -f "$KEY" ]; then
    err "release key not found: $KEY"
    err "  it is docs/release-pubkey.asc of the repository; name your copy with --key"
    exit 2
fi

if ! command -v gpgv >/dev/null 2>&1 || ! command -v gpg >/dev/null 2>&1; then
    err "gpg and gpgv are needed to verify $SIG"
    err "  install: brew install gnupg (macOS) / apt install gnupg (Linux)"
    exit 2
fi

WORK=$(mktemp -d 2>/dev/null) || { err "could not create a temporary folder"; exit 2; }
trap 'rm -rf "$WORK"' EXIT
mkdir "$WORK/home" && chmod 700 "$WORK/home"

# A keyring that holds the release key and nothing else. --dearmor only converts the file: no key is imported
# anywhere, and no agent is started.
if ! gpg --no-options --batch --homedir "$WORK/home" --dearmor < "$KEY" > "$WORK/release-key.gpg" 2> "$WORK/dearmor.err" \
        || [ ! -s "$WORK/release-key.gpg" ]; then
    err "could not read the release key $KEY:"
    sed 's/^/  /' "$WORK/dearmor.err" >&2
    exit 2
fi

log "signature check of $SIG against the release key"
STATUS=$(gpgv --homedir "$WORK/home" --keyring "$WORK/release-key.gpg" --status-fd 1 "$SIG" "$TARBALL" 2> "$WORK/gpgv.err")
GPGV_STATUS=$?
sed 's/^/  /' "$WORK/gpgv.err" >&2

if [ "$GPGV_STATUS" -ne 0 ]; then
    err "signature verification FAILED (gpgv exit status $GPGV_STATUS)"
    err "  the file is not signed by the release key, or it was changed after signing"
    exit 2
fi

# gpgv accepts a signature by a revoked key or subkey (exit status 0, with a REVKEYSIG line): refuse it. A signature
# by an expired key (EXPKEYSIG) is accepted: every signature of a key reports so once the key has expired, whenever
# it was made.
if printf '%s\n' "$STATUS" | grep -q '^\[GNUPG:\] REVKEYSIG '; then
    err "signature verification FAILED: the key that signed has been revoked"
    exit 2
fi

# [GNUPG:] VALIDSIG <key that signed> <date> ... <primary key>
SIGNERS=$(printf '%s\n' "$STATUS" | awk '$1 == "[GNUPG:]" && $2 == "VALIDSIG" { print $NF }' | sort -u)
if [ -z "$SIGNERS" ]; then
    err "signature verification FAILED: gpgv reported no valid signature"
    exit 2
fi
if [ "$SIGNERS" != "$RELEASE_KEY_FINGERPRINT" ]; then
    err "signature verification FAILED: signed by another key than the release key"
    err "  signed by: $(printf '%s' "$SIGNERS" | tr '\n' ' ')"
    err "  expected:  $RELEASE_KEY_FINGERPRINT"
    exit 2
fi

log "✓ signed by the release key $RELEASE_KEY_FINGERPRINT"
log "all checks passed"
exit 0
