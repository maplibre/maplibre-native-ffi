// The WebGPU demo: the map's session asks the browser for a device, and
// configures the canvas's WebGPU context with it.

import { run } from "./demo.ts";

// The DOM types that TypeScript ships leave out WebGPU.
interface Gpu {
  requestAdapter(): Promise<object | null>;
}

run({
  id: "webgpu",
  async check() {
    const gpu = (navigator as { gpu?: Gpu }).gpu;
    if (gpu === undefined) {
      throw new Error("This browser doesn't support WebGPU. Try WebGL.");
    }
    if ((await gpu.requestAdapter()) === null) {
      throw new Error("This browser found no WebGPU adapter. Try WebGL.");
    }
  },
});
