#!/usr/bin/env bash
# Builds and signs the example's HAP for one ABI from the binding's OHPM HARs.
#
#   build-hap.sh <abi> <facade HAR> <backend HAR>
#
# The HAP is build/<abi>/entry-default-signed.hap under the example.
#
# DevEco Studio builds apps with hvigor, which isn't published. This script
# calls the tools that hvigor calls, which all ship in the OpenHarmony SDK:
# ets-loader compiles the ArkUI pages with the SDK's own webpack configuration,
# es2abc merges them and the binding into modules.abc, restool compiles the
# resources, app_packing_tool packs the HAP, and hap-sign-tool signs it with the
# SDK's debug signing material.
set -euo pipefail

usage="usage: build-hap.sh <abi> <facade HAR> <backend HAR>"
abi=${1:?$usage}
facade_har=${2:?$usage}
backend_har=${3:?$usage}
example="$(cd "$(dirname "$0")/.." && pwd)"
sdk="$(dirname "${OHOS_SDK_NATIVE:?OHOS_SDK_NATIVE must name the OpenHarmony native SDK}")"
tools="$sdk/toolchains"
signing="$tools/lib"
loader="$sdk/ets/build-tools/ets-loader"
# The ArkTS component names its compiler directory for the host it targets.
case "$(uname -s)" in
  Darwin) ark=build-mac ;;
  *) ark=build ;;
esac
es2abc="$loader/bin/ark/$ark/bin/es2abc"
for tool in "$loader/webpack.config.js" "$es2abc" "$tools/restool" \
  "$signing/app_packing_tool.jar" "$signing/hap-sign-tool.jar"; do
  if [[ ! -e "$tool" ]]; then
    echo "error: the OpenHarmony SDK has no $tool; reinstall it with mise -E ohos install --force ohos-sdk" >&2
    exit 1
  fi
done

work="$example/build/$abi"
module="$work/entry/src/main"
rm -rf "$work"
mkdir -p "$work/libs/$abi" "$work/cache" "$work/compiled" "$work/pack"
cp -R "$example/entry" "$work/entry"

# Install both HARs in oh_modules, where ohpm puts them and ets-loader resolves
# them. The backend carries the native libraries.
for har in "$facade_har" "$backend_har"; do
  name=$(tar -xOzf "$har" package/oh-package.json5 | jq -er .name)
  mkdir -p "$work/entry/oh_modules/$name"
  tar -xzf "$har" -C "$work/entry/oh_modules/$name" --strip-components 1
done
facade="$work/entry/oh_modules/@maplibre/native-ffi"
backend="$work/entry/oh_modules/$(tar -xOzf "$backend_har" package/oh-package.json5 | jq -er .name)"
if [[ ! -f "$facade/oh-package.json5" || ! -d "$backend/libs/$abi" ]]; then
  echo "error: pass the @maplibre/native-ffi HAR, then a backend HAR with $abi libraries" >&2
  exit 1
fi
cp "$backend/libs/$abi/"*.so "$work/libs/$abi/"

# The app requires and targets the API floor that the facade declares.
api=$(jq -er .compatibleSdkVersion "$facade/oh-package.json5")
jq --argjson api "$api" '.app.minAPIVersion = $api | .app.targetAPIVersion = $api' \
  "$example/entry/src/main/module.json" >"$module/module.json"
bundle=$(jq -er .app.bundleName "$module/module.json")

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
    --config "$loader/webpack.config.js" \
    --env buildMode=debug --env compilerType=ark --env sourceMap=none
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

"$tools/restool" -i "$module" -o "$work/compiled" -p "$bundle" \
  -r "$work/compiled/ResourceTable.h" -f >/dev/null

# The packing tool empties its working directory when it exits, so it runs in
# a directory of its own.
(
  cd "$work/pack"
  java -jar "$signing/app_packing_tool.jar" --mode hap --force true \
    --json-path "$work/compiled/module.json" \
    --ets-path "$work/modules.abc" \
    --lib-path "$work/libs" \
    --resources-path "$work/compiled/resources" \
    --index-path "$work/compiled/resources.index" \
    --out-path "$work/entry-default-unsigned.hap" >/dev/null
)

# The SDK ships signing keys and an unsigned profile template but no
# application certificate. The SDK's application CA issues one for the
# release key, the template becomes this bundle's debug profile with no
# device list, and the profile key signs it.
password=123456
keystore="$signing/OpenHarmony.p12"
for alias in root:"openharmony application root ca" sub:"openharmony application ca"; do
  keytool -exportcert -rfc -alias "${alias#*:}" -keystore "$keystore" \
    -storetype PKCS12 -storepass "$password" >"$work/${alias%%:*}-ca.cer"
done
sign_tool=(java -jar "$signing/hap-sign-tool.jar")
"${sign_tool[@]}" generate-app-cert -keyAlias "openharmony application release" \
  -issuer "C=CN, O=OpenHarmony, OU=OpenHarmony Team, CN=OpenHarmony Application CA" \
  -issuerKeyAlias "openharmony application ca" \
  -subject "C=CN, O=OpenHarmony, OU=MapLibre, CN=MapLibre Native FFI example" \
  -signAlg SHA256withECDSA -keystoreFile "$keystore" -keystorePwd "$password" \
  -keyPwd "$password" -issuerKeyPwd "$password" -outForm certChain \
  -rootCaCertFile "$work/root-ca.cer" -subCaCertFile "$work/sub-ca.cer" \
  -outFile "$work/app.cer" >/dev/null
python3 - "$signing/UnsgnedDebugProfileTemplate.json" "$work/profile.json" \
  "$bundle" "$work/app.cer" <<'PYTHON'
import json
import sys

template, target, bundle, chain = sys.argv[1:]
with open(template) as source:
    profile = json.load(source)
with open(chain) as source:
    certificates = source.read()
marker = "-----BEGIN CERTIFICATE-----"
leaf = certificates[: certificates.index(marker, len(marker))]
profile["bundle-info"]["bundle-name"] = bundle
profile["bundle-info"]["development-certificate"] = leaf
# The template expired in 2024 and names two devices; a profile without a
# device list installs on the emulator.
profile["validity"] = {"not-before": 1600000000, "not-after": 2500000000}
profile.pop("debug-info", None)
with open(target, "w") as destination:
    json.dump(profile, destination)
PYTHON
"${sign_tool[@]}" sign-profile -keyAlias "openharmony application profile release" \
  -signAlg SHA256withECDSA -mode localSign \
  -profileCertFile "$signing/OpenHarmonyProfileRelease.pem" \
  -inFile "$work/profile.json" -outFile "$work/profile.p7b" \
  -keystoreFile "$keystore" -keystorePwd "$password" -keyPwd "$password" >/dev/null
"${sign_tool[@]}" sign-app -keyAlias "openharmony application release" \
  -signAlg SHA256withECDSA -mode localSign -appCertFile "$work/app.cer" \
  -profileFile "$work/profile.p7b" -inFile "$work/entry-default-unsigned.hap" \
  -outFile "$work/entry-default-signed.hap" \
  -keystoreFile "$keystore" -keystorePwd "$password" -keyPwd "$password" >/dev/null
echo "Signed $work/entry-default-signed.hap for $abi."
