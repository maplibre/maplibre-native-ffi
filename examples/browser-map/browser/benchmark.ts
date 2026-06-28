import type {
  BrowserMapBenchmarkReport,
  BrowserMapMemorySample,
  BrowserMapModule,
  BrowserMapSlowFrame,
  Viewport,
} from "./types";

interface BenchmarkCamera {
  name: string;
  longitude: number;
  latitude: number;
  zoom: number;
  bearing: number;
  pitch: number;
}

const benchmarkDurationMs = 45_500;
const cityIntervalMs = 3_500;
const cityFlyDurationMs = 2_500;
const slowFrameThresholdMs = 50;
const maxSlowFrameSamples = 20;
const memorySampleIntervalMs = 1_000;
const memorySamplesStorageKey = "maplibre-browser-map-memory-samples";
const maxStoredMemorySamples = 500;

const benchmarkCameras: BenchmarkCamera[] = [
  {
    name: "San Francisco",
    longitude: -122.4194,
    latitude: 37.7749,
    zoom: 14.5,
    bearing: 25,
    pitch: 45,
  },
  {
    name: "New York",
    longitude: -73.9857,
    latitude: 40.7484,
    zoom: 15,
    bearing: -20,
    pitch: 50,
  },
  {
    name: "London",
    longitude: -0.1276,
    latitude: 51.5072,
    zoom: 14.8,
    bearing: 35,
    pitch: 45,
  },
  {
    name: "Tokyo",
    longitude: 139.7671,
    latitude: 35.6812,
    zoom: 15,
    bearing: -35,
    pitch: 52,
  },
  {
    name: "Paris",
    longitude: 2.3522,
    latitude: 48.8566,
    zoom: 14.7,
    bearing: 15,
    pitch: 48,
  },
  {
    name: "Sydney",
    longitude: 151.2093,
    latitude: -33.8688,
    zoom: 14.4,
    bearing: -25,
    pitch: 42,
  },
];

export class BrowserBenchmark {
  private warmupTimestamp = 0;
  private startTimestamp = 0;
  private previousTimestamp = 0;
  private nextCityTimestamp = 0;
  private cityFlightEndsAt = 0;
  private nextMemorySampleTimestamp = 0;
  private cityIndex = 0;
  private renderedFrames = 0;
  private complete = false;
  private currentCity = "starting";
  private readonly frameDeltas: number[] = [];
  private readonly nativeFrameTimes: number[] = [];
  private readonly runLoopTimes: number[] = [];
  private readonly runnableTimes: number[] = [];
  private readonly readyRunnableCounts: number[] = [];
  private readonly runnableCounts: number[] = [];
  private readonly eventDrainTimes: number[] = [];
  private readonly renderUpdateTimes: number[] = [];
  private readonly presentFrameTimes: number[] = [];
  private readonly slowFrames: BrowserMapSlowFrame[] = [];
  private readonly memorySamples: BrowserMapMemorySample[] = [];

  static create(
    module: BrowserMapModule,
    getViewport: () => Viewport,
  ): BrowserBenchmark | null {
    if (new URLSearchParams(location.search).get("benchmark") !== "1") {
      return null;
    }
    return new BrowserBenchmark(module, getViewport);
  }

  private constructor(
    private readonly module: BrowserMapModule,
    private readonly getViewport: () => Viewport,
  ) {
    clearStoredMemorySamples();
  }

  recordFrame(
    timestamp: DOMHighResTimeStamp,
    rendered: boolean,
    nativeMs: number,
    runLoopMs: number,
    runnableMs: number,
    readyRunnableCount: number,
    runnableCount: number,
    eventDrainMs: number,
    renderUpdateMs: number,
    presentMs: number,
  ): void {
    if (this.complete) return;
    if (this.startTimestamp === 0) {
      this.recordWarmupFrame(timestamp);
      return;
    }

    const deltaMs = timestamp - this.previousTimestamp;
    this.frameDeltas.push(deltaMs);
    if (
      deltaMs > slowFrameThresholdMs &&
      this.slowFrames.length < maxSlowFrameSamples
    ) {
      this.slowFrames.push({
        elapsedMs: round(timestamp - this.startTimestamp),
        deltaMs: round(deltaMs),
        nativeMs: round(nativeMs),
        runLoopMs: round(runLoopMs),
        runnableMs: round(runnableMs),
        readyRunnableCount,
        runnableCount,
        eventDrainMs: round(eventDrainMs),
        renderUpdateMs: round(renderUpdateMs),
        presentMs: round(presentMs),
        rendered,
        city: this.currentCity,
      });
    }
    this.previousTimestamp = timestamp;
    this.nativeFrameTimes.push(nativeMs);
    this.runLoopTimes.push(runLoopMs);
    this.runnableTimes.push(runnableMs);
    this.readyRunnableCounts.push(readyRunnableCount);
    this.runnableCounts.push(runnableCount);
    this.eventDrainTimes.push(eventDrainMs);
    this.renderUpdateTimes.push(renderUpdateMs);
    this.presentFrameTimes.push(presentMs);
    if (rendered) {
      this.renderedFrames += 1;
    }

    const elapsedMs = timestamp - this.startTimestamp;
    if (timestamp >= this.nextMemorySampleTimestamp) {
      this.recordMemorySample(timestamp, "interval");
      this.nextMemorySampleTimestamp = timestamp + memorySampleIntervalMs;
    }
    if (elapsedMs >= benchmarkDurationMs) {
      this.finish(elapsedMs);
      return;
    }

    if (timestamp >= this.nextCityTimestamp) {
      this.flyToNextCity(timestamp);
      this.nextCityTimestamp += cityIntervalMs;
    }
    if (timestamp >= this.cityFlightEndsAt) {
      this.driveCamera(elapsedMs / 1000);
    }
  }

  private recordWarmupFrame(timestamp: DOMHighResTimeStamp): void {
    if (this.warmupTimestamp === 0) {
      this.warmupTimestamp = timestamp;
      console.info("browser-map benchmark waiting for initial map load");
    }
    if (this.module._mln_browser_map_is_fully_loaded() === 0) {
      return;
    }
    if (this.startTimestamp === 0) {
      this.startTimestamp = timestamp;
      this.previousTimestamp = timestamp;
      this.nextCityTimestamp = timestamp + cityIntervalMs;
      this.nextMemorySampleTimestamp = timestamp;
      console.info(
        `browser-map benchmark started after ${round(timestamp - this.warmupTimestamp)}ms warmup`,
      );
      this.flyToNextCity(timestamp);
    }
  }

  private flyToNextCity(timestamp: DOMHighResTimeStamp): void {
    const camera = benchmarkCameras[this.cityIndex % benchmarkCameras.length];
    this.cityIndex += 1;
    this.currentCity = camera.name;
    this.cityFlightEndsAt = timestamp + cityFlyDurationMs;
    this.recordMemorySample(timestamp, `before ${camera.name}`);
    this.module._mln_browser_map_fly_to(
      camera.longitude,
      camera.latitude,
      camera.zoom,
      camera.bearing,
      camera.pitch,
    );
    console.info(`browser-map benchmark city: ${camera.name}`);
    this.recordMemorySample(timestamp, `after ${camera.name}`);
  }

  private driveCamera(elapsedSeconds: number): void {
    const viewport = this.getViewport();
    const centerX = viewport.width / 2;
    const centerY = viewport.height / 2;
    this.module._mln_browser_map_move_by(
      Math.sin(elapsedSeconds * 2.1) * 3.0,
      Math.cos(elapsedSeconds * 1.7) * 2.0,
    );
    this.module._mln_browser_map_scale_by(
      Math.pow(2, Math.sin(elapsedSeconds * 2.7) * 0.006),
      centerX,
      centerY,
    );
    this.module._mln_browser_map_rotate_pitch_by(
      Math.sin(elapsedSeconds * 1.3) * 0.3,
      Math.cos(elapsedSeconds * 1.1) * 0.15,
    );
  }

  private finish(durationMs: number): void {
    this.complete = true;
    this.recordMemorySample(this.startTimestamp + durationMs, "finish");
    const report = buildReport(
      this.frameDeltas,
      this.nativeFrameTimes,
      this.runLoopTimes,
      this.runnableTimes,
      this.readyRunnableCounts,
      this.runnableCounts,
      this.eventDrainTimes,
      this.renderUpdateTimes,
      this.presentFrameTimes,
      this.slowFrames,
      this.memorySamples,
      this.renderedFrames,
      durationMs,
      this.module,
    );
    window.maplibreBrowserMapBenchmark = report;
    console.info(`browser-map benchmark result: ${JSON.stringify(report)}`);
  }

  private recordMemorySample(
    timestamp: DOMHighResTimeStamp,
    reason: string,
  ): void {
    const sample = sampleMemory(
      this.module,
      timestamp - this.startTimestamp,
      reason,
      this.currentCity,
    );
    this.memorySamples.push(sample);
    storeMemorySample(sample);
    console.info(`browser-map memory sample: ${JSON.stringify(sample)}`);
  }
}

function clearStoredMemorySamples(): void {
  try {
    window.localStorage.removeItem(memorySamplesStorageKey);
  } catch {
    // Diagnostic only; storage may be unavailable in some browser modes.
  }
}

function storeMemorySample(sample: BrowserMapMemorySample): void {
  try {
    const raw = window.localStorage.getItem(memorySamplesStorageKey);
    const existing: unknown = raw ? JSON.parse(raw) : [];
    const samples = Array.isArray(existing) ? existing : [];
    samples.push(sample);
    window.localStorage.setItem(
      memorySamplesStorageKey,
      JSON.stringify(samples.slice(-maxStoredMemorySamples)),
    );
  } catch {
    // Diagnostic only; storage may be unavailable in some browser modes.
  }
}

function buildReport(
  frameDeltas: number[],
  nativeFrameTimes: number[],
  runLoopTimes: number[],
  runnableTimes: number[],
  readyRunnableCounts: number[],
  runnableCounts: number[],
  eventDrainTimes: number[],
  renderUpdateTimes: number[],
  presentFrameTimes: number[],
  slowFrames: BrowserMapSlowFrame[],
  memorySamples: BrowserMapMemorySample[],
  renderedFrames: number,
  durationMs: number,
  module: BrowserMapModule,
): BrowserMapBenchmarkReport {
  const sorted = [...frameDeltas].sort((left, right) => left - right);
  const sortedNative = [...nativeFrameTimes].sort(
    (left, right) => left - right,
  );
  const sortedRunLoop = [...runLoopTimes].sort((left, right) => left - right);
  const sortedRunnable = [...runnableTimes].sort((left, right) => left - right);
  const sortedReadyRunnableCounts = [...readyRunnableCounts].sort(
    (left, right) => left - right,
  );
  const sortedRunnableCounts = [...runnableCounts].sort(
    (left, right) => left - right,
  );
  const sortedEventDrain = [...eventDrainTimes].sort(
    (left, right) => left - right,
  );
  const sortedRenderUpdate = [...renderUpdateTimes].sort(
    (left, right) => left - right,
  );
  const sortedPresent = [...presentFrameTimes].sort(
    (left, right) => left - right,
  );
  const totalFrames = frameDeltas.length;
  const averageMs =
    totalFrames === 0
      ? 0
      : frameDeltas.reduce((sum, value) => sum + value, 0) / totalFrames;
  return {
    totalFrames,
    renderedFrames,
    durationMs: round(durationMs),
    averageFps: averageMs === 0 ? 0 : round(1000 / averageMs),
    p50Ms: percentile(sorted, 0.5),
    p95Ms: percentile(sorted, 0.95),
    p99Ms: percentile(sorted, 0.99),
    maxMs: round(sorted.length === 0 ? 0 : sorted[sorted.length - 1]),
    framesOver50Ms: frameDeltas.filter((delta) => delta > 50).length,
    framesOver100Ms: frameDeltas.filter((delta) => delta > 100).length,
    nativeP95Ms: percentile(sortedNative, 0.95),
    nativeP99Ms: percentile(sortedNative, 0.99),
    nativeMaxMs: round(
      sortedNative.length === 0 ? 0 : sortedNative[sortedNative.length - 1],
    ),
    runLoopP95Ms: percentile(sortedRunLoop, 0.95),
    runLoopP99Ms: percentile(sortedRunLoop, 0.99),
    runLoopMaxMs: max(sortedRunLoop),
    runnableP95Ms: percentile(sortedRunnable, 0.95),
    runnableP99Ms: percentile(sortedRunnable, 0.99),
    runnableMaxMs: max(sortedRunnable),
    readyRunnableMax: max(sortedReadyRunnableCounts),
    runnableMax: max(sortedRunnableCounts),
    eventDrainP95Ms: percentile(sortedEventDrain, 0.95),
    eventDrainP99Ms: percentile(sortedEventDrain, 0.99),
    eventDrainMaxMs: max(sortedEventDrain),
    renderUpdateP95Ms: percentile(sortedRenderUpdate, 0.95),
    renderUpdateP99Ms: percentile(sortedRenderUpdate, 0.99),
    renderUpdateMaxMs: max(sortedRenderUpdate),
    presentP95Ms: percentile(sortedPresent, 0.95),
    presentP99Ms: percentile(sortedPresent, 0.99),
    presentMaxMs: max(sortedPresent),
    heapSizeBytes: module._mln_browser_map_heap_size(),
    heapMaxBytes: module._mln_browser_map_heap_max(),
    memorySamples,
    cities: benchmarkCameras.map((camera) => camera.name),
    slowFrames,
  };
}

function sampleMemory(
  module: BrowserMapModule,
  elapsedMs: number,
  reason: string,
  city: string,
): BrowserMapMemorySample {
  return {
    elapsedMs: round(elapsedMs),
    reason,
    city,
    heapSizeBytes: module._mln_browser_map_heap_size(),
    heapMaxBytes: module._mln_browser_map_heap_max(),
    mallocArenaBytes: module._mln_browser_map_malloc_arena(),
    mallocAllocatedBytes: module._mln_browser_map_malloc_allocated(),
    mallocFreeBytes: module._mln_browser_map_malloc_free(),
    mallocKeepcostBytes: module._mln_browser_map_malloc_keepcost(),
  };
}

function percentile(sorted: number[], fraction: number): number {
  if (sorted.length === 0) return 0;
  const index = Math.min(
    sorted.length - 1,
    Math.floor(sorted.length * fraction),
  );
  return round(sorted[index]);
}

function max(sorted: number[]): number {
  return round(sorted.length === 0 ? 0 : sorted[sorted.length - 1]);
}

function round(value: number): number {
  return Math.round(value * 10) / 10;
}
