#!/usr/bin/env bash
# Runs the suite as a page in headless Chromium, for a web preset whose browser
# module the binding's build task built.
#
#   test-browser.sh <preset>
#
# The page loads the test module, whose addon exports the suite's native probe
# beside the addon's own, and the suite bundled by test/browser/vite.config.ts.
# Everything runs on the page's thread, and the cases await what the module
# reports.
set -euo pipefail

preset=${1:?usage: test-browser.sh <web-preset>}
cd "$(dirname "$0")/.."
build="$PWD/native/build/$preset"

cmake --build "$build" --target maplibre_native_ffi_test

page="$(mktemp -d)"
trap 'rm -rf "$page"' EXIT
cp -R "$build/test-package/." "$page/"
(cd test/browser && MAPLIBRE_NATIVE_FFI_BROWSER_BUNDLE="$page" pnpm exec vp pack)
cat >"$page/index.html" <<'HTML'
<!DOCTYPE html>
<html lang="en">
  <head><meta charset="utf-8" /><title>TypeScript binding suite</title></head>
  <body>
    <script>
      const report = (output) =>
        void fetch("/__result", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ status: 70, output }),
        });
      addEventListener("error", (event) => report("uncaught error: " + event.message));
    </script>
    <script type="module" src="runner.js"></script>
  </body>
</html>
HTML

case "$preset" in
  *-webgpu) backend=webgpu ;;
  *) backend=opengl ;;
esac
# A reachability case forces garbage collection, which V8 exposes behind a
# flag.
node "$MISE_MONOREPO_ROOT/scripts/run-browser-test.mjs" "$page/index.html" \
  --render-backend "$backend" --browser-arg --js-flags=--expose-gc \
  --timeout-seconds 900
