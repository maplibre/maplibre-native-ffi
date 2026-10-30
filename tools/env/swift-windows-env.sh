# Sourced by Swift tasks after native-env.sh on a Windows host.
# shellcheck shell=bash

# Swift uses Microsoft's SDK and CRT even when the C DLL came from MinGW.
source "$MISE_MONOREPO_ROOT/tools/env/windows-msvc-env.sh"
swift_windows_import_dir="$native_install_dir/lib"
if [[ -f "$native_install_dir/lib/libmaplibre-native-c.dll.a" ]]; then
  # lld-link reads GNU import archives, but Swift's -l flag searches for .lib.
  swift_windows_import_dir="$MISE_MONOREPO_ROOT/.build/$usage_preset/import"
  mkdir -p "$swift_windows_import_dir"
  cp "$native_install_dir/lib/libmaplibre-native-c.dll.a" "$swift_windows_import_dir/maplibre-native-c.lib"
  # pkg-config also names the Rust platform archive installed beside the DLL.
  cp "$native_install_dir/lib/libmln_ffi_platform.a" "$swift_windows_import_dir/mln_ffi_platform.lib"
fi
swift_windows_args=(
  --scratch-path ".build/$usage_preset"
  -Xcc "-I$native_install_dir/include"
  -Xlinker "-libpath:$(cygpath -m "$swift_windows_import_dir")"
  -Xlinker "$swift_windows_import_dir/maplibre-native-c.lib"
)
