// Calls through the introspected API as a GJS program does: handle objects,
// boxed values, an event handler, a command, queries, readiness, and errors.

import Gio from "gi://Gio";
import GLib from "gi://GLib";
import MaplibreNativeFfi from "gi://MaplibreNativeFfi?version=0";
import System from "system";

const {
  CameraOptions,
  CameraUpdate,
  CameraUpdateMode,
  FeatureId,
  LatLng,
  LogicalExtent,
  Map,
  MapMode,
  MapOptions,
  Runtime,
  RuntimeEventFlag,
  RuntimeEventHandler,
  RuntimeEventType,
} = MaplibreNativeFfi;

for (const [type, methods] of [
  [Runtime, ["release", "set_event_handler", "clear_event_handler"]],
  [
    Map,
    [
      "ready",
      "release",
      "set_style_json",
      "update_camera",
      "get_camera",
      "list_style_layers",
    ],
  ],
]) {
  for (const method of methods) {
    Gio._promisify(type.prototype, `${method}_async`, `${method}_finish`);
  }
}

function check(condition, message) {
  if (!condition) throw new Error(message);
}

async function run() {
  const runtime = Runtime.create(null);
  const options = new MapOptions();
  options.set_initial_extent(
    new LogicalExtent({ width: 64, height: 64, scale_factor: 1 }),
  );
  options.set_map_mode(MapMode.CONTINUOUS);
  options.set_event_mask(RuntimeEventFlag.MAP_STYLE_LOADED);
  const map = runtime.create_map(options);
  await map.ready_async();

  // The handler receives each batch as an array of typed events.
  const loaded = new Promise((resolve) => {
    const handler = new RuntimeEventHandler();
    handler.set_callback((events) => {
      for (const event of events) {
        // An ID needs 64 bits, which a JavaScript number can't hold.
        if (
          map.is_event_source(event) &&
          event.get_event_type() === RuntimeEventType.MAP_STYLE_LOADED
        ) {
          resolve();
        }
      }
    });
    runtime.set_event_handler_async(handler);
  });
  await map.set_style_json_async(
    '{"version":8,"sources":{},"layers":[{"id":"land","type":"background"}]}',
  );
  await loaded;

  // GJS frees the array that a query returns, which owns its items.
  const layers = await map.list_style_layers_async();
  check(
    layers.length === 1 && layers[0].get_id() === "land",
    "the style's layers",
  );

  const camera = new CameraOptions();
  camera.set_zoom(4);
  camera.set_center(new LatLng({ latitude: 10, longitude: 20 }));
  const update = new CameraUpdate();
  update.set_mode(CameraUpdateMode.JUMP);
  update.set_camera(camera);
  const completion = await map.update_camera_async(update);
  check(typeof completion.generation === "number", "a command's generation");
  const found = await map.get_camera_async();
  check(found.camera.zoom === 4, `zoom ${found.camera.zoom}`);
  check(found.camera.center.longitude === 20, "the camera's center");

  const id = FeatureId.new_uint_value(7);
  check(id.is_uint_value() && id.get_uint_value() === 7, "a union variant");

  await runtime.clear_event_handler_async();
  await map.release_async();
  try {
    await map.release_async();
    throw new Error("a second release succeeded");
  } catch (error) {
    check(
      error.matches?.(
        MaplibreNativeFfi.Error,
        MaplibreNativeFfi.Error.INVALID_ARGUMENT,
      ),
      `a released map fails with ${String(error)}`,
    );
  }
  await runtime.release_async();
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
