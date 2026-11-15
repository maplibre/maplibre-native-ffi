// The WebGL demo: the map's session makes a WebGL 2 context on the canvas.

import { run } from "./demo.ts";

run({
  id: "webgl",
  async check() {
    if (new OffscreenCanvas(1, 1).getContext("webgl2") === null) {
      throw new Error("This browser doesn't support WebGL 2.");
    }
  },
});
