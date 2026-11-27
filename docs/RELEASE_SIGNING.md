# Release signing

UC Remote 3 custom-firmware release artifacts are GPG-signed by a project-scoped key. The signature is validated by `scripts/verify-release.sh` before a user installs the artifact on their device, and by the release job itself before it publishes.

The release key (the primary key in `docs/release-pubkey.asc`, "madalone UC Remote 3 Release"):

```
8A6F 881E 750D 0CE1 D22A  D58B 3172 A28D ABF0 7621
```

Releases are signed by its signing subkey (`D4EB 43A3 627C 5451 1548  DEEF 741C D70F D3BC 103A`). `scripts/verify-release.sh` has the primary fingerprint built in and accepts no other key.

This doc covers:

1. Generating the project's GPG key (one-time, by the maintainer)
2. Configuring GitHub Actions secrets so CI can sign releases
3. Publishing the public key so users can verify
4. How users verify a downloaded release
5. Key rotation procedure
6. What the release job does

## 1. Generate the project GPG key (maintainer, one-time)

Run locally — never on a shared machine.

```bash
gpg --batch --full-generate-key <<EOF
%echo generating UC Remote 3 release signing key
Key-Type: RSA
Key-Length: 4096
Subkey-Type: RSA
Subkey-Length: 4096
Name-Real: madalone UC Remote 3 Release
Name-Email: releases@madalone-uc-remote.invalid
Expire-Date: 2y
Passphrase: $(openssl rand -base64 32)
%commit
%echo done
EOF
```

Notes:
- **Project-scoped identity** — the key is for releases, not personal mail. Use an `.invalid` email (RFC 2606) so it's clearly not a real address.
- **2-year expiry** — forces a conscious rotation. Shorter than the project's lifetime but long enough to avoid constant rotation churn.
- **4096-bit RSA** — overkill for a hobby project but the performance cost is negligible and it future-proofs.
- **Random passphrase** — generated fresh. Save it securely (password manager). You'll need it for CI.

Get the key ID:

```bash
gpg --list-secret-keys --keyid-format LONG | grep -A1 madalone-uc-remote.invalid
```

Note the `rsa4096/XXXXXXXXXXXXXXXX` portion — that's `$KEY_ID` below.

## 2. Configure GitHub Actions secrets

Export the private key ASCII-armored:

```bash
gpg --export-secret-keys --armor "$KEY_ID" > /tmp/release-private.asc
```

Add it to the repo as a GitHub Actions secret. **Never commit this file.**

Via `gh` CLI:

```bash
gh secret set RELEASE_GPG_PRIVATE_KEY < /tmp/release-private.asc --repo mmadalone/Madalones-Defolded-Circle-3
gh secret set RELEASE_GPG_PASSPHRASE --body "$PASSPHRASE" --repo mmadalone/Madalones-Defolded-Circle-3
```

Or via the GitHub web UI: Settings → Secrets and variables → Actions → New repository secret.

Delete the exported file:

```bash
shred -u /tmp/release-private.asc   # Linux
# macOS: rm -P /tmp/release-private.asc
```

## 3. Publish the public key

Export the public half into the repo, where `scripts/verify-release.sh` reads it, and put the key's primary fingerprint into that script (`RELEASE_KEY_FINGERPRINT`, 40 characters without blanks) and at the top of this file. All three go in one commit:

```bash
gpg --export --armor "$KEY_ID" > docs/release-pubkey.asc
gpg --show-keys --with-fingerprint docs/release-pubkey.asc     # the fingerprint to put into the script and this file
git add docs/release-pubkey.asc scripts/verify-release.sh docs/RELEASE_SIGNING.md
git commit -m "[chore] release: publish GPG public key $KEY_ID"
```

The public key is safe to commit — it only enables verification, not signing.

The script refuses every signature until `RELEASE_KEY_FINGERPRINT` names the new key, and the release job runs the script before it publishes, so a tag run with a new key and the old fingerprint publishes nothing.

## 4. Verifying a release (users)

After downloading a release tarball, its `.asc` and `remote-ui.hash` from GitHub, verify it from a checkout of this repository (the script reads `docs/release-pubkey.asc` of the same checkout; `--key <file>` names another copy):

```bash
./scripts/verify-release.sh \
    remote-ui-v1.6.5-UCR3-static.tar.gz \
    remote-ui.hash \
    remote-ui-v1.6.5-UCR3-static.tar.gz.asc
```

Nothing has to be imported: the script checks the signature with `gpgv` against the release key alone, in a temporary folder, and neither reads nor changes your own keyring. It needs `gpg` and `gpgv` (both come with GnuPG).

Since v1.6.6 a missing signature is a failure. Development Builds (the `latest` pre-release) are not signed: check those with `--checksum-only`, which compares the hash and says that nothing shows who made the file.

```bash
./scripts/verify-release.sh --checksum-only remote-ui-<version>-UCR3-static.tar.gz
```

Exit codes:
- `0` — SHA256 matches and the signature is the release key's (with `--checksum-only`: SHA256 matches, the signature was not checked)
- `1` — SHA256 mismatch (tampered or corrupt download), or no line for the tarball in the hash file
- `2` — no signature file, a signature that does not verify, a signature by another key or by a revoked key, the key file missing or unreadable, or `gpg`/`gpgv` missing
- `3` — usage error (a missing file or argument, or no sha256 tool)

`scripts/test-verify-release.sh` tests the script with throwaway keys; CI runs it (job "Release verification script").

## 5. Key rotation

Rotate every 2 years (before the key expires) or immediately on suspected compromise.

```bash
# Generate a new key (see step 1)
# Update the GitHub secrets (see step 2)
# Commit the new public key and its fingerprint in scripts/verify-release.sh (see step 3)
# Revoke the old key and distribute the revocation certificate
gpg --output docs/release-pubkey-old-revoked.asc --gen-revoke $OLD_KEY_ID
git add docs/release-pubkey-old-revoked.asc
git commit -m "[chore] release: revoke previous GPG key $OLD_KEY_ID"
```

Users get the new public key and the new fingerprint with their next pull. From then on their checkout verifies only releases signed by the new key: the script accepts one fingerprint. Say so in the CHANGELOG entry of the rotation, and name the last tag signed with the old key.

To check a release that was signed with the old key, run the script of that last tag from a separate worktree, so that your checkout stays as it is. That tag must be v1.6.6 or later: the script of an older tag passes a release without a signature.

```bash
git worktree add ../verify-old vX.Y.Z
../verify-old/scripts/verify-release.sh remote-ui-vX.Y.Z-UCR3-static.tar.gz remote-ui.hash remote-ui-vX.Y.Z-UCR3-static.tar.gz.asc
git worktree remove ../verify-old
```

The script refuses a signature by a key whose revocation is in the key file it reads, so a revocation added to `docs/release-pubkey.asc` makes every release that key signed fail the check: do that only for a compromised key. An expired key is accepted, because every signature of a key is reported as "by an expired key" once it has expired, whenever it was made.

## 6. What the release job does

The `release` job of `.github/workflows/build.yml` runs for a pushed tag `vX.Y.Z`, after the tests and both builds:

1. It writes `remote-ui.hash` (one `sha256  <hash>  <file>` line per tarball).
2. It signs every tarball with the key in `RELEASE_GPG_PRIVATE_KEY`, in a keyring of its own that is deleted before the next step. **If the secret is missing the job fails and nothing is published** (up to v1.6.5 it published an unsigned release with a notice in the log).
3. It runs `scripts/verify-release.sh` on every tarball: the check a user runs, against `docs/release-pubkey.asc` of the tagged commit. A secret that holds another key than the published one stops the release here.
4. It takes the release notes from the version's section of `CHANGELOG.md` (`tools/release_notes.py`).
5. It publishes with `gh release create --verify-tag`: the files go to a draft, which is published when they are all there.

A manual run of the workflow with "release_dry_run" ticked does steps 1 to 4 on any branch or tag and publishes nothing (only a pushed tag publishes): use it after a key rotation, before the next tag.

If a release run is cancelled while it uploads, a draft release can be left behind: delete it on the Releases page before running the job again.

Development Builds (`latest`, every push to `main` that changes code, tests, tools or the bundle) are not signed.
