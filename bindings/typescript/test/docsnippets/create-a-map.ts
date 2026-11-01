// Snippets for the docs site's Create a map page. They are type-checked with
// the tests, which never run them.

import {
  type MapHandle,
  MapMode,
  type RenderSessionHandle,
  RuntimeHandle,
} from "../../src/index.ts";

// Attaches a render target, as the Rendering guides show.
declare function attachTarget(map: MapHandle): Promise<RenderSessionHandle>;

export async function createAMap(): Promise<void> {
  // #region runtime
  const runtime = await RuntimeHandle.create();
  // #endregion runtime

  // #region map
  const { map, ready } = runtime.createMap({
    initialExtent: { width: 800, height: 600, scaleFactor: 2 },
    mapMode: MapMode.continuous,
  });
  await ready;
  // #endregion map

  const session = await attachTarget(map);

  // #region close
  session.dispose();
  await map.close();
  await runtime.close();
  // #endregion close
}
