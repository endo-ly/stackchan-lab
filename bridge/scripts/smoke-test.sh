#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${STACKCHAN_BRIDGE_URL:-http://127.0.0.1:8787}"

request() {
  local method="$1"
  local path="$2"
  local body="${3:-}"

  if [[ -n "$body" ]]; then
    curl -fsS -X "$method" "$BASE_URL$path" \
      -H "Content-Type: application/json" \
      -d "$body" | jq .
  else
    curl -fsS -X "$method" "$BASE_URL$path" | jq .
  fi
}

expect_status() {
  local expected="$1"
  local method="$2"
  local path="$3"
  local body="${4:-}"
  local actual

  actual="$(
    curl -sS -o /tmp/stackchan-bridge-smoke-response.json -w "%{http_code}" \
      -X "$method" "$BASE_URL$path" \
      -H "Content-Type: application/json" \
      -d "$body"
  )"

  jq . /tmp/stackchan-bridge-smoke-response.json
  if [[ "$actual" != "$expected" ]]; then
    echo "Expected HTTP $expected, got $actual for $method $path" >&2
    exit 1
  fi
}

request GET /health
request GET /version
request GET /capabilities
request GET /status
request POST /face '{"expression":"happy"}'
request POST /face '{"expression":"doubt"}'
expect_status 400 POST /face '{"expression":"thinking"}'
request POST /led '{"mood":"calm"}'
request POST /pose '{"pose":"neutral"}'
request POST /move '{"x":0,"y":450}'
request POST /reset
