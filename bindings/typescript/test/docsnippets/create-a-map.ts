// Snippets for the docs site's Create a map page. They are type-checked with
// the tests, which never run them.

import {
  CameraUpdateMode,
  type MapHandle,
  MapMode,
  type RenderSessionHandle,
  RuntimeHandle,
} from "../../src/index.ts";

function reportFailure(_error: unknown): void {}

// Attaches a render target, as the render target guides show.
declare function attachTarget(
  map: MapHandle,
): Promise<RenderSessionHandle>;

export async function createAMap(): Promise<void> {
  // #region runtime
  await using runtime = await RuntimeHandle.create();
  // #endregion runtime

  // #region map
  await using map = runtime.createMap({
    initialExtent: { width: 800, height: 600, scaleFactor: 2 },
    mapMode: MapMode.continuous,
  });
  await map.ready;
  // #endregion map

  // #region style
  map.discarding.setStyleUrl(
    "https://demotiles.maplibre.org/style.json",
  );
  // #endregion style

  const update = {
    mode: CameraUpdateMode.jump,
    camera: { zoom: 3 },
  };

  // #region wait
  await map.updateCamera(update);

  // Use the discarding variant to ignore the result.
  map.discarding.updateCamera(update);
  // #endregion wait

  // #region errors
  try {
    await map.updateCamera(update);
  } catch (error) {
    // Its message describes the failure.
    reportFailure(error);
  }
  // #endregion errors

  // #region close
  await using session = await attachTarget(map);
  // Leaving the scope releases the session, the map, and the runtime, in
  // that order, and waits for each.
  // #endregion close
}
