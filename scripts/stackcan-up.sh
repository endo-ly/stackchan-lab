#!/bin/bash
SESSION="stackcan"
BRIDGE_DIR="/root/workspace/stackchan-lab/bridge"
VG_DIR="/root/workspace/voice-gateway"

source "$(dirname "${BASH_SOURCE[0]}")/config.sh"
VG_URL="$(yaml_value 'c["voice_gateway"]["base_url"]')"
VG_HOST="${VG_URL#http://}"
VG_HOST="${VG_HOST%:*}"
VG_PORT="${VG_URL##*:}"

set -euo pipefail

if tmux has-session -t "$SESSION" 2>/dev/null; then
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] Stopping existing '$SESSION' session..."
  tmux kill-session -t "$SESSION"
fi

tmux new-session -d -s "$SESSION" -n "voice"
tmux send-keys -t "$SESSION:voice" "cd $VG_DIR && uv run uvicorn app.main:app --host $VG_HOST --port $VG_PORT" Enter

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Voice Gateway starting on ${VG_HOST}:${VG_PORT}..."

sleep 2
tmux new-window -t "$SESSION" -n "bridge"
tmux send-keys -t "$SESSION:bridge" "cd $BRIDGE_DIR && npm start" Enter

echo "[$(date '+%Y-%m-%d %H:%M:%S')] Bridge starting..."
echo "[$(date '+%Y-%m-%d %H:%M:%S')] tmux attach -t $SESSION  to view logs."
