#!/usr/bin/env bash
set -e
bin=./build/src/athena
run() { printf "$1" | "$bin"; }
run "uci\nisready\nposition modern\nperft 1\nperft 2\nquit\n"
run "uci\nisready\nposition classic\nperft 1\nperft 2\nquit\n"
run "uci\nisready\nposition modern\ngo depth 2\nquit\n"
