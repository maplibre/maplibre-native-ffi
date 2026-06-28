import { BrowserBenchmark } from "./benchmark";
import { InputController } from "./input";
import type { BrowserMapModule, Viewport } from "./types";
import { readViewport, setCanvasPhysicalSize } from "./viewport";
import { WebGPUDeviceHost, WebGPUTextureHost } from "./webgpu-host";

interface InitialCamera {
  longitude: number;
  latitude: number;
  zoom: number;
  bearing: number;
  pitch: number;
}

const defaultCamera: InitialCamera = {
  longitude: -122.4194,
  latitude: 37.7749,
  zoom: 13,
  bearing: 12,
  pitch: 30,
};

const crashLogStorageKey = "maplibre-browser-map-crash-log";
const maxCrashLogEntries = 200;

class BrowserMapShell {
  private module: BrowserMapModule | null = null;
  private webgpuHost: WebGPUDeviceHost | null = null;
  private textureHost: WebGPUTextureHost | null = null;
  private benchmark: BrowserBenchmark | null = null;
  private viewport: Viewport = { width: 960, height: 640, scale: 1 };
  private readonly input: InputController;

  constructor(private readonly canvas: HTMLCanvasElement) {
    this.input = new InputController(
      canvas,
      () => this.module,
      () => this.viewport,
    );
  }

  start(): void {
    this.input.attach();
    if (!("gpu" in navigator)) {
      throw new Error("WebGPU is not available in this browser");
    }
    loadScript("/browser-map.js")
      .then(() =>
        createMapLibreModule({
          locateFile: (path) => path,
          printErr: (message) => {
            const text = String(message);
            persistCrashLog(text);
            console.error(text);
          },
        }),
      )
      .then((module) => this.initialize(module))
      .catch((error: unknown) => {
        console.error(error);
      });
  }

  private async initialize(module: BrowserMapModule): Promise<void> {
    this.module = module;
    window.maplibreBrowserMap = module;
    module._mln_browser_map_set_trace(traceEnabled() ? 1 : 0);

    this.viewport = readViewport(this.canvas);
    setCanvasPhysicalSize(this.canvas, this.viewport);
    this.webgpuHost = await WebGPUDeviceHost.create(module);
    this.textureHost = new WebGPUTextureHost(
      module,
      this.canvas,
      this.webgpuHost,
      this.viewport,
    );

    const camera = readInitialCamera();
    const result = module._mln_browser_map_init(
      this.viewport.width,
      this.viewport.height,
      this.viewport.scale,
      this.webgpuHost.devicePtr,
      0,
    );
    this.subscribeResize();
    if (result === 0) {
      module._mln_browser_map_jump_to(
        camera.longitude,
        camera.latitude,
        camera.zoom,
        camera.bearing,
        camera.pitch,
      );
      this.benchmark = BrowserBenchmark.create(module, () => this.viewport);
      requestAnimationFrame((timestamp) => this.frame(timestamp));
    } else {
      console.error("Browser map initialization failed");
    }
  }

  private subscribeResize(): void {
    new ResizeObserver(() => this.syncViewport()).observe(this.canvas);
    window.addEventListener("resize", () => this.syncViewport());
    window
      .matchMedia(`(resolution: ${window.devicePixelRatio}dppx)`)
      .addEventListener("change", () => this.syncViewport(), { once: true });
  }

  private syncViewport(): void {
    if (!this.module || !this.textureHost) return;
    const nextViewport = readViewport(this.canvas);
    if (
      nextViewport.width === this.viewport.width &&
      nextViewport.height === this.viewport.height &&
      nextViewport.scale === this.viewport.scale
    ) {
      return;
    }
    if (this.textureHost.resize(nextViewport) !== 0) {
      console.error("Browser map resize failed");
      return;
    }
    this.viewport = nextViewport;
  }

  private frame(timestamp: DOMHighResTimeStamp): void {
    if (!this.module || !this.textureHost) return;
    this.input.applyPending();
    const nativeStart = performance.now();
    const frameResult = this.module._mln_browser_map_render_frame();
    const nativeMs = performance.now() - nativeStart;
    const runLoopMs = this.module._mln_browser_map_last_run_loop_ms();
    const runnableMs = this.module._mln_browser_map_last_runnable_ms();
    const readyRunnableCount =
      this.module._mln_browser_map_last_ready_runnable_count();
    const runnableCount = this.module._mln_browser_map_last_runnable_count();
    const eventDrainMs = this.module._mln_browser_map_last_event_drain_ms();
    const renderUpdateMs = this.module._mln_browser_map_last_render_update_ms();
    let presentMs = 0;
    if (frameResult === 1) {
      const texturePtr = this.module._mln_browser_map_acquire_owned_texture();
      if (texturePtr !== 0) {
        const presentStart = performance.now();
        this.textureHost.presentOwnedTexture(texturePtr);
        this.module._mln_browser_map_release_owned_texture_frame();
        presentMs = performance.now() - presentStart;
      }
    }
    this.benchmark?.recordFrame(
      timestamp,
      frameResult === 1,
      nativeMs,
      runLoopMs,
      runnableMs,
      readyRunnableCount,
      runnableCount,
      eventDrainMs,
      renderUpdateMs,
      presentMs,
    );
    requestAnimationFrame((nextTimestamp) => this.frame(nextTimestamp));
  }
}

function readInitialCamera(): InitialCamera {
  const params = new URLSearchParams(location.search);
  return {
    longitude: numberParam(params, "lon", defaultCamera.longitude),
    latitude: numberParam(params, "lat", defaultCamera.latitude),
    zoom: numberParam(params, "zoom", defaultCamera.zoom),
    bearing: numberParam(params, "bearing", defaultCamera.bearing),
    pitch: numberParam(params, "pitch", defaultCamera.pitch),
  };
}

function traceEnabled(): boolean {
  return new URLSearchParams(location.search).get("trace") === "1";
}

function numberParam(
  params: URLSearchParams,
  name: string,
  fallback: number,
): number {
  const rawValue = params.get(name);
  if (rawValue === null) return fallback;
  const value = Number(rawValue);
  return Number.isFinite(value) ? value : fallback;
}

function loadScript(source: string): Promise<void> {
  return new Promise((resolve, reject) => {
    const script = document.createElement("script");
    script.src = source;
    script.onload = () => resolve();
    script.onerror = () => reject(new Error(`failed to load ${source}`));
    document.head.append(script);
  });
}

function persistCrashLog(message: string): void {
  if (!isCrashLogMessage(message)) return;
  try {
    const entries = readCrashLog();
    entries.push({
      timestamp: new Date().toISOString(),
      url: location.href,
      message,
    });
    localStorage.setItem(
      crashLogStorageKey,
      JSON.stringify(entries.slice(-maxCrashLogEntries)),
    );
  } catch {
    // Diagnostic only; storage may be unavailable in some browser modes.
  }
}

function readCrashLog(): Array<{
  timestamp: string;
  url: string;
  message: string;
}> {
  const raw = localStorage.getItem(crashLogStorageKey);
  if (!raw) return [];
  const parsed = JSON.parse(raw);
  return Array.isArray(parsed) ? parsed : [];
}

function isCrashLogMessage(message: string): boolean {
  return (
    message.includes("std::terminate") ||
    message.includes("RuntimeError") ||
    message.includes("Aborted") ||
    message.includes("memory access") ||
    message.includes("unreachable") ||
    message.includes("WebGPU device lost") ||
    message.includes("WebGPU uncaptured error")
  );
}

window.addEventListener("error", (event) => {
  persistCrashLog(
    `${event.message}${event.error?.stack ? `\n${event.error.stack}` : ""}`,
  );
});

window.addEventListener("unhandledrejection", (event) => {
  persistCrashLog(String(event.reason));
});

const canvas = document.getElementById("canvas");
if (!(canvas instanceof HTMLCanvasElement)) {
  throw new Error("browser map canvas is missing");
}
new BrowserMapShell(canvas).start();
