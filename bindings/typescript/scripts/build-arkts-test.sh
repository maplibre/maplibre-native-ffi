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
# A hap carries the application's own description beside the module's, and the
# packing tool rejects a manifest without it.
python3 - "$source_dir/entry/src/ohosTest/module.json5" "$work/module.json" "$bundle_id" <<'PYTHON'
import json
import sys

source, target, bundle = sys.argv[1], sys.argv[2], sys.argv[3]
with open(source) as handle:
    manifest = json.load(handle)
manifest["app"] = {
    "bundleName": bundle,
    "vendor": "maplibre",
    "versionCode": 1000000,
    "versionName": "1.0.0",
    "minAPIVersion": 12,
    "targetAPIVersion": 12,
    "apiReleaseType": "Release",
    "debug": True,
    "bundleType": "app",
    # The application carries its own icon and label, not only its abilities.
    "icon": "$media:icon",
    "label": "$string:app_name",
}
with open(target, "w") as handle:
    json.dump(manifest, handle)
PYTHON
printf '{"src":["pages/Index"]}\n' >"$work/resources/base/profile/main_pages.json"
# The device's manifest parser requires the icon, label, and pages a module
# names, so the resources they point at are compiled rather than skipped.
# restool takes the module directory, which holds the manifest beside the
# resources the manifest refers to, rather than the resource directory itself.
mkdir -p "$work/module/resources" "$work/compiled"
cp -r "$source_dir/entry/src/ohosTest/resources/." "$work/module/resources/"
cp "$work/module.json" "$work/module/module.json"
"$toolchains/restool" -i "$work/module" -o "$work/compiled" \
  -p "$bundle_id" -r ResourceTable -f

# The packing tool empties its own working directory on the way out, whatever
# paths it is given, so it is run from the scratch directory rather than from
# the checkout. Run from the repository root it deletes the repository.
echo "packing the hap"
unsigned="$work/$module-unsigned.hap"
# The tool empties its working directory as it exits, the hap it just wrote
# included, so it is given a directory of its own to consume and the hap is
# written outside it.
mkdir -p "$work/pack"
(
  cd "$work/pack"
  java -jar "$toolchains/lib/app_packing_tool.jar" \
    --mode hap \
    --json-path ../module.json \
    --ets-path ../modules.abc \
    --lib-path ../libs \
    --resources-path ../compiled/resources \
    --out-path "../$module-unsigned.hap"
)

# The SDK ships the signing keys but no application certificate. The release
# key's own certificate is self-signed and the verifier rejects it, so a
# certificate is issued for that key by the CA in the same keystore, and the
# chain the verifier walks is emitted alongside it.
echo "issuing the application certificate"
app_cert="$work/app-release.cer"
root_ca="$work/root-ca.cer"
sub_ca="$work/sub-ca.cer"
keytool -exportcert -rfc -alias "openharmony application root ca" \
  -keystore "$toolchains/lib/OpenHarmony.p12" -storetype PKCS12 \
  -storepass 123456 >"$root_ca"
keytool -exportcert -rfc -alias "openharmony application ca" \
  -keystore "$toolchains/lib/OpenHarmony.p12" -storetype PKCS12 \
  -storepass 123456 >"$sub_ca"

java -jar "$toolchains/lib/hap-sign-tool.jar" generate-app-cert \
  -keyAlias "openharmony application release" \
  -issuer "C=CN, O=OpenHarmony, OU=OpenHarmony Team, CN=OpenHarmony Application CA" \
  -issuerKeyAlias "openharmony application ca" \
  -subject "C=CN, O=OpenHarmony, OU=MapLibre, CN=MapLibre Native FFI Conformance" \
  -signAlg SHA256withECDSA \
  -keystoreFile "$toolchains/lib/OpenHarmony.p12" \
  -keystorePwd 123456 \
  -keyPwd 123456 \
  -issuerKeyPwd 123456 \
  -outForm certChain \
  -rootCaCertFile "$root_ca" \
  -subCaCertFile "$sub_ca" \
  -outFile "$app_cert"

# Signing is two steps. The SDK ships an unsigned provisioning profile template
# rather than a signed profile, so the template is signed first and the hap is
# signed against the result.
echo "signing the provisioning profile"
profile="$work/profile.p7b"
manifest="$work/profile.json"
python3 - "$toolchains/lib/UnsgnedDebugProfileTemplate.json" "$manifest" \
  "$bundle_id" "$app_cert" <<'PYTHON'
import json
import sys

template, target, bundle, certificate = sys.argv[1:5]
with open(template) as source:
    profile = json.load(source)
# A device checks the profile against the bundle it is asked to install, and
# against the certificate the hap was signed with. The template names an example
# application and carries the SDK's own certificate, so both are replaced.
profile["bundle-info"]["bundle-name"] = bundle
# The template expired in 2024 and names other people's devices. A profile that
# restricts installation to a device list cannot serve an emulator that reports
# no udid, so this one carries a release distribution instead, which is not
# device-bound.
# The template is a debug profile that expired in 2024. Its shape is what the
# device's parser expects, so only the dates change: a release profile needs a
# distribution type the parser accepts, and guessing at one costs more than
# leaving the template's own kind in place.
profile["validity"] = {"not-before": 1600000000, "not-after": 2500000000}
# The device's parser wants these named on a release profile.
profile["bundle-info"]["app-identifier"] = "maplibre-native-ffi-conformance"
profile.setdefault("app-privilege-capabilities", [])
profile.setdefault("acls", {"allowed-acls": [""]})
with open(certificate) as handle:
    chain = handle.read()
first = chain.index("-----BEGIN CERTIFICATE-----")
second = chain.index("-----BEGIN CERTIFICATE-----", first + 1)
# A release profile names the certificate that signs distributed builds, where
# a debug one names the development certificate.
leaf = chain[first:second]
profile["bundle-info"]["development-certificate"] = leaf
profile["bundle-info"]["distribution-certificate"] = leaf
with open(target, "w") as destination:
    json.dump(profile, destination)
PYTHON

(
  cd "$work"
  java -jar "$toolchains/lib/hap-sign-tool.jar" sign-profile \
  -keyAlias "openharmony application profile release" \
  -signAlg SHA256withECDSA \
  -mode localSign \
  -profileCertFile "$toolchains/lib/OpenHarmonyProfileRelease.pem" \
  -inFile "$manifest" \
  -keystoreFile "$toolchains/lib/OpenHarmony.p12" \
  -outFile "$profile" \
  -keyPwd 123456 \
  -keystorePwd 123456
)

echo "signing the hap"
signed="$work/$module.hap"
(
  cd "$work"
  java -jar "$toolchains/lib/hap-sign-tool.jar" sign-app \
  -keyAlias "openharmony application release" \
  -signAlg SHA256withECDSA \
  -mode localSign \
  -appCertFile "$app_cert" \
  -profileFile "$profile" \
  -inFile "$unsigned" \
  -keystoreFile "$toolchains/lib/OpenHarmony.p12" \
  -outFile "$signed" \
  -keyPwd 123456 \
  -keystorePwd 123456
)

echo "built $signed"

# Nothing above this point needs a device. What follows installs the application
# and starts the runner, which prints results the way every other runtime's
# runner does.
if [[ "${MLN_TS_ARKTS_INSTALL:-1}" == "0" ]]; then
  exit 0
fi

connect_key=127.0.0.1:55555
hdc="$toolchains/hdc"

echo "installing on $connect_key"
"$hdc" -t "$connect_key" install -r "$signed"

echo "running the conformance suite"
"$hdc" -t "$connect_key" shell aa test \
  -b "$bundle_id" \
  -m "$module" \
  -s unittest /ets/testrunner/OpenHarmonyTestRunner \
  -s timeout 300000
