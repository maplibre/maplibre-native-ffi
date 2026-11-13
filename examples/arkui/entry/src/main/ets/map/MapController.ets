// A continuous map on an XComponent's surface. The component reports its
// surface when the platform creates one, and the controller attaches a session
// to it, or gives an attached session the new surface. Everything else matches
// the browser example in examples/browser.

import {
  CameraUpdateMode,
  FramePacerHandle,
  LogicalSize,
  MapHandle,
  MapMode,
  ohosFramePacerCreate,
  RenderDriverKind,
  RenderResult,
  RenderSessionHandle,
  RuntimeEvent,
  RuntimeEventFlag,
  RuntimeEventType,
  RuntimeHandle,
} from '@maplibre/native-ffi';

const styleUrl = 'https://tiles.openfreemap.org/styles/liberty';

export interface Status {
  message: string;
  error: boolean;
}

export class MapController {
  private readonly onStatus: (status: Status) => void;
  private readonly scaleFactor: number;
  private failed = false;
  private disposed = false;

  private runtime: RuntimeHandle | undefined = undefined;
  private map: MapHandle | undefined = undefined;
  private pacer: FramePacerHandle | undefined = undefined;
  private session: RenderSessionHandle | undefined = undefined;

  // The component's latest size, its latest surface, and the surface that the
  // session renders to. A surface ID is the one the component reports.
  private size: LogicalSize | undefined = undefined;
  private surface: bigint | undefined = undefined;
  private target: bigint | undefined = undefined;
  // Target changes run one at a time, in the order the component reports them.
  private targets: Promise<void> = Promise.resolve();

  constructor(scaleFactor: number, onStatus: (status: Status) => void) {
    this.scaleFactor = scaleFactor;
    this.onStatus = onStatus;
  }

  get currentMap(): MapHandle | undefined {
    return this.map;
  }

  start(): void {
    this.startMap().catch((error: Error) => this.fail(error));
  }

  dispose(): void {
    this.disposed = true;
    const session = this.session;
    const pacer = this.pacer;
    const map = this.map;
    const runtime = this.runtime;
    this.session = undefined;
    this.pacer = undefined;
    this.map = undefined;
    this.runtime = undefined;
    this.target = undefined;
    // After any target change in flight, release the session, and only then
    // the objects it used.
    this.targets = this.targets.then(async () => {
      await session?.release();
      pacer?.release();
      map?.dispose();
      runtime?.dispose();
    }).catch((error: Error) => console.error(`MapLibre: ${error.message}`));
  }

  setSize(size: LogicalSize): void {
    this.size = size;
    if (this.session !== undefined && this.target !== undefined) {
      this.session.discarding.resize(size);
    } else {
      this.retarget();
    }
  }

  surfaceAvailable(surface: bigint): void {
    this.surface = surface;
    this.retarget();
  }

  // The platform is destroying the surface. Release it from the session, and
  // keep the session and its tiles for the next surface.
  surfaceLost(surface: bigint): void {
    if (this.surface === surface) {
      this.surface = undefined;
    }
    this.enqueue(async () => {
      if (this.target === surface) {
        this.target = undefined;
        await this.session?.releaseTarget();
      }
    });
  }

  private async startMap(): Promise<void> {
    this.showStatus('Loading the library…');
    const runtime = RuntimeHandle.create();
    if (this.disposed) {
      runtime.dispose();
      return;
    }
    this.runtime = runtime;

    const map = runtime.createMap({
      initialExtent: {
        width: this.size?.width ?? 1,
        height: this.size?.height ?? 1,
        scaleFactor: this.scaleFactor,
      },
      mapMode: MapMode.continuous,
      eventMask: RuntimeEventFlag.mapStyleLoaded |
        RuntimeEventFlag.mapLoadingFailed |
        RuntimeEventFlag.mapRenderError,
    });
    this.map = map;
    runtime.discarding.setEventHandler({
      callback: (events: readonly RuntimeEvent[]) => {
        for (const event of events) {
          if (event.source !== map.id) {
            continue;
          }
          if (event.type === RuntimeEventType.mapStyleLoaded) {
            this.showStatus('');
          } else if (event.type === RuntimeEventType.mapLoadingFailed ||
            event.type === RuntimeEventType.mapRenderError) {
            this.fail(new Error(event.message));
          }
        }
      },
    });
    // The pacer demands each frame at the display's next refresh after the map
    // changes, so the session needs no redraw wake.
    this.pacer = ohosFramePacerCreate();
    await map.ready;
    if (this.disposed) {
      return;
    }
    map.discarding.updateCamera({
      mode: CameraUpdateMode.jump,
      camera: {
        center: { latitude: 48.8584, longitude: 2.2945 },
        zoom: 15.5,
        bearing: -30,
        pitch: 55,
      },
    });
    map.discarding.setStyleUrl(styleUrl);
    this.showStatus('Loading the style…');
    this.retarget();
  }

  // Gives the session the component's latest surface, attaching the session
  // first if there is none.
  private retarget(): void {
    this.enqueue(async () => {
      // The component can report its surface before the map is ready to take
      // a session.
      await this.map?.ready;
      const map = this.map;
      const size = this.size;
      const surface = this.surface;
      if (map === undefined || size === undefined || surface === undefined ||
        surface === this.target) {
        return;
      }
      this.target = surface;
      if (this.session === undefined) {
        // The session renders on a library thread, in a graphics context that
        // the library makes for the window.
        const session = map.attachOhosSurface({ logicalSize: size, surfaceId: surface }, undefined, {
          driver: RenderDriverKind.coreWorker,
          frameWake: { callback: () => this.drainFrameResults() },
        });
        this.session = session;
        session.setFramePacer(this.pacer);
        await session.ready;
      } else {
        await this.session.setOhosSurfaceTarget({ logicalSize: size, surfaceId: surface });
      }
    });
  }

  private enqueue(change: () => Promise<void>): void {
    this.targets = this.targets
      .then(() => this.disposed ? undefined : change())
      .catch((error: Error) => this.fail(error));
  }

  // The pacer demands the frames, and the frame wake reports how they went.
  private drainFrameResults(): void {
    const session = this.session;
    if (session === undefined) {
      return;
    }
    const results = session.drainFrameResults();
    if (results.some((result) => result.disposition === RenderResult.failed)) {
      this.fail(new Error(`The map failed to render a frame: ${session.getFailure()}`));
    }
  }

  // Once the controller shows an error, later progress doesn't replace it.
  private showStatus(message: string): void {
    if (this.failed || this.disposed) {
      return;
    }
    this.onStatus({ message: message, error: false });
  }

  private fail(error: Error): void {
    console.error(`MapLibre: ${error.message}`);
    if (this.failed || this.disposed) {
      return;
    }
    this.failed = true;
    this.onStatus({ message: error.message, error: true });
  }
}
