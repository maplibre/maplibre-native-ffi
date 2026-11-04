// Opens the WebGPU demo where the browser has a WebGPU adapter, and JSPI to
// wait for its calls, and the WebGL demo elsewhere.

const openPage = (page: string) => location.replace(page + location.hash);

// The DOM types that TypeScript ships leave out WebGPU.
const gpu = (navigator as { gpu?: { requestAdapter(): Promise<unknown> } }).gpu;
if (gpu !== undefined && "Suspending" in WebAssembly) {
  gpu.requestAdapter().then(
    (adapter) => openPage(adapter === null ? "webgl/" : "webgpu/"),
    () => openPage("webgl/"),
  );
} else {
  openPage("webgl/");
}
