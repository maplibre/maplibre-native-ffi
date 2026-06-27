import type {
  BrowserMapBenchmarkReport,
  BrowserMapModule,
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

const benchmarkDurationMs = 30_000;
const cityIntervalMs = 3_500;

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
  private startTimestamp = 0;
  private previousTimestamp = 0;
  private nextCityTimestamp = 0;
  private cityIndex = 0;
  private renderedFrames = 0;
  private complete = false;
  private readonly frameDeltas: number[] = [];

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
  ) {}

  recordFrame(timestamp: DOMHighResTimeStamp, rendered: boolean): void {
    if (this.complete) return;
    if (this.startTimestamp === 0) {
      this.startTimestamp = timestamp;
      this.previousTimestamp = timestamp;
      this.nextCityTimestamp = timestamp;
      console.info("browser-map benchmark started");
    } else {
      this.frameDeltas.push(timestamp - this.previousTimestamp);
      this.previousTimestamp = timestamp;
    }
    if (rendered) {
      this.renderedFrames += 1;
    }

    const elapsedMs = timestamp - this.startTimestamp;
    if (elapsedMs >= benchmarkDurationMs) {
      this.finish(elapsedMs);
      return;
    }

    if (timestamp >= this.nextCityTimestamp) {
      this.jumpToNextCity();
      this.nextCityTimestamp += cityIntervalMs;
    }
    this.driveCamera(elapsedMs / 1000);
  }

  private jumpToNextCity(): void {
    const camera = benchmarkCameras[this.cityIndex % benchmarkCameras.length];
    this.cityIndex += 1;
    this.module._mln_browser_map_cancel_transitions();
    this.module._mln_browser_map_jump_to(
      camera.longitude,
      camera.latitude,
      camera.zoom,
      camera.bearing,
      camera.pitch,
    );
    console.info(`browser-map benchmark city: ${camera.name}`);
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
    const report = buildReport(
      this.frameDeltas,
      this.renderedFrames,
      durationMs,
    );
    window.maplibreBrowserMapBenchmark = report;
    console.info(`browser-map benchmark result: ${JSON.stringify(report)}`);
  }
}

function buildReport(
  frameDeltas: number[],
  renderedFrames: number,
  durationMs: number,
): BrowserMapBenchmarkReport {
  const sorted = [...frameDeltas].sort((left, right) => left - right);
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
    cities: benchmarkCameras.map((camera) => camera.name),
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

function round(value: number): number {
  return Math.round(value * 10) / 10;
}
