#!/bin/bash
source "$(dirname "${BASH_SOURCE[0]}")/config.sh"
STACKCHAN_IP="$(yaml_value 'c["wifi"]["base_url"]' | sed 's|http://||')"

PING_INTERVAL=5
OFFLINE_THRESHOLD=3
UP_SCRIPT="$(dirname "${BASH_SOURCE[0]}")/stackcan-up.sh"
DOWN_SCRIPT="$(dirname "${BASH_SOURCE[0]}")/stackcan-down.sh"
LOG_TAG="stackcan-watchdog"

log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] [$LOG_TAG] $*"
}

online=false
fail_count=0

log "Watching ${STACKCHAN_IP} (interval=${PING_INTERVAL}s, offline_threshold=${OFFLINE_THRESHOLD})"

while true; do
  if ping -c 1 -W 1 "$STACKCHAN_IP" > /dev/null 2>&1; then
    fail_count=0

    if [ "$online" = false ]; then
      online=true
      log "DETECTED: Stack-chan online at ${STACKCHAN_IP}"
      log "Starting services..."
      if bash "$UP_SCRIPT"; then
        log "Services started successfully."
      else
        log "ERROR: Failed to start services (exit code $?)."
      fi
    fi
  else
    fail_count=$((fail_count + 1))

    if [ "$online" = true ] && [ "$fail_count" -ge "$OFFLINE_THRESHOLD" ]; then
      online=false
      log "OFFLINE: Stack-chan unreachable (${fail_count} consecutive failures)."
      log "Stopping services..."
      if bash "$DOWN_SCRIPT"; then
        log "Services stopped."
      else
        log "ERROR: Failed to stop services (exit code $?)."
      fi
    fi
  fi

  sleep "$PING_INTERVAL"
done
