#!/usr/bin/env bash

root="$(cd "$(dirname "$0")/.." && pwd)"

nvim --headless -l "$root/test/mcp.lua" "$(command -v scout)"
exitCode=$?

: > "$(dirname "$1")/mcp-samples.md"
exit $exitCode
