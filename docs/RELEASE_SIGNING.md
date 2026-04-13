# Release signing

UC Remote 3 custom-firmware release artifacts are GPG-signed by a project-scoped key. The signature is validated by `scripts/verify-release.sh` before a user installs the artifact on their device.

This doc covers:

1. Generating the project's GPG key (one-time, by the maintainer)
2. Configuring GitHub Actions secrets so CI can sign releases
3. Publishing the public key so users can verify
4. How users verify a downloaded release
5. Key rotation procedure

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

Export the public half and commit it to the repo so users can import it during verification:

```bash
gpg --export --armor "$KEY_ID" > docs/release-pubkey.asc
git add docs/release-pubkey.asc
git commit -m "[chore] release: publish GPG public key $KEY_ID"
```

The public key is safe to commit — it only enables verification, not signing.

## 4. Verifying a release (users)

After downloading a release tarball from GitHub, verify it:

```bash
# One-time: import the project's public key
gpg --import docs/release-pubkey.asc

# Verify the artifact
./scripts/verify-release.sh \
    remote-ui-v1.2.2-UCR2-static.tar.gz \
    remote-ui.hash \
    remote-ui-v1.2.2-UCR2-static.tar.gz.asc
```

Exit codes:
- `0` — SHA256 matches, GPG signature valid
- `1` — SHA256 mismatch (tampered or corrupt download)
- `2` — GPG signature failed (tampered OR public key not imported)
- `3` — usage error (missing files)

## 5. Key rotation

Rotate every 2 years (before the key expires) or immediately on suspected compromise.

```bash
# Generate a new key (see step 1)
# Update the GitHub secrets (see step 2)
# Commit the new public key (see step 3)
# Revoke the old key and distribute the revocation certificate
gpg --output docs/release-pubkey-old-revoked.asc --gen-revoke $OLD_KEY_ID
git add docs/release-pubkey-old-revoked.asc
git commit -m "[chore] release: revoke previous GPG key $OLD_KEY_ID"
```

Users pull the new public key on their next verification run — no flag day required, though a CHANGELOG note is helpful.

## CI behavior when secrets are missing

`scripts/deploy-canary.sh` and `scripts/verify-release.sh` are self-contained — they don't depend on CI. The `.github/workflows/build.yml` release job:

- **If `RELEASE_GPG_PRIVATE_KEY` is set:** CI imports the key, signs every `.tar.gz` artifact, and uploads `.asc` files alongside the release. `verify-release.sh` can then validate signatures.
- **If the secret is unset:** CI skips signing entirely, emits a `::notice::` in the log, and still publishes the artifacts + SHA256 hashes. `verify-release.sh` still validates SHA256 and gracefully skips the GPG step when no `.asc` is present.

This graceful-skip design means a fresh clone produces working releases immediately; GPG signing is opt-in and can be added later without breaking the pipeline.
