// The WebGL demo. The session draws with a WebGL 2 context on the page's
// thread whenever the page services it.

import {
  RenderDriverKind,
  registerWebGlContext,
  WebGlContextKind,
} from "@maplibre/native-ffi";

import { run } from "./demo.ts";

run({
  id: "webgl",
  async prepare(canvas) {
    const context = canvas.getContext("webgl2", {
      depth: true,
      stencil: true,
      antialias: true,
    });
    if (context === null) {
      throw new Error("This browser doesn't support WebGL 2.");
    }
    return (map, logicalSize, wakes) =>
      map.attachOpenGlSurface(
        {
          logicalSize,
          context: {
            data: {
              kind: "webGl",
              value: {
                kind: WebGlContextKind.existing,
                // The page never closes the registration, since the session
                // uses the context for as long as the page lives.
                context: registerWebGlContext(context).context,
              },
            },
          },
        },
        {
          driver: RenderDriverKind.callerGraphicsThread,
          frameWake: { callback: wakes.frame },
          driverWorkWake: { callback: wakes.driverWork },
        },
      );
  },
});
