#!/bin/bash
# stackan-down.sh - Stop Bridge + Voice Gateway tmux session

SESSION="stackcan"

set -euo pipefail

if ! tmux has-session -t "$SESSION" 2>/dev/null; then
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] No '$SESSION' session running."
  exit 0
fi

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Stopping '$SESSION' session..."
tmux kill-session -t "$SESSION"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Stopped."
