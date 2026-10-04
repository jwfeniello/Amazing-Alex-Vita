#!/usr/bin/env bash
set -euo pipefail
project="$(cd "$(dirname "$0")/.." && pwd)"
test_root="$(mktemp -d "${TMPDIR:-/tmp}/alex-jni.XXXXXX")"
mkdir -p "$test_root/assets"
printf hello > "$test_root/assets/fixture.bin"
cc -std=gnu11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-incompatible-pointer-types \
  -Wno-discarded-qualifiers -DFALSOJNI_DEBUGLEVEL=4 -DFALSOJNI_HOST_TEST \
  "-DDATA_PATH=\"$test_root/\"" -I"$project/lib" -I"$project/source" \
  "$project/tests/jni_host.c" "$project/source/java.c" "$project/source/java_refs.c" \
  "$project/source/jni_log.c" "$project/lib/falso_jni/FalsoJNI.c" \
  "$project/lib/falso_jni/FalsoJNI_ImplBridge.c" "$project/lib/falso_jni/converter.c" \
  -pthread -o "$test_root/test-jni"
"$test_root/test-jni"
