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
  _mln_browser_map_set_trace(enabled: number): void;
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
  _mln_browser_map_move_by_animated(deltaX: number, deltaY: number): number;
  _mln_browser_map_scale_by(scale: number, x: number, y: number): number;
  _mln_browser_map_scale_by_animated(
    scale: number,
    x: number,
    y: number,
  ): number;
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
  _mln_browser_map_fly_to(
    longitude: number,
    latitude: number,
    zoom: number,
    bearing: number,
    pitch: number,
  ): number;
  _mln_browser_map_is_fully_loaded(): number;
  _mln_browser_map_last_run_loop_ms(): number;
  _mln_browser_map_last_runnable_ms(): number;
  _mln_browser_map_last_event_drain_ms(): number;
  _mln_browser_map_last_render_update_ms(): number;
  _mln_browser_map_last_ready_runnable_count(): number;
  _mln_browser_map_last_runnable_count(): number;
  _mln_browser_map_heap_size(): number;
  _mln_browser_map_heap_max(): number;
  _mln_browser_map_malloc_arena(): number;
  _mln_browser_map_malloc_allocated(): number;
  _mln_browser_map_malloc_free(): number;
  _mln_browser_map_malloc_keepcost(): number;
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
  nativeP95Ms: number;
  nativeP99Ms: number;
  nativeMaxMs: number;
  runLoopP95Ms: number;
  runLoopP99Ms: number;
  runLoopMaxMs: number;
  runnableP95Ms: number;
  runnableP99Ms: number;
  runnableMaxMs: number;
  readyRunnableMax: number;
  runnableMax: number;
  eventDrainP95Ms: number;
  eventDrainP99Ms: number;
  eventDrainMaxMs: number;
  renderUpdateP95Ms: number;
  renderUpdateP99Ms: number;
  renderUpdateMaxMs: number;
  presentP95Ms: number;
  presentP99Ms: number;
  presentMaxMs: number;
  heapSizeBytes: number;
  heapMaxBytes: number;
  memorySamples: BrowserMapMemorySample[];
  cities: string[];
  slowFrames: BrowserMapSlowFrame[];
}

export interface BrowserMapMemorySample {
  elapsedMs: number;
  reason: string;
  city: string;
  heapSizeBytes: number;
  heapMaxBytes: number;
  mallocArenaBytes: number;
  mallocAllocatedBytes: number;
  mallocFreeBytes: number;
  mallocKeepcostBytes: number;
}

export interface BrowserMapSlowFrame {
  elapsedMs: number;
  deltaMs: number;
  nativeMs: number;
  runLoopMs: number;
  runnableMs: number;
  readyRunnableCount: number;
  runnableCount: number;
  eventDrainMs: number;
  renderUpdateMs: number;
  presentMs: number;
  rendered: boolean;
  city: string;
}

export interface ModuleFactoryOptions {
  locateFile(path: string): string;
  printErr?(message: unknown): void;
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
