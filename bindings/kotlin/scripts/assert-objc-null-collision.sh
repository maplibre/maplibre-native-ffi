#!/usr/bin/env bash
# Prove clang rejects a Kotlin-shaped Objective-C property named NULL and
# accepts NULL_POINTER. The Apple generated-header compile is what catches
# the next surprise; this locks in the failure that NativePointer.NULL hit.
set -euo pipefail

clang_bin=${CLANG:-clang}
if ! command -v "$clang_bin" >/dev/null; then
  echo "error: clang is required to assert the Objective-C NULL collision" >&2
  exit 1
fi

root=$(mktemp -d)
trap 'rm -rf "$root"' EXIT

mkdir -p \
  "$root/before/nullCollision.framework/Headers" \
  "$root/after/nullCollision.framework/Headers"

# maplibre-compose reported Kotlin/Native emitting
# `@property (readonly) ... *NULL`, then Clang expanding __stddef_null.h.
cat >"$root/before/nullCollision.framework/Headers/nullCollision.h" <<'EOF'
#include <stddef.h>

@interface NativePointer
@end

@interface NativePointerCompanion
@property (readonly) NativePointer *NULL;
@end
EOF

cat >"$root/after/nullCollision.framework/Headers/nullCollision.h" <<'EOF'
#include <stddef.h>

@interface NativePointer
@end

@interface NativePointerCompanion
@property (readonly) NativePointer *NULL_POINTER;
@end
EOF

cat >"$root/import-header.m" <<'EOF'
#import <nullCollision/nullCollision.h>
EOF

compile() {
  local search_dir=$1
  # Linux clang rejects -fobjc-arc. The NULL macro collision does not need ARC.
  "$clang_bin" -fsyntax-only -x objective-c \
    -F "$search_dir" \
    "$root/import-header.m"
}

echo "clang rejects @property ... *NULL"
if before_log=$(compile "$root/before" 2>&1); then
  echo "error: clang accepted a property named NULL" >&2
  exit 1
fi
printf '%s\n' "$before_log"
if ! grep -q "expected member name or ';' after declaration specifiers" <<<"$before_log"; then
  echo "error: clang failed for a reason other than the NULL macro collision" >&2
  exit 1
fi
if ! grep -q "__stddef_null.h" <<<"$before_log"; then
  echo "error: clang failed without expanding __stddef_null.h" >&2
  exit 1
fi

echo "clang accepts @property ... *NULL_POINTER"
compile "$root/after"
echo "Objective-C NULL collision assertion passed"
