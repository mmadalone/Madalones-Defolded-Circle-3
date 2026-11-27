#!/usr/bin/env bash
# Copyright (c) 2026 madalone. Test of scripts/verify-release.sh with throwaway keys and files.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# verify-release.sh is the check a user runs on a download and the check build.yml's release job runs before it
# publishes. Up to v1.6.5 it reported success for a release without a signature and for a signature by any key
# in the user's keyring. This test pins what it must refuse and what it must accept.
#
# It makes two keys in a temporary keyring: a stand-in for the release key (a certify-only primary key with a
# signing subkey; the real releases are signed by a subkey too) and "someone else". The script under test is a copy of verify-release.sh
# whose pinned fingerprint is the stand-in's, next to a docs/release-pubkey.asc that holds the stand-in's public
# key, so the default key path is tested too. The real script, with the real fingerprint, is run once: it must
# refuse the stand-in's signature.
#
# Needs gpg and gpgv (2.1 or newer, for ed25519 keys). Nothing outside a temporary folder is touched.
#
#   bash scripts/test-verify-release.sh        exit 0: every case as expected; 1: a case failed (each is listed)

set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REAL_SCRIPT="$HERE/verify-release.sh"

WORK=$(mktemp -d) || { echo "FAIL could not create a temporary folder"; exit 1; }
export GNUPGHOME="$WORK/gnupg"
cleanup() {
    gpgconf --kill gpg-agent >/dev/null 2>&1
    rm -rf "$WORK"
}
trap cleanup EXIT
mkdir -p "$GNUPGHOME" "$WORK/repo/scripts" "$WORK/repo/docs" "$WORK/dl"
chmod 700 "$GNUPGHOME"

gpg_quiet() { gpg --batch --quiet --pinentry-mode loopback --passphrase '' "$@"; }
primary_of() { gpg --batch --with-colons --list-keys "$1" | awk -F: '$1 == "fpr" { print $10; exit }'; }

# --- the two keys ---
gpg_quiet --quick-generate-key "Stand-in Release <release@example.invalid>" ed25519 cert never \
    || { echo "FAIL could not make the stand-in release key (gpg 2.1 or newer is needed)"; exit 1; }
RELEASE_FPR=$(primary_of "release@example.invalid")
gpg_quiet --quick-add-key "$RELEASE_FPR" ed25519 sign never \
    || { echo "FAIL could not add the signing subkey"; exit 1; }
gpg_quiet --quick-generate-key "Someone Else <else@example.invalid>" ed25519 sign never \
    || { echo "FAIL could not make the other key"; exit 1; }
OTHER_FPR=$(primary_of "else@example.invalid")
gpg --batch --armor --export "$RELEASE_FPR" > "$WORK/repo/docs/release-pubkey.asc"
gpg --batch --armor --export "$OTHER_FPR" > "$WORK/other-pubkey.asc"

# --- the script under test: the real one with the stand-in's fingerprint ---
SCRIPT="$WORK/repo/scripts/verify-release.sh"
sed "s/^RELEASE_KEY_FINGERPRINT=.*/RELEASE_KEY_FINGERPRINT=\"$RELEASE_FPR\"/" "$REAL_SCRIPT" > "$SCRIPT"
if [ "$(grep -c "^RELEASE_KEY_FINGERPRINT=\"$RELEASE_FPR\"\$" "$SCRIPT")" -ne 1 ]; then
    echo "FAIL verify-release.sh has no single RELEASE_KEY_FINGERPRINT= line to replace"
    exit 1
fi
if ! grep -q '^RELEASE_KEY_FINGERPRINT="[0-9A-F]\{40\}"$' "$REAL_SCRIPT"; then
    echo "FAIL verify-release.sh does not pin a 40-character fingerprint"
    exit 1
fi

# --- the files ---
NAME="remote-ui-v9.9.9-UCR3-static.tar.gz"
sha_of() {
    if command -v sha256sum >/dev/null 2>&1; then sha256sum < "$1" | awk '{print $1}'; else shasum -a 256 < "$1" | awk '{print $1}'; fi
}
fresh() {      # a new download folder: the tarball, its hash file, no signature
    rm -rf "$WORK/dl"; mkdir -p "$WORK/dl"
    head -c 4096 /dev/urandom > "$WORK/dl/$NAME"
    echo "sha256  $(sha_of "$WORK/dl/$NAME")  $NAME" > "$WORK/dl/remote-ui.hash"
}
sign_with() {  # sign_with <key fingerprint>: a detached signature next to the tarball
    gpg_quiet --yes -u "$1" --detach-sign --armor --output "$WORK/dl/$NAME.asc" "$WORK/dl/$NAME"
}

FAILED=0
CASES=0
expect() {     # expect <exit code> <label> <script> [args...]
    local want="$1" label="$2"; shift 2
    CASES=$((CASES + 1))
    local out; out=$("$@" 2>&1); local got=$?
    if [ "$got" -eq "$want" ]; then
        echo "ok   $label (exit $got)"
    else
        echo "FAIL $label: exit $got, expected $want"
        printf '%s\n' "$out" | sed 's/^/       /'
        FAILED=$((FAILED + 1))
    fi
}

fresh; sign_with "$RELEASE_FPR"
expect 0 "a good signature by the release key's signing subkey" bash "$SCRIPT" "$WORK/dl/$NAME"
expect 0 "the same, with the hash file and the signature named" \
    bash "$SCRIPT" "$WORK/dl/$NAME" "$WORK/dl/remote-ui.hash" "$WORK/dl/$NAME.asc"
expect 0 "the same, with the key named by --key" \
    bash "$SCRIPT" --key "$WORK/repo/docs/release-pubkey.asc" "$WORK/dl/$NAME"
expect 2 "the real script (the real fingerprint) refuses the stand-in's signature" \
    bash "$REAL_SCRIPT" --key "$WORK/repo/docs/release-pubkey.asc" "$WORK/dl/$NAME"

# Nothing has to be imported: with an empty keyring of the user's the check still passes, and that keyring is
# left as it was (the script reads the key file, in a temporary folder of its own).
mkdir -p "$WORK/empty-gnupg"; chmod 700 "$WORK/empty-gnupg"
CASES=$((CASES + 1))
OUT=$(GNUPGHOME="$WORK/empty-gnupg" bash "$SCRIPT" "$WORK/dl/$NAME" 2>&1); GOT=$?
if [ "$GOT" -eq 0 ] && [ -z "$(ls -A "$WORK/empty-gnupg")" ]; then
    echo "ok   a good signature with an empty keyring of the user's, which stays empty (exit 0)"
else
    echo "FAIL a good signature with an empty keyring of the user's: exit $GOT, expected 0; left in that keyring: $(ls -A "$WORK/empty-gnupg" | tr '\n' ' ')"
    printf '%s\n' "$OUT" | sed 's/^/       /'
    FAILED=$((FAILED + 1))
fi

# The hash file and the signature under other names than the defaults: the 2nd and 3rd argument are really used
mv "$WORK/dl/remote-ui.hash" "$WORK/dl/hashes.txt"; mv "$WORK/dl/$NAME.asc" "$WORK/dl/sig.asc"
expect 0 "the hash file and the signature under other names, named" \
    bash "$SCRIPT" "$WORK/dl/$NAME" "$WORK/dl/hashes.txt" "$WORK/dl/sig.asc"
expect 3 "the hash file under another name, not named" bash "$SCRIPT" "$WORK/dl/$NAME"
mv "$WORK/dl/hashes.txt" "$WORK/dl/remote-ui.hash"; mv "$WORK/dl/sig.asc" "$WORK/dl/$NAME.asc"

# A backslash in the path (a folder named so on Linux and macOS, a path separator on Windows): sha256sum and shasum
# then put a backslash in front of the hash they print, which up to v1.6.5 made a good download a "mismatch".
mkdir -p "$WORK/back\\slash"
cp "$WORK/dl/$NAME" "$WORK/dl/$NAME.asc" "$WORK/dl/remote-ui.hash" "$WORK/back\\slash/"
expect 0 "a good download under a path with a backslash in it" bash "$SCRIPT" "$WORK/back\\slash/$NAME"

fresh
expect 2 "no signature file" bash "$SCRIPT" "$WORK/dl/$NAME"
expect 0 "no signature file, --checksum-only" bash "$SCRIPT" --checksum-only "$WORK/dl/$NAME"
echo "sha256  0000000000000000000000000000000000000000000000000000000000000000  $NAME" > "$WORK/dl/remote-ui.hash"
expect 1 "a wrong hash, --checksum-only (the hash is still compared)" bash "$SCRIPT" --checksum-only "$WORK/dl/$NAME"

fresh; sign_with "$OTHER_FPR"
expect 2 "a signature by another key that the user's keyring trusts" bash "$SCRIPT" "$WORK/dl/$NAME"
expect 2 "a signature by another key, that key given as --key" \
    bash "$SCRIPT" --key "$WORK/other-pubkey.asc" "$WORK/dl/$NAME"

fresh; sign_with "$RELEASE_FPR"
head -c 4096 /dev/urandom > "$WORK/dl/$NAME"
expect 1 "the tarball changed after signing, the old hash file" bash "$SCRIPT" "$WORK/dl/$NAME"
echo "sha256  $(sha_of "$WORK/dl/$NAME")  $NAME" > "$WORK/dl/remote-ui.hash"
expect 2 "the tarball changed after signing, the hash file redone" bash "$SCRIPT" "$WORK/dl/$NAME"
expect 0 "the same, --checksum-only (it does not look at the signature)" \
    bash "$SCRIPT" --checksum-only "$WORK/dl/$NAME"

fresh; sign_with "$RELEASE_FPR"
expect 2 "the release key file is missing" bash "$SCRIPT" --key "$WORK/no-such-key.asc" "$WORK/dl/$NAME"
echo "not a key" > "$WORK/bad-key.asc"
expect 2 "the release key file is not a key" bash "$SCRIPT" --key "$WORK/bad-key.asc" "$WORK/dl/$NAME"
echo "sha256  0000000000000000000000000000000000000000000000000000000000000000  $NAME" > "$WORK/dl/remote-ui.hash"
expect 1 "a hash file with another hash" bash "$SCRIPT" "$WORK/dl/$NAME"
echo "sha256  $(sha_of "$WORK/dl/$NAME")  another-file.tar.gz" > "$WORK/dl/remote-ui.hash"
expect 1 "a hash file without this tarball" bash "$SCRIPT" "$WORK/dl/$NAME"
expect 3 "no tarball named" bash "$SCRIPT"
expect 3 "an unknown option" bash "$SCRIPT" --no-such-option "$WORK/dl/$NAME"

# One signature file that holds a signature by the release key and one by another key: refused, also when the key
# file holds both keys. Only the release key signs a release.
fresh
gpg_quiet --yes -u "$RELEASE_FPR" -u "$OTHER_FPR" --detach-sign --armor --output "$WORK/dl/$NAME.asc" "$WORK/dl/$NAME"
cat "$WORK/repo/docs/release-pubkey.asc" "$WORK/other-pubkey.asc" > "$WORK/both-keys.asc"
expect 2 "signatures by the release key and by another key, both keys given" \
    bash "$SCRIPT" --key "$WORK/both-keys.asc" "$WORK/dl/$NAME"
expect 2 "signatures by the release key and by another key, only the release key known" \
    bash "$SCRIPT" "$WORK/dl/$NAME"

# Last, because it revokes the stand-in in the keyring: a key file that carries the release key's revocation. gpgv
# still calls the signature valid (exit status 0); the script must not. gpg wrote the revocation certificate when
# it made the key (with a colon in front of its first line, so that it is not imported by accident).
fresh; sign_with "$RELEASE_FPR"
expect 0 "a good signature, before the key is revoked" bash "$SCRIPT" "$WORK/dl/$NAME"
sed 's/^:-----BEGIN/-----BEGIN/' "$GNUPGHOME/openpgp-revocs.d/$RELEASE_FPR.rev" > "$WORK/revocation.asc"
gpg --batch --quiet --import "$WORK/revocation.asc"
gpg --batch --armor --export "$RELEASE_FPR" > "$WORK/revoked-pubkey.asc"
expect 2 "the same signature, the release key revoked since" \
    bash "$SCRIPT" --key "$WORK/revoked-pubkey.asc" "$WORK/dl/$NAME"

if [ "$FAILED" -ne 0 ]; then
    echo "$FAILED of $CASES cases FAILED"
    exit 1
fi
echo "all $CASES cases as expected"
