#!/usr/bin/env bash
set -e
bin=./build/src/athena

run() { printf "$1" | "$bin"; }

# Modern start position, depths 1–3
run "uci
isready
position modern
go depth 1
go depth 2
go depth 3
quit
"

# Classic start position, depths 1–3
run "uci
isready
position classic
go depth 1
go depth 2
go depth 3
quit
"
