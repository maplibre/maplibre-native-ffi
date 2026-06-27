export interface Viewport {
  width: number;
  height: number;
  scale: number;
}

interface EmscriptenWebGPU {
  importJsDevice(device: GPUDevice): number;
  getJsObject(pointer: number): GPUTexture;
}

export interface BrowserMapModule {
  webgpu: EmscriptenWebGPU;
  _mln_browser_map_init(
    logicalWidth: number,
    logicalHeight: number,
    scaleFactor: number,
    webgpuDevice: number,
    webgpuQueue: number,
  ): number;
  _mln_browser_map_render_frame(): number;
  _mln_browser_map_acquire_owned_texture(): number;
  _mln_browser_map_release_owned_texture_frame(): number;
  _mln_browser_map_resize(
    logicalWidth: number,
    logicalHeight: number,
    scaleFactor: number,
  ): number;
  _mln_browser_map_move_by(deltaX: number, deltaY: number): number;
  _mln_browser_map_scale_by(scale: number, x: number, y: number): number;
  _mln_browser_map_rotate_pitch_by(
    bearingDelta: number,
    pitchDelta: number,
  ): number;
  _mln_browser_map_rotate_by(bearingDelta: number): number;
  _mln_browser_map_pitch_by(pitchDelta: number): number;
  _mln_browser_map_reset_orientation(): number;
  _mln_browser_map_jump_to(
    longitude: number,
    latitude: number,
    zoom: number,
    bearing: number,
    pitch: number,
  ): number;
  _mln_browser_map_cancel_transitions(): number;
}

export interface BrowserMapBenchmarkReport {
  totalFrames: number;
  renderedFrames: number;
  durationMs: number;
  averageFps: number;
  p50Ms: number;
  p95Ms: number;
  p99Ms: number;
  maxMs: number;
  framesOver50Ms: number;
  framesOver100Ms: number;
  cities: string[];
}

export interface ModuleFactoryOptions {
  locateFile(path: string): string;
}

declare global {
  const GPUShaderStage: {
    readonly VERTEX: number;
    readonly FRAGMENT: number;
    readonly COMPUTE: number;
  };

  const GPUTextureUsage: {
    readonly COPY_SRC: number;
    readonly COPY_DST: number;
    readonly TEXTURE_BINDING: number;
    readonly STORAGE_BINDING: number;
    readonly RENDER_ATTACHMENT: number;
  };

  function createMapLibreModule(
    options: ModuleFactoryOptions,
  ): Promise<BrowserMapModule>;

  interface HTMLCanvasElement {
    getContext(contextId: "webgpu"): GPUCanvasContext | null;
  }

  interface Window {
    maplibreBrowserMap?: BrowserMapModule;
    maplibreBrowserMapBenchmark?: BrowserMapBenchmarkReport;
  }
}
