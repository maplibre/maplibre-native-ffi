#!/usr/bin/env bash
# Builds the ArkTS conformance application and runs it on an OpenHarmony device.
#
# DevEco Studio drives this through hvigor, which is published nowhere public.
# hvigor is an orchestrator, though, and every tool it calls ships in the SDK:
# the ArkTS compiler turns the sources into bytecode, app_packing_tool assembles
# the hap, hap-sign-tool signs it with the SDK's own debug material, and hdc
# installs it and starts the runner. This is that sequence, written out.
set -euo pipefail

preset="${1:?usage: build-arkts-test.sh <preset>}"
root="${MISE_MONOREPO_ROOT:?MISE_MONOREPO_ROOT must be set}"
sdk_root="${OHOS_SDK_NATIVE:?OHOS_SDK_NATIVE must point at the OpenHarmony native SDK}"
# OHOS_SDK_NATIVE names the native component; its siblings hold the rest.
sdk="$(dirname "$sdk_root")"
ets="$sdk/ets"
toolchains="$sdk/toolchains"
es2abc="$ets/build-tools/ets-loader/bin/ark/build/bin/es2abc"

bundle_id=org.maplibre.nativeffi.conformance
module=entry_test
source_dir="$root/bindings/typescript/test-arkts"
work="$root/build/$preset/arkts-test"
payload="$root/bindings/typescript/runtime-arkts"

for tool in "$es2abc" "$toolchains/lib/app_packing_tool.jar" \
  "$toolchains/lib/hap-sign-tool.jar" "$toolchains/hdc"; do
  if [[ ! -e "$tool" ]]; then
    echo "the OpenHarmony SDK is missing $tool" >&2
    exit 1
  fi
done

# The addon is what the suite is testing, so its absence is a missing
# prerequisite rather than something to pack around.
addon="$payload/maplibre-native-ffi.so"
if [[ ! -f "$addon" ]]; then
  echo "no ArkTS payload at $addon; run //bindings/typescript:build:arkts $preset first" >&2
  exit 1
fi

case "$preset" in
ohos-arm64-*) abi=arm64-v8a ;;
ohos-x64-*) abi=x86_64 ;;
*)
  echo "build-arkts-test.sh takes an ohos preset, not $preset" >&2
  exit 1
  ;;
esac

rm -rf "$work"
mkdir -p "$work/ets" "$work/libs/$abi" "$work/resources/base/profile"
# A hap carries its native libraries under the ABI it was built for.
cp "$addon" "$work/libs/$abi/libmaplibre-native-ffi.so"
if [[ -d "$payload/lib" ]]; then
  cp "$payload"/lib/*.so "$work/libs/$abi/" 2>/dev/null || true
fi

# One bundle, because ArkTS resolves modules through its own build rather than
# through node_modules. The payload's addon import stays external: the runtime
# resolves a native module by library name.
# The bundler writes inside its own package and the artifact is copied out.
# Handing it an output directory elsewhere made its clean step delete this
# checkout twice, `.git` included, which in a worktree is an ordinary file and
# so no more protected than any other. `--no-clean` says the same thing twice on
# purpose: nothing here should be removing directories it did not create.
echo "bundling the conformance suite"
bundle_dir="$root/bindings/typescript/api/dist-arkts"
(
  cd "$root/bindings/typescript/api"
  pnpm exec vp pack src/arkts-entry.ts \
    --format esm \
    --no-clean \
    --out-dir dist-arkts \
    --deps.never-bundle 'libmaplibre-native-ffi.so'
)
cp "$bundle_dir/arkts-entry.mjs" "$work/ets/conformance.bundle.js"

# Each source becomes one bytecode record, and the records merge into the file
# the runtime loads.
echo "compiling ArkTS sources"
info="$work/filesInfo.txt"
: >"$info"
add_source() {
  local file="$1" record="$2"
  printf '%s;%s;esm;%s;%s\n' "$file" "$record" "$file" "$record" >>"$info"
}
add_source "$work/ets/conformance.bundle.js" "$module/ets/conformance.bundle"
while IFS= read -r -d '' source; do
  relative="${source#"$source_dir"/entry/src/ohosTest/}"
  target="$work/${relative%.ets}.ts"
  mkdir -p "$(dirname "$target")"
  cp "$source" "$target"
  add_source "$target" "$module/${relative%.ets}"
done < <(find "$source_dir/entry/src/ohosTest/ets" -name '*.ets' -print0)

"$es2abc" --merge-abc --module --extension=ts \
  --output "$work/modules.abc" "@$info"

# The manifest and resources the packing tool reads.
cp "$source_dir/entry/src/ohosTest/module.json5" "$work/module.json"
printf '{"src":["pages/Index"]}\n' >"$work/resources/base/profile/main_pages.json"
"$toolchains/restool" -i "$work/resources" -o "$work" -p "$bundle_id" -r ResourceTable \
  >/dev/null 2>&1 || true

echo "packing the hap"
unsigned="$work/$module-unsigned.hap"
mise exec -- java -jar "$toolchains/lib/app_packing_tool.jar" \
  --mode hap \
  --json-path "$work/module.json" \
  --ets-path "$work/modules.abc" \
  --lib-path "$work/libs" \
  --resources-path "$work/resources" \
  --out-path "$unsigned" \
  --force true

# Signing is two steps. The SDK ships an unsigned provisioning profile template
# rather than a signed profile, so the template is signed first and the hap is
# signed against the result.
echo "signing the provisioning profile"
profile="$work/profile.p7b"
manifest="$work/profile.json"
python3 - "$toolchains/lib/UnsgnedDebugProfileTemplate.json" "$manifest" "$bundle_id" <<'PYTHON'
import json
import sys

template, target, bundle = sys.argv[1], sys.argv[2], sys.argv[3]
with open(template) as source:
    profile = json.load(source)
# The template names an example application; a device checks this against the
# bundle it is asked to install.
profile["bundle-info"]["bundle-name"] = bundle
with open(target, "w") as destination:
    json.dump(profile, destination)
PYTHON

mise exec -- java -jar "$toolchains/lib/hap-sign-tool.jar" sign-profile \
  -keyAlias "openharmony application profile release" \
  -signAlg SHA256withECDSA \
  -mode localSign \
  -profileCertFile "$toolchains/lib/OpenHarmonyProfileRelease.pem" \
  -inFile "$manifest" \
  -keystoreFile "$toolchains/lib/OpenHarmony.p12" \
  -outFile "$profile" \
  -keyPwd 123456 \
  -keystorePwd 123456

echo "signing the hap"
signed="$work/$module.hap"
mise exec -- java -jar "$toolchains/lib/hap-sign-tool.jar" sign-app \
  -keyAlias "openharmony application release" \
  -signAlg SHA256withECDSA \
  -mode localSign \
  -appCertFile "$toolchains/lib/OpenHarmonyProfileRelease.pem" \
  -profileFile "$profile" \
  -inFile "$unsigned" \
  -keystoreFile "$toolchains/lib/OpenHarmony.p12" \
  -outFile "$signed" \
  -keyPwd 123456 \
  -keystorePwd 123456

echo "built $signed"
