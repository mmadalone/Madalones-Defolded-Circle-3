#!/usr/bin/env bash
# Copyright (c) 2026 madalone. Canary deploy to UC Remote 3 with auto-revert.
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Flow:
#   1. POST the built tarball to /api/system/install/ui
#   2. Wait a short grace period for the UI process to restart
#   3. Poll /api/system/install/ui for up to HEALTH_TIMEOUT seconds, waiting
#      for `active: true` in the response
#   4a. Success → exit 0
#   4b. Health failure → PUT /api/system/install/ui?enable=false to revert,
#       exit 1
#   4c. Revert failure → exit 2 (critical, device may be in bad state)
#
# Environment variables (source .env.local first — see .env.local.example):
#   UC3_HOST    host or host:port of the device (or mock)
#   UC3_USER    HTTP basic auth username (default: web-configurator)
#   UC3_PIN     HTTP basic auth password
#
# Optional:
#   HEALTH_TIMEOUT   seconds to wait for healthy state (default 30)
#   POLL_INTERVAL    seconds between health polls (default 2)
#   GRACE_SECONDS    seconds to wait before first poll (default 3)
#   DRY_RUN=1        log all operations but don't call the API
#
# Usage:
#   set -a; source .env.local; set +a
#   ./scripts/deploy-canary.sh /tmp/remote-ui-deploy.tar.gz
#
# Mock rehearsal:
#   python3 scripts/mock-uc3-api.py &
#   MOCK_PID=$!
#   UC3_HOST=127.0.0.1:8080 UC3_USER=mock UC3_PIN=mock \
#     ./scripts/deploy-canary.sh /tmp/remote-ui-deploy.tar.gz
#   kill $MOCK_PID
#
# Exit codes:
#   0  success — deploy healthy
#   1  deploy failed, auto-revert succeeded (device back on stock UI)
#   2  CRITICAL: deploy failed AND auto-revert failed (manual recovery needed)
#   3  usage error

set -u

TARBALL="${1:-${TARBALL:-}}"
HEALTH_TIMEOUT="${HEALTH_TIMEOUT:-30}"
POLL_INTERVAL="${POLL_INTERVAL:-2}"
GRACE_SECONDS="${GRACE_SECONDS:-3}"
DRY_RUN="${DRY_RUN:-0}"

log() { echo "[canary] $*" >&2; }
err() { echo "[canary][ERROR] $*" >&2; }

if [ -z "$TARBALL" ]; then
    err "usage: $0 <path-to-tarball>"
    err "  or set TARBALL env var"
    exit 3
fi

if [ ! -f "$TARBALL" ] && [ "$DRY_RUN" != "1" ]; then
    err "tarball not found: $TARBALL"
    exit 3
fi

: "${UC3_HOST:?UC3_HOST not set (source .env.local first)}"
: "${UC3_USER:=web-configurator}"
: "${UC3_PIN:?UC3_PIN not set (source .env.local first)}"

INSTALL_URL="http://${UC3_HOST}/api/system/install/ui"
INSTALL_POST_URL="${INSTALL_URL}?void_warranty=yes"
REVERT_URL="${INSTALL_URL}?enable=false"
AUTH="${UC3_USER}:${UC3_PIN}"

log "target:    ${UC3_HOST}"
log "artifact:  ${TARBALL}"
log "timeout:   ${HEALTH_TIMEOUT}s (poll every ${POLL_INTERVAL}s)"
log "dry-run:   ${DRY_RUN}"

# --- Stage 1: POST artifact ---
log "stage 1/3: POST ${INSTALL_POST_URL}"
if [ "$DRY_RUN" = "1" ]; then
    log "  DRY_RUN=1 — skipping actual upload"
    INSTALL_RESP='{"installed":true,"active":true}'
    INSTALL_RC=0
else
    INSTALL_RESP=$(curl --location --silent --show-error --max-time 120 \
        --form "file=@${TARBALL}" \
        -u "$AUTH" \
        "$INSTALL_POST_URL" 2>&1)
    INSTALL_RC=$?
fi

if [ $INSTALL_RC -ne 0 ]; then
    err "install POST failed (curl exit $INSTALL_RC): $INSTALL_RESP"
    exit 1
fi
log "  response: $INSTALL_RESP"

# Quick sanity: response should mention installed=true
if ! echo "$INSTALL_RESP" | grep -q '"installed":[[:space:]]*true'; then
    err "install response did not confirm installed=true"
    err "  full: $INSTALL_RESP"
    # Try to revert even though we're not sure what state the device is in
    goto_revert=1
else
    goto_revert=0
fi

# --- Stage 2: Grace period ---
log "stage 2/3: waiting ${GRACE_SECONDS}s for UI process restart"
sleep "$GRACE_SECONDS"

# --- Stage 3: Health poll ---
if [ $goto_revert -eq 0 ]; then
    log "stage 3/3: polling health"
    elapsed=0
    healthy=0
    last_resp=""
    while [ $elapsed -lt "$HEALTH_TIMEOUT" ]; do
        if [ "$DRY_RUN" = "1" ]; then
            healthy=1
            break
        fi
        last_resp=$(curl --silent --show-error --max-time 5 \
            -u "$AUTH" "$INSTALL_URL" 2>&1) || last_resp="<curl failed>"
        if echo "$last_resp" | grep -q '"active":[[:space:]]*true'; then
            healthy=1
            break
        fi
        log "  not healthy yet (elapsed ${elapsed}s)"
        sleep "$POLL_INTERVAL"
        elapsed=$((elapsed + POLL_INTERVAL))
    done

    if [ $healthy -eq 1 ]; then
        log "✓ deploy healthy (elapsed ${elapsed}s)"
        exit 0
    fi

    err "✗ deploy NOT healthy after ${HEALTH_TIMEOUT}s"
    err "  last response: ${last_resp}"
fi

# --- Stage 4: Auto-revert ---
log "stage 4: reverting to stock UI via ${REVERT_URL}"
if [ "$DRY_RUN" = "1" ]; then
    log "  DRY_RUN=1 — skipping actual revert"
    exit 1
fi

REVERT_RESP=$(curl --silent --show-error --max-time 30 -X PUT \
    -u "$AUTH" "$REVERT_URL" 2>&1)
REVERT_RC=$?

if [ $REVERT_RC -ne 0 ]; then
    err "CRITICAL: revert API call failed (curl exit $REVERT_RC)"
    err "  response: $REVERT_RESP"
    err "  device may be in an inconsistent state — manual recovery required"
    exit 2
fi

log "  revert response: $REVERT_RESP"

# Verify revert took effect
if ! echo "$REVERT_RESP" | grep -qE '"(active|enabled)":[[:space:]]*false'; then
    err "CRITICAL: revert response did not confirm disabled state"
    err "  full: $REVERT_RESP"
    exit 2
fi

log "✓ deploy reverted successfully — device back on stock UI"
exit 1
