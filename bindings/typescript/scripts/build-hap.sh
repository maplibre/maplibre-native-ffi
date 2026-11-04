#!/usr/bin/env bash
# Builds and signs a test HAP from staged sources and libraries.
#
#   build-hap.sh <work-dir> <bundle-name> <abi> <api>
#
# <work-dir>/ets holds the ES module sources, and <work-dir>/libs/<abi> the
# native libraries. The HAP requires and targets API level <api>. es2abc
# compiles the sources into one merged modules.abc, and tools/ohos/pack-hap
# packs and signs it with the libraries and the manifest. The HAP is
# <work-dir>/entry_test.hap.
set -euo pipefail

usage="usage: build-hap.sh <work-dir> <bundle-name> <abi> <api>"
work=${1:?$usage}
bundle=${2:?$usage}
abi=${3:?$usage}
api=${4:?$usage}
module=entry_test
sdk="$(dirname "${OHOS_SDK_NATIVE:?OHOS_SDK_NATIVE must name the OpenHarmony native SDK}")"
# The ArkTS component names its compiler directory for the host it targets.
case "$(uname -s)" in
  Darwin) ark=build-mac ;;
  *) ark=build ;;
esac
es2abc="$sdk/ets/build-tools/ets-loader/bin/ark/$ark/bin/es2abc"
resources="$(cd "$(dirname "$0")/../test/arkts/resources" && pwd)"

if [[ ! -e "$es2abc" ]]; then
  echo "error: the OpenHarmony SDK has no $es2abc; reinstall it with mise -E ohos install --force ohos-sdk" >&2
  exit 1
fi

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
rm -rf "$work/module"
mkdir -p "$work/module"
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
"$(dirname "$0")/../../../tools/ohos/pack-hap" "$work/module" "$work/modules.abc" \
  "$work/libs" "$work/$module.hap"
echo "Signed $work/$module.hap for $abi."
