// Creates a runtime and releases it, then releases it again and expects the
// library's error for a released handle, through the installed typelib.

import Gio from "gi://Gio";
import GLib from "gi://GLib";
import MaplibreNativeFfi from "gi://MaplibreNativeFfi?version=0";
import System from "system";

Gio._promisify(
  MaplibreNativeFfi.Runtime.prototype,
  "release_async",
  "release_finish",
);

async function run() {
  const options = new MaplibreNativeFfi.RuntimeOptions({ asset_path: "." });
  const runtime = MaplibreNativeFfi.Runtime.create(options);
  await runtime.release_async();
  try {
    await runtime.release_async();
  } catch (error) {
    // The domain's codes are the native statuses, which are negative.
    if (
      !error.matches?.(
        MaplibreNativeFfi.Error,
        MaplibreNativeFfi.Error.INVALID_ARGUMENT,
      ) ||
      error.code >= 0
    ) {
      throw error;
    }
    // A runtime that was never released tears down when disposed.
    MaplibreNativeFfi.Runtime.create(null).dispose();
    return;
  }
  throw new Error("a second release succeeded");
}

const loop = new GLib.MainLoop(null, false);
let status = 1;
run()
  .then(
    () => {
      status = 0;
    },
    (error) => logError(error),
  )
  .finally(() => loop.quit());
loop.run();
System.exit(status);
