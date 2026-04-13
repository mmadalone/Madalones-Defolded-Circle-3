#!/usr/bin/env python3
# Copyright (c) 2026 madalone. Mock UC Remote 3 API for canary deploy rehearsal.
# SPDX-License-Identifier: GPL-3.0-or-later
"""
Mock HTTP server simulating the UC Remote 3 install/revert endpoints.

Used by scripts/deploy-canary.sh in dry-run mode so the canary deploy pipeline
can be tested end-to-end without an actual UC3 device. No external dependencies
(stdlib only).

Endpoints:
  POST /api/system/install/ui?void_warranty=yes   — accept multipart .tar.gz
  PUT  /api/system/install/ui?enable=false        — revert (disable custom UI)
  PUT  /api/system/install/ui?enable=true         — re-enable custom UI
  GET  /api/system/install/ui                     — query install status
                                                    (canary uses this as health)

Behavior modes (env vars — set before launching):

  MOCK_PORT=8080            port to listen on (default 8080)
  MOCK_FAIL_HEALTH=1        install returns 200 but GET /api/system/install/ui
                            always reports `active: false` — simulates a bad
                            deploy where the binary installed but the UI
                            process never came up
  MOCK_FAIL_INSTALL=1       install endpoint returns HTTP 500 — simulates a
                            rejected upload (corrupt tarball, auth failure,
                            disk full, etc.)
  MOCK_FAIL_REVERT=1        revert endpoint returns HTTP 500 — simulates the
                            worst case where auto-revert also fails
  MOCK_VERBOSE=1            log request/response bodies to stderr

Auth is accepted but not validated (the real UC3 requires Basic auth with
`web-configurator:$UC3_PIN` — the mock accepts anything so the canary script
doesn't have to hard-code test credentials).

Usage:
  # Normal run (exit with Ctrl-C):
  python3 scripts/mock-uc3-api.py

  # Simulate bad deploy:
  MOCK_FAIL_HEALTH=1 python3 scripts/mock-uc3-api.py

  # From deploy-canary.sh's POV:
  python3 scripts/mock-uc3-api.py &
  MOCK_PID=$!
  source .env.local  # UC3_HOST=127.0.0.1:8080 UC3_USER=mock UC3_PIN=mock
  ./scripts/deploy-canary.sh /tmp/remote-ui-deploy.tar.gz
  kill $MOCK_PID
"""

import json
import os
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs

PORT = int(os.environ.get('MOCK_PORT', '8080'))
FAIL_HEALTH = os.environ.get('MOCK_FAIL_HEALTH', '') == '1'
FAIL_INSTALL = os.environ.get('MOCK_FAIL_INSTALL', '') == '1'
FAIL_REVERT = os.environ.get('MOCK_FAIL_REVERT', '') == '1'
VERBOSE = os.environ.get('MOCK_VERBOSE', '') == '1'

# In-memory state updated by POST/PUT, read by GET
state = {
    'component': 'ui',
    'installed': False,
    'active': False,
    'installation_date': '',
    'release': {
        'name': {'en': 'Stock UI'},
        'version': '0.0.0',
        'description': {'en': '(pre-install placeholder)'},
        'developer': {'name': '', 'url': ''},
    },
}


def _log(msg):
    sys.stderr.write(f"[mock-uc3] {msg}\n")
    sys.stderr.flush()


class Handler(BaseHTTPRequestHandler):
    def _send_json(self, code, body):
        payload = json.dumps(body).encode('utf-8')
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)
        if VERBOSE:
            _log(f"  → {code} {body}")

    def do_POST(self):
        parsed = urlparse(self.path)
        if parsed.path != '/api/system/install/ui':
            self._send_json(404, {'error': 'endpoint not found'})
            return

        length = int(self.headers.get('Content-Length', '0'))
        if length > 0:
            # Drain the multipart body — we don't need to parse it for the
            # canary rehearsal, just consume it so the client gets a response.
            self.rfile.read(length)

        if FAIL_INSTALL:
            self._send_json(500, {
                'error': 'MOCK_FAIL_INSTALL=1 — simulated install failure',
            })
            return

        state['installed'] = True
        state['active'] = True
        state['installation_date'] = '2026-04-13T22:00:00+02:00'
        state['release'] = {
            'name': {'en': 'Custom Charging Screen (MOCK)'},
            'version': '1.2.2',
            'description': {'en': 'Mock UC3 API for canary rehearsal'},
            'developer': {'name': 'mock', 'url': 'http://127.0.0.1'},
        }
        self._send_json(200, state)

    def do_PUT(self):
        parsed = urlparse(self.path)
        if parsed.path != '/api/system/install/ui':
            self._send_json(404, {'error': 'endpoint not found'})
            return

        params = parse_qs(parsed.query)
        if 'enable' not in params:
            self._send_json(400, {'error': 'missing enable query param'})
            return

        if FAIL_REVERT:
            self._send_json(500, {
                'error': 'MOCK_FAIL_REVERT=1 — simulated revert failure',
            })
            return

        enable = params['enable'][0].lower() == 'true'
        state['active'] = enable
        self._send_json(200, {
            'component': 'ui',
            'active': enable,
            'enabled': enable,
        })

    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path != '/api/system/install/ui':
            self._send_json(404, {'error': 'endpoint not found'})
            return

        if FAIL_HEALTH:
            # Install returned 200 but the UI process never came up healthy —
            # active stays false so the canary health poll fails.
            view = dict(state)
            view['active'] = False
            view['health'] = 'unhealthy (MOCK_FAIL_HEALTH=1)'
            self._send_json(200, view)
        else:
            self._send_json(200, state)

    def log_message(self, format, *args):
        _log(f"{self.address_string()} {format % args}")


def main():
    addr = ('127.0.0.1', PORT)
    srv = HTTPServer(addr, Handler)
    _log(f"listening on http://127.0.0.1:{PORT}")
    _log(f"modes: FAIL_HEALTH={FAIL_HEALTH} "
         f"FAIL_INSTALL={FAIL_INSTALL} "
         f"FAIL_REVERT={FAIL_REVERT} "
         f"VERBOSE={VERBOSE}")
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        _log("shutting down")


if __name__ == '__main__':
    main()
