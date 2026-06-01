#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG="$SCRIPT_DIR/../bridge/config.yaml"

yaml_value() {
  local expr="$1"
  python3 -c 'import yaml, sys; c=yaml.safe_load(open(sys.argv[1])); print('"$expr"', end="")' "$CONFIG"
}
