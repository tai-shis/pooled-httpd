#!/usr/bin/env bash

# generated with ai

# Formats every .c and .h file under src/ in place, using the repo's .clang-format.
# Build output (bin/) is skipped, and third_party/ is outside src/, so Unity is never touched.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/." && pwd)"
cd "$repo_root"

find src -path '*/bin' -prune -o \( -name '*.c' -o -name '*.h' \) -print0 |
    xargs -0 clang-format -i
