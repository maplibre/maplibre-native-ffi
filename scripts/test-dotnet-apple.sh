#!/usr/bin/env bash
# Builds an Apple package consumer and runs its portable binding suite where supported.
set -euo pipefail
preset=${1:?usage: test-dotnet-apple.sh <preset> [--build-only]}
case "$preset" in
  ios-simulator-*) platform=ios; rid=iossimulator-arm64 ;;
  ios-maccatalyst-*) platform=maccatalyst; rid=maccatalyst-arm64 ;;
  ios-arm64-*) platform=ios; rid=ios-arm64 ;;
  tvos-simulator-*) platform=tvos; rid=tvossimulator-arm64 ;;
  tvos-arm64-*) platform=tvos; rid=tvos-arm64 ;;
  *) echo "The .NET Apple suite cannot build $preset." >&2; exit 2 ;;
esac

# These Apple SDKs target Xcode 26.4. Keep selection local to the consumer test.
dotnet workload install "$platform" --from-rollback-file bindings/dotnet/WorkloadRollback.json --verbosity quiet
for developer_dir in "${DEVELOPER_DIR:-$(xcode-select -p)}" /Applications/Xcode*.app/Contents/Developer; do
  if [[ $(DEVELOPER_DIR="$developer_dir" xcodebuild -version 2>/dev/null | head -1) == "Xcode 26.4"* ]]; then
    export DEVELOPER_DIR="$developer_dir"
    break
  fi
done
python tools/release/dotnet_release.py prepare-apple "$preset"
work="$MISE_MONOREPO_ROOT/build/$preset/dotnet-apple-test"
project=bindings/dotnet/tests/Maplibre.NativeFfi.AppleTest/Maplibre.NativeFfi.AppleTest.csproj
dotnet build "$project" -c Release -r "$rid" \
  "-p:MaplibreApplePlatform=$platform" \
  "-p:MaplibreNativeCInstallDir=$MISE_MONOREPO_ROOT/build/$preset/install" \
  "-p:MaplibreNativeFfiPackageVersion=0.0.0-apple-ci" \
  "-p:BaseOutputPath=$work/app/" \
  "-p:BaseIntermediateOutputPath=$work/obj/" \
  "-p:RestorePackagesPath=$work/restore" \
  "-p:RestoreAdditionalProjectSources=$work/packages"
if [[ ${2:-} == --build-only ]]; then
  exit 0
fi
app="$work/app/Release/net10.0-$platform/$rid/Maplibre.NativeFfi.Tests.app"
if [[ $platform == maccatalyst ]]; then
  exec perl -e 'alarm shift; exec @ARGV' 300 "$app/Contents/MacOS/Maplibre.NativeFfi.Tests"
fi
if [[ $rid != *simulator* ]]; then
  echo "Run the .NET Apple suite with a simulator or Mac Catalyst preset." >&2
  exit 2
fi
export MLN_FFI_SIMULATOR_RUNTIME=$platform
exec "$MISE_MONOREPO_ROOT/scripts/run-ios-simulator-test.sh" "$app/Maplibre.NativeFfi.Tests" 300
