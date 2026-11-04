// A continuous map on a MapSurfaceView. The view lends its surface when the
// platform provides one, and the controller attaches a session to it, or gives
// an attached session the new surface. Everything else matches the browser
// demo in docs/demo.

import {
  CameraUpdateMode,
  FrameDemandFlag,
  type LogicalSize,
  type MapHandle,
  MapMode,
  OpenGlClientApi,
  OpenGlContextOwnership,
  RenderDriverKind,
  RenderResult,
  type RenderSessionAttachOptions,
  type RenderSessionHandle,
  RuntimeEventFlag,
  RuntimeEventType,
  RuntimeHandle,
} from "@maplibre/native-ffi/hermes";
import { PixelRatio } from "react-native";
import { type SurfaceHandles, surfaces } from "react-native-example-host";

const styleUrl = "https://tiles.openfreemap.org/styles/liberty";

export interface Status {
  readonly message: string;
  readonly error: boolean;
}

export class MapController {
  readonly #onStatus: (status: Status) => void;
  #failed = false;
  #disposed = false;

  #runtime: RuntimeHandle | undefined;
  #map: MapHandle | undefined;
  #session: RenderSessionHandle | undefined;

  // The view's latest size, its latest surface, and the surface that the
  // session renders to.
  #size: LogicalSize | undefined;
  #surface: number | undefined;
  #target: number | undefined;
  // Target changes run one at a time, in the order the view reports them.
  #targets: Promise<void> = Promise.resolve();

  constructor(onStatus: (status: Status) => void) {
    this.#onStatus = onStatus;
  }

  get map(): MapHandle | undefined {
    return this.#map;
  }

  start(): void {
    this.#start().catch((error: unknown) => this.#fail(error));
  }

  dispose(): void {
    this.#disposed = true;
    const session = this.#session;
    const map = this.#map;
    const runtime = this.#runtime;
    const target = this.#target;
    this.#session = this.#map = this.#runtime = this.#target = undefined;
    try {
      session?.dispose();
      map?.dispose();
      runtime?.dispose();
    } catch (error) {
      // On a reload, React Native has the host clean the binding up before
      // React unmounts the app, and that cleanup already ended these objects.
      console.log("The binding was already cleaned up:", error);
    }
    if (target !== undefined) surfaces.release(target);
  }

  setSize(size: LogicalSize): void {
    this.#size = size;
    if (this.#session !== undefined) {
      this.#session.discarding.resize(size);
    } else {
      this.#retarget();
    }
  }

  surfaceAvailable(surface: number): void {
    this.#surface = surface;
    this.#retarget();
  }

  // The platform has taken the surface away, as when the app goes to the
  // background. Release it from the session, and keep the session and its
  // tiles for the next surface.
  surfaceLost(surface: number): void {
    if (this.#surface === surface) this.#surface = undefined;
    this.#enqueue(async () => {
      if (this.#target === surface) {
        this.#target = undefined;
        await this.#session?.releaseTarget();
      }
      surfaces.release(surface);
    });
  }

  async #start(): Promise<void> {
    this.#showStatus("Loading the library…");
    const runtime = await RuntimeHandle.create();
    if (this.#disposed) return runtime.dispose();
    this.#runtime = runtime;

    const map = runtime.createMap({
      initialExtent: {
        width: 1,
        height: 1,
        ...this.#size,
        scaleFactor: PixelRatio.get(),
      },
      mapMode: MapMode.continuous,
      eventMask:
        RuntimeEventFlag.mapStyleLoaded |
        RuntimeEventFlag.mapLoadingFailed |
        RuntimeEventFlag.mapRenderError,
    });
    this.#map = map;
    runtime.discarding.setEventHandler({
      callback: (events) => {
        for (const event of events) {
          if (event.source !== map.id) continue;
          switch (event.type) {
            case RuntimeEventType.mapStyleLoaded:
              this.#showStatus("");
              break;
            case RuntimeEventType.mapLoadingFailed:
            case RuntimeEventType.mapRenderError:
              this.#fail(new Error(event.message));
              break;
          }
        }
      },
    });
    await map.ready;
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
    this.#showStatus("Loading the style…");
    this.#retarget();
  }

  // Gives the session the view's latest surface, attaching the session first
  // if there is none.
  #retarget(): void {
    this.#enqueue(async () => {
      const map = this.#map;
      const size = this.#size;
      const surface = this.#surface;
      if (
        map === undefined ||
        size === undefined ||
        surface === undefined ||
        surface === this.#target
      ) {
        return;
      }
      const handles = surfaces.acquire(surface);
      // The view withdrew the surface before the controller got to it.
      if (handles === undefined) return;
      const previous = this.#target;
      this.#target = surface;
      if (this.#session === undefined) {
        this.#session = attach(map, size, handles, {
          frameWake: { callback: () => this.#drainFrameResults() },
          driverWorkWake: {
            callback: () => this.#session?.serviceDriverWork(),
          },
        });
        await this.#session.ready;
      } else {
        await setTarget(this.#session, size, handles);
      }
      if (previous !== undefined) surfaces.release(previous);
      this.#demandFrame();
    });
  }

  #enqueue(change: () => Promise<void>): void {
    this.#targets = this.#targets
      .then(() => (this.#disposed ? undefined : change()))
      .catch((error: unknown) => this.#fail(error));
  }

  #demandFrame = (): void => {
    if (this.#target === undefined) return;
    this.#session?.demandFrame({ flags: FrameDemandFlag.waitForUpdate });
  };

  #drainFrameResults(): void {
    const results = this.#session?.drainFrameResults() ?? [];
    if (results.length === 0) return;
    if (
      results.some(({ disposition }) => disposition === RenderResult.failed)
    ) {
      this.#fail(new Error("The map failed to render a frame."));
    } else {
      requestAnimationFrame(this.#demandFrame);
    }
  }

  // Once the controller shows an error, later progress doesn't replace it.
  #showStatus(message: string): void {
    if (this.#failed || this.#disposed) return;
    this.#onStatus({ message, error: false });
  }

  #fail(error: unknown): void {
    console.error(error);
    if (this.#failed || this.#disposed) return;
    this.#failed = true;
    this.#onStatus({
      message: error instanceof Error ? error.message : String(error),
      error: true,
    });
  }
}

type Wakes = Pick<RenderSessionAttachOptions, "frameWake" | "driverWorkWake">;

// Metal sessions render on a library thread. An EGL session's context belongs
// to the thread that services it, which is this one, the JavaScript thread.
function attach(
  map: MapHandle,
  logicalSize: LogicalSize,
  handles: SurfaceHandles,
  wakes: Wakes,
): RenderSessionHandle {
  switch (handles.kind) {
    case "metal":
      return map.attachMetalSurface(
        { logicalSize, layer: handles.layer },
        { driver: RenderDriverKind.coreWorker, frameWake: wakes.frameWake },
      );
    case "egl":
      return map.attachOpenGlSurface(
        {
          logicalSize,
          context: {
            ownership: OpenGlContextOwnership.dedicated,
            data: {
              kind: "egl",
              value: {
                display: handles.display,
                config: handles.config,
                clientApi: OpenGlClientApi.gles,
              },
            },
          },
          surface: handles.surface,
        },
        { driver: RenderDriverKind.callerGraphicsThread, ...wakes },
      );
  }
}

function setTarget(
  session: RenderSessionHandle,
  logicalSize: LogicalSize,
  handles: SurfaceHandles,
): Promise<void> {
  switch (handles.kind) {
    case "metal":
      return session.setMetalSurfaceTarget({
        logicalSize,
        layer: handles.layer,
      });
    case "egl":
      return session.setOpenGlSurfaceTarget({
        logicalSize,
        surface: handles.surface,
      });
  }
}
