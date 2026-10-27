#!/usr/bin/env bash
# Builds and signs a test HAP from staged sources and libraries.
#
#   build-hap.sh <work-dir> <bundle-name> <abi> <api>
#
# <work-dir>/ets holds the ES module sources, and <work-dir>/libs/<abi> the
# native libraries. The HAP requires and targets API level <api>. DevEco Studio drives these steps through hvigor, which is
# not published; every tool that hvigor calls ships in the OpenHarmony SDK:
# es2abc compiles the sources into one merged modules.abc, restool compiles
# the resources, app_packing_tool packs the HAP, and hap-sign-tool signs it
# with the SDK's debug signing material. The HAP is <work-dir>/entry_test.hap.
set -euo pipefail

usage="usage: build-hap.sh <work-dir> <bundle-name> <abi> <api>"
work=${1:?$usage}
bundle=${2:?$usage}
abi=${3:?$usage}
api=${4:?$usage}
module=entry_test
sdk="$(dirname "${OHOS_SDK_NATIVE:?OHOS_SDK_NATIVE must name the OpenHarmony native SDK}")"
tools="$sdk/toolchains"
signing="$tools/lib"
# The ArkTS component names its compiler directory for the host it targets.
case "$(uname -s)" in
  Darwin) ark=build-mac ;;
  *) ark=build ;;
esac
es2abc="$sdk/ets/build-tools/ets-loader/bin/ark/$ark/bin/es2abc"
resources="$(cd "$(dirname "$0")/../test/arkts/resources" && pwd)"

for tool in "$es2abc" "$tools/restool" "$signing/app_packing_tool.jar" "$signing/hap-sign-tool.jar"; do
  if [[ ! -e "$tool" ]]; then
    echo "error: the OpenHarmony SDK has no $tool; reinstall it with mise -E ohos install --force ohos-sdk" >&2
    exit 1
  fi
done

# A source names a system module as @ohos.x and a packed library by its file
# name. Compiled records name them @ohos:x and @app:<bundle>/<module>/<name>,
# and a relative import names its record without an extension.
records="$work/records.txt"
: >"$records"
while IFS= read -r -d '' source; do
  relative=${source#"$work"/}
  BUNDLE="$bundle/$module" perl -pi -e '
    s#(["\x27])\@ohos\.#$1\@ohos:#g;
    s#(["\x27])lib(\w+)\.so(["\x27])#$1\@app:$ENV{BUNDLE}/$2$3#g;
    s#(["\x27])(\.\.?/[^"\x27]*)\.(?:ts|js|mjs)(["\x27])#$1$2$3#g;
  ' "$source"
  printf '%s;%s;esm;%s;%s\n' "$source" "$bundle/$module/${relative%.*}" \
    "$module/$relative" "$module" >>"$records"
done < <(find "$work/ets" -type f \( -name '*.ts' -o -name '*.js' -o -name '*.mjs' \) -print0)
"$es2abc" --merge-abc --module --extension=ts --output "$work/modules.abc" "@$records"

# The manifest names the test ability that the runner attaches to, and the
# network permission that the native HTTP client needs for the origin.
manifest="$work/module/module.json"
rm -rf "$work/module" "$work/compiled"
mkdir -p "$work/module" "$work/compiled"
cp -R "$resources" "$work/module/resources"
cat >"$manifest" <<JSON
{
  "app": {
    "bundleName": "$bundle",
    "vendor": "maplibre",
    "versionCode": 1,
    "versionName": "1.0.0",
    "minAPIVersion": $api,
    "targetAPIVersion": $api,
    "apiReleaseType": "Release",
    "debug": true,
    "bundleType": "app",
    "icon": "\$media:icon",
    "label": "\$string:label"
  },
  "module": {
    "name": "$module",
    "type": "entry",
    "description": "\$string:label",
    "mainElement": "TestAbility",
    "deviceTypes": ["default"],
    "deliveryWithInstall": true,
    "installationFree": false,
    "virtualMachine": "ark",
    "compileMode": "esmodule",
    "requestPermissions": [{ "name": "ohos.permission.INTERNET" }],
    "abilities": [
      {
        "name": "TestAbility",
        "srcEntry": "./ets/TestAbility.ts",
        "exported": true,
        "icon": "\$media:icon",
        "label": "\$string:label",
        "startWindowIcon": "\$media:icon",
        "startWindowBackground": "\$color:background"
      }
    ]
  }
}
JSON
"$tools/restool" -i "$work/module" -o "$work/compiled" -p "$bundle" \
  -r "$work/compiled/ResourceTable.h" -f >/dev/null

# The packing tool empties its working directory when it exits, so it runs in
# a directory of its own.
mkdir -p "$work/pack"
(
  cd "$work/pack"
  java -jar "$signing/app_packing_tool.jar" --mode hap --force true \
    --json-path "$work/compiled/module.json" \
    --ets-path "$work/modules.abc" \
    --lib-path "$work/libs" \
    --resources-path "$work/compiled/resources" \
    --index-path "$work/compiled/resources.index" \
    --out-path "$work/unsigned.hap" >/dev/null
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
  -subject "C=CN, O=OpenHarmony, OU=MapLibre, CN=MapLibre Native FFI tests" \
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
  -profileFile "$work/profile.p7b" -inFile "$work/unsigned.hap" \
  -outFile "$work/$module.hap" \
  -keystoreFile "$keystore" -keystorePwd "$password" -keyPwd "$password" >/dev/null
echo "Signed $work/$module.hap for $abi."
