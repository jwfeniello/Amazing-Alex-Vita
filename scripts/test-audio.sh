#!/usr/bin/env bash
set -euo pipefail
project="$(cd "$(dirname "$0")/.." && pwd)"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/alex-audio.XXXXXX")"
cc -std=gnu11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I"$project/tests/audio_stubs" -I"$project/lib" -I"$project/source" \
  "$project/tests/audio_host.c" -o "$test_root/test-audio"
"$test_root/test-audio"
