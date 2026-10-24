// The WebGPU imports of the Kotlin/Wasm binding's WebGraphics, which the
// module of a WebGPU build links and exports as Module.mlnKwWebgpu.
//
// emdawnwebgpu keeps every WebGPU object of the module in its own table, so
// only the module can turn a page's GPUDevice or GPUCanvasContext into the
// handle that a session descriptor takes. An import is a copy: the handle holds
// one reference, and its release leaves the JavaScript object usable.
//
// Each import passes no parent. Sessions use spontaneous callbacks, which an
// object without an instance still receives.
addToLibrary({
  $mlnKwWebgpu__deps: [
    "$WebGPU",
    "wgpuDeviceRelease",
    "wgpuSurfaceRelease",
    "wgpuTextureRelease",
    "wgpuTextureViewRelease",
  ],
  $mlnKwWebgpu: {
    // Each import returns a handle that holds one reference, or 0 when the
    // object is not of its kind in this realm.
    importDevice: (device) =>
      globalThis.GPUDevice && device instanceof GPUDevice
        ? WebGPU.importJsDevice(device)
        : 0,
    importSurface: (context) =>
      globalThis.GPUCanvasContext && context instanceof GPUCanvasContext
        ? WebGPU.importJsSurface(context)
        : 0,
    importTexture: (texture) =>
      globalThis.GPUTexture && texture instanceof GPUTexture
        ? WebGPU.importJsTexture(texture)
        : 0,
    importTextureView: (view) =>
      globalThis.GPUTextureView && view instanceof GPUTextureView
        ? WebGPU.importJsTextureView(view)
        : 0,

    releaseDevice: (handle) => _wgpuDeviceRelease(handle),
    releaseSurface: (handle) => _wgpuSurfaceRelease(handle),
    releaseTexture: (handle) => _wgpuTextureRelease(handle),
    releaseTextureView: (handle) => _wgpuTextureViewRelease(handle),

    // The JavaScript object behind [handle], or null when it names none.
    object: (handle) => WebGPU.getJsObject(handle) ?? null,

    // The WGPUTextureFormat value of a GPUTextureFormat [name], or -1 when the
    // module knows no such format.
    textureFormat: (name) => WebGPU.TextureFormat.indexOf(name),
  },
});
