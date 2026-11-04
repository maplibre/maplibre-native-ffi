// The WebGPU demo. The session draws on the page's thread, with the page's
// GPUDevice, whenever the page services it.

import {
  importWebGpuDevice,
  importWebGpuSurface,
  RenderDriverKind,
  webGpuTextureFormat,
} from "@maplibre/native-ffi";

import { run } from "./demo.ts";

// The DOM types that TypeScript ships leave out WebGPU.
interface Gpu {
  requestAdapter(): Promise<{ requestDevice(): Promise<object> } | null>;
  getPreferredCanvasFormat(): string;
}

run({
  id: "webgpu",
  async prepare(canvas) {
    const gpu = (navigator as { gpu?: Gpu }).gpu;
    if (gpu === undefined) {
      throw new Error("This browser doesn't support WebGPU. Try WebGL.");
    }
    // The library waits for WebGPU's asynchronous calls through JSPI.
    if (!("Suspending" in WebAssembly)) {
      throw new Error(
        "This browser doesn't support JSPI, which the WebGPU backend needs. Try WebGL.",
      );
    }
    const adapter = await gpu.requestAdapter();
    if (adapter === null) {
      throw new Error("This browser found no WebGPU adapter. Try WebGL.");
    }
    const device = await adapter.requestDevice();
    const context = canvas.getContext("webgpu");
    if (context === null) {
      throw new Error("The canvas refused a WebGPU context. Try WebGL.");
    }
    const format = gpu.getPreferredCanvasFormat();

    return (map, logicalSize, wakes) =>
      map.attachWebGpuSurface(
        {
          logicalSize,
          // The page never closes these imports, since the session uses them
          // for as long as the page lives.
          context: { device: importWebGpuDevice(device).pointer },
          surface: importWebGpuSurface(context).pointer,
          format: webGpuTextureFormat(format),
        },
        {
          driver: RenderDriverKind.callerGraphicsThread,
          frameWake: { callback: wakes.frame },
          driverWorkWake: { callback: wakes.driverWork },
        },
      );
  },
});
