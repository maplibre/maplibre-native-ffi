#!/usr/bin/env bash
# Packs the OHPM facade and the backends that the OpenHarmony presets name:
#
#   pack-ohpm.sh [--version <version>] <out dir> <preset>...
#
# A user installs @maplibre/native-ffi and one backend package. The facade
# holds the ArkTS entry and the addon's type declarations. A backend HAR
# holds the addon, shared C library, and shared C++ runtime for every ABI
# that its presets name, plus the C headers and native licenses.
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

# HARs carry the merged module manifest that hvigor normally generates,
# including the API floor it checks when merging a dependency into a HAP.
prepare_package() {
  local kind=$1 name=$2 package="$staging/$1/package"
  mkdir -p "$package/src/main"
  cp "$binding/ohpm/package/README.md" "$binding/ohpm/package/CHANGELOG.md" \
    "$binding/../../LICENSE" "$package/"
  jq --arg name "$name" --arg version "$version" --arg kind "$kind" \
    --argjson api "$api" '
      .name = $name | .version = $version | .compatibleSdkVersion = $api
      | if $kind == "facade" then del(.nativeComponents)
        else del(.main, .dependencies)
          | .description = ("The " + $kind + " backend of @maplibre/native-ffi on OpenHarmony.")
          | .nativeComponents[].compatibleSdkVersion = $api
        end' "$binding/ohpm/oh-package.json" >"$package/oh-package.json5"
  jq --arg kind "$kind" --argjson api "$api" '
      .app = {minAPIVersion: $api, targetAPIVersion: $api, apiReleaseType: "Release"}
      | .module.name += ("_" + $kind)' \
    "$binding/ohpm/package/src/main/module.json" >"$package/src/main/module.json"
  if [[ $kind != facade ]]; then
    # OHPM requires an entry declaration even for a package of native libraries.
    printf 'export {};\n' >"$package/index.d.ts"
  fi
  # OHPM's review rejects a package whose changelog does not name its version.
  printf '\n## %s\n\nSee the [release notes](%s/v%s).\n' "$version" \
    https://github.com/maplibre/maplibre-native-ffi/releases/tag "$version" \
    >>"$package/CHANGELOG.md"
}

prepare_package facade @maplibre/native-ffi
facade="$staging/facade/package"
cp -R "$staging/bundle/." "$facade/"
cp -R "$binding/ohpm/package/src/main/cpp" "$facade/src/main/"

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
    prepare_package "$backend" "@maplibre/native-ffi-$backend"
  fi
  install="$binding/native/build/$preset/install"
  mkdir -p "$package/libs/$abi"
  cp "$install/libmaplibre_native_ffi.so" "$install/libmaplibre-native-c.so" \
    "$install/libc++_shared.so" "$package/libs/$abi/"
  cp -R "$install/include" "$install/licenses" "$package/"
  mkdir -p "$staging/$backend/inventories"
  cp "$install/sbom/maplibre-native-ffi.cdx.json" "$staging/$backend/inventories/$preset.json"
done

for package in "$staging"/*/package; do
  kind="$(basename "$(dirname "$package")")"
  name=$(jq -er .name "$package/oh-package.json5")
  sbom_inputs=()
  if [[ $kind != facade ]]; then
    for inventory in "$staging/$kind/inventories"/*.json; do
      sbom_inputs+=(--input "$inventory")
    done
  fi
  python3 "$binding/../../tools/release/binding_sbom.py" package \
    --name "$name" --version "$version" --ecosystem generic \
    --output "$package/sbom/maplibre-native-ffi.cdx.json" "${sbom_inputs[@]}"
  tar -czf "$out/${name#@maplibre/}-$version.har" -C "$(dirname "$package")" package
done
