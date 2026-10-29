#!/usr/bin/env bash
# Packs the OHPM package of each backend that the OpenHarmony presets name:
#
#   pack-ohpm.sh [--version <version>] <out dir> <preset>...
#
# A user installs one package, such as @maplibre/native-ffi-opengl. OHPM has
# no per-platform packages, so a backend's HAR holds the files in ohpm/package,
# the manifest in ohpm/oh-package.json, the bundled ArkTS entry, and, for each
# ABI that a preset names, the addon, shared C library, and shared C++ runtime
# that the build task installed. The C headers live under include/. Every
# package takes the same version, 0.0.0 unless --version names one.
set -euo pipefail

usage="usage: pack-ohpm.sh [--version <version>] <out dir> <preset>..."
version=0.0.0
if [[ ${1:-} == --version ]]; then
  version=${2:?$usage}
  shift 2
fi
out=${1:?$usage}
shift
if (($# == 0)); then
  echo "$usage" >&2
  exit 2
fi
binding="$(cd "$(dirname "$0")/.." && pwd)"
staging="$binding/build/ohpm-packages"
# The package declares the API floor that every OpenHarmony preset compiles
# against, which the CI packing job reads without a configured build.
api=$(jq -er '.configurePresets[] | select(.name == "_ohos")
  | .cacheVariables.OHOS_COMPATIBLE_SDK_VERSION' "$binding/../../CMakePresets.json")
rm -rf "$staging"
mkdir -p "$out"

(
  cd "$binding/ohpm"
  MAPLIBRE_NATIVE_FFI_OHPM_BUNDLE="$staging/bundle" pnpm exec vp pack >/dev/null
)

for preset in "$@"; do
  case "$preset" in
    ohos-arm64-egl) abi=arm64-v8a backend=opengl ;;
    ohos-arm64-vulkan) abi=arm64-v8a backend=vulkan ;;
    ohos-x64-egl) abi=x86_64 backend=opengl ;;
    ohos-x64-vulkan) abi=x86_64 backend=vulkan ;;
    *)
      echo "error: $preset has no OHPM package; only OpenHarmony presets do" >&2
      exit 2
      ;;
  esac
  package="$staging/$backend/package"
  if [[ ! -d $package ]]; then
    mkdir -p "$package"
    cp -R "$binding/ohpm/package/." "$staging/bundle/." "$package/"
    cp "$binding/../../LICENSE" "$package/"
    # The formatter writes a .json5 file in JSON5's own style, which jq cannot
    # read, so the manifest is kept as JSON, which JSON5 readers accept too.
    jq --arg name "@maplibre/native-ffi-$backend" --arg version "$version" \
      --argjson api "$api" \
      '.name = $name | .version = $version | .compatibleSdkVersion = $api
        | .nativeComponents[].compatibleSdkVersion = $api' \
      "$binding/ohpm/oh-package.json" >"$package/oh-package.json5"
    # OHPM's review rejects a package whose changelog does not name its version.
    printf '\n## %s\n\nSee the [release notes](%s/v%s).\n' "$version" \
      https://github.com/maplibre/maplibre-native-ffi/releases/tag "$version" \
      >>"$package/CHANGELOG.md"
  fi
  # Every ABI of a backend carries the same headers and licenses.
  install="$binding/native/build/$preset/install"
  mkdir -p "$package/libs/$abi"
  cp "$install/libmaplibre_native_ffi.so" "$install/libmaplibre-native-c.so" \
    "$install/libc++_shared.so" \
    "$package/libs/$abi/"
  cp -R "$install/include" "$install/licenses" "$package/"
  mkdir -p "$staging/$backend/inventories"
  cp "$install/sbom/maplibre-native-ffi.cdx.json" "$staging/$backend/inventories/$preset.json"
done

for package in "$staging"/*/package; do
  backend="$(basename "$(dirname "$package")")"
  sbom_inputs=()
  for inventory in "$staging/$backend/inventories"/*.json; do
    sbom_inputs+=(--input "$inventory")
  done
  python3 "$binding/../../tools/release/binding_sbom.py" package \
    --name "@maplibre/native-ffi-$backend" --version "$version" --ecosystem generic \
    --output "$package/sbom/maplibre-native-ffi.cdx.json" "${sbom_inputs[@]}"
  tar -czf "$out/native-ffi-$backend-$version.har" -C "$(dirname "$package")" package
done
