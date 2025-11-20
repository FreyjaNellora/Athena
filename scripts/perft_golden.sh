#!/usr/bin/env bash
set -euo pipefail
bin=./build/src/athena

check() {
  local mode=$1 depth=$2 expect=$3
  got=$(printf "uci\nisready\nposition %s\nperft %d\nquit\n" "$mode" "$depth" | "$bin" \
    | awk '/^[[:space:]]*[0-9]+[[:space:]]/ { last=$2 } END { print last }')
  if [[ "$got" != "$expect" ]]; then
    echo "FAIL: $mode depth $depth expected $expect, got $got"
    exit 1
  fi
  echo "OK  : $mode depth $depth == $expect"
}

check modern 1 20
check modern 2 395
check classic 1 20
check classic 2 395
