#!/usr/bin/env bash
# Builds and signs the example's HAP for one ABI from the binding's OHPM HARs.
#
#   build-hap.sh <abi> <facade HAR> <runtime HAR>
#
# The HAP is build/<abi>/entry-default-signed.hap under the example.
#
# DevEco Studio builds apps with hvigor. Its packages are on the OpenHarmony npm
# registry, but their license is unknown: their package.json names a
# LICENSE.txt that they don't include. This script calls the OpenHarmony SDK's
# tools instead: ets-loader compiles the ArkUI pages with the SDK's own webpack
# configuration, es2abc merges them and the binding into modules.abc, and
# tools/ohos/pack-hap packs and signs the HAP. webpack.config.cjs gives
# ets-loader the API floor, so ArkTS linter errors fail the build as they do in
# DevEco Studio. CMake builds the app's own native module with the SDK's
# toolchain file.
set -euo pipefail

usage="usage: build-hap.sh <abi> <facade HAR> <runtime HAR>"
abi=${1:?$usage}
facade_har=${2:?$usage}
runtime_har=${3:?$usage}
example="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(dirname "${OHOS_SDK_NATIVE:?OHOS_SDK_NATIVE must name the OpenHarmony native SDK}")"
loader="$sdk/ets/build-tools/ets-loader"
# The ArkTS component names its compiler directory for the host it targets.
case "$(uname -s)" in
  Darwin) ark=build-mac ;;
  *) ark=build ;;
esac
es2abc="$loader/bin/ark/$ark/bin/es2abc"
for tool in "$loader/webpack.config.js" "$es2abc"; do
  if [[ ! -e "$tool" ]]; then
    echo "error: the OpenHarmony SDK has no $tool; reinstall it with mise -E ohos install --force ohos-sdk" >&2
    exit 1
  fi
done

work="$example/build/$abi"
module="$work/entry/src/main"
rm -rf "$work"
mkdir -p "$work/libs/$abi" "$work/cache"
cp -R "$example/entry" "$work/entry"

# Install both HARs in oh_modules, where ohpm puts them and ets-loader resolves
# them. The runtime carries the native libraries.
for har in "$facade_har" "$runtime_har"; do
  name=$(tar -xOzf "$har" package/oh-package.json5 | jq -er .name)
  mkdir -p "$work/entry/oh_modules/$name"
  tar -xzf "$har" -C "$work/entry/oh_modules/$name" --strip-components 1
done
facade="$work/entry/oh_modules/@maplibre/native-ffi"
runtime="$work/entry/oh_modules/$(tar -xOzf "$runtime_har" package/oh-package.json5 | jq -er .name)"
if [[ ! -f "$facade/oh-package.json5" || ! -d "$runtime/libs/$abi" ]]; then
  echo "error: pass the @maplibre/native-ffi HAR, then a runtime HAR with $abi libraries" >&2
  exit 1
fi
cp "$runtime/libs/$abi/"*.so "$work/libs/$abi/"

# The app requires and targets the API floor that the facade declares.
api=$(jq -er .compatibleSdkVersion "$facade/oh-package.json5")
jq --argjson api "$api" '.app.minAPIVersion = $api | .app.targetAPIVersion = $api' \
  "$example/entry/src/main/module.json" >"$module/module.json"
bundle=$(jq -er .app.bundleName "$module/module.json")

# The app's native module, which the pages import as libohos_window.so. Its
# declarations go in oh_modules, as ohpm installs a native module's types.
cmake -S "$example/entry/src/main/cpp" -B "$work/cpp" -G Ninja -Wno-deprecated \
  -DCMAKE_TOOLCHAIN_FILE="$OHOS_SDK_NATIVE/build/cmake/ohos.toolchain.cmake" \
  -DOHOS_ARCH="$abi" -DOHOS_COMPATIBLE_SDK_VERSION="$api" \
  -DCMAKE_BUILD_TYPE=Release
cmake --build "$work/cpp"
cp "$work/cpp/libohos_window.so" "$work/libs/$abi/"
mkdir -p "$work/entry/oh_modules/libohos_window.so"
cp "$example/entry/src/main/cpp/types/libohos_window/"* \
  "$work/entry/oh_modules/libohos_window.so/"

# ets-loader reads the project from the environment and a build file, and
# compiles the pages and abilities into records named <bundle>/entry/ets/<path>.
# Its webpack path mishandles packaged ES modules, so it leaves the facade
# external and imports it by the record name in harNameOhmMap. It deletes
# nodeModulesPath, which is its own output.
facade_main=$(jq -er .main "$facade/oh-package.json5")
facade_record="$bundle/entry/oh_modules/@maplibre/native-ffi/${facade_main%.*}"
jq -n --arg work "$work" --arg record "$facade_record" '{
  compileMode: "esmodule",
  packageManagerType: "ohpm",
  projectRootPath: $work,
  modulePathMap: { entry: ($work + "/entry") },
  nodeModulesPath: ($work + "/cache/oh_modules"),
  harNameOhmMap: { "@maplibre/native-ffi": ("@bundle:" + $record) },
  moduleName: "entry"
}' >"$work/build.json"
(
  # The configuration resolves its loaders from its own directory.
  cd "$loader"
  aceModuleRoot="$module/ets" aceModuleBuild="$work/ets" \
    aceModuleJsonPath="$module/module.json" \
    aceProfilePath="$module/resources/base/profile" \
    aceBuildJson="$work/build.json" cachePath="$work/cache" \
    node "$loader/node_modules/webpack/bin/webpack.js" \
    --config "$example/scripts/webpack.config.cjs" \
    --env compatibleSdkVersion="$api" --env buildMode=debug \
    --env compilerType=ark --env sourceMap=none
)
if [[ ! -s "$work/cache/filesInfo.txt" ]]; then
  echo "error: ets-loader didn't compile the pages" >&2
  exit 1
fi

# The facade's entry is one ES module, which becomes the record that the pages
# import. It names the packed addon by its file name, which becomes
# @app:<bundle>/entry/<name>. es2abc merges every record into modules.abc.
facade_source="$work/cache/facade.js"
BUNDLE="$bundle/entry" perl -pe '
  s#(["\x27])lib(\w+)\.so(["\x27])#$1\@app:$ENV{BUNDLE}/$2$3#g;
' "$facade/$facade_main" >"$facade_source"
cp "$work/cache/filesInfo.txt" "$work/records.txt"
printf '%s;%s;esm;%s;entry\n' "$facade_source" "$facade_record" \
  "entry/oh_modules/@maplibre/native-ffi/$facade_main" >>"$work/records.txt"
"$es2abc" --merge-abc --module --output "$work/modules.abc" "@$work/records.txt"

"$example/../../tools/ohos/pack-hap" "$module" "$work/modules.abc" "$work/libs" \
  "$work/entry-default-signed.hap"
echo "Signed $work/entry-default-signed.hap for $abi."
