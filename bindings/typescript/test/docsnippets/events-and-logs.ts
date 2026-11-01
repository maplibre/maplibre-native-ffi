// Snippets for the docs site's Events and logs page. They are type-checked
// with the tests, which never run them.

import {
  type LogSeverity,
  type MapHandle,
  RuntimeEventFlag,
  RuntimeEventType,
  type RuntimeHandle,
  setLogCallback,
} from "../../src/index.ts";

function onStyleLoaded(): void {}

function onLoadingFailed(_message: string): void {}

function writeLog(_severity: LogSeverity, _message: string): void {}

export async function eventsAndLogs(runtime: RuntimeHandle): Promise<void> {
  let map: MapHandle | undefined;

  // #region handler
  // The handler runs on this thread. The binding queues each call.
  runtime.discarding.setEventHandler((events) => {
    for (const event of events) {
      if (event.source !== map?.id) continue;

      switch (event.type) {
        case RuntimeEventType.mapStyleLoaded:
          onStyleLoaded();
          break;
        case RuntimeEventType.mapLoadingFailed:
          onLoadingFailed(event.message);
          break;
      }
    }
  });
  // #endregion handler

  // #region mask
  const created = runtime.createMap({
    initialExtent: { width: 512, height: 512, scaleFactor: 1 },
    eventMask:
      RuntimeEventFlag.mapStyleLoaded | RuntimeEventFlag.mapLoadingFailed,
  });
  // #endregion mask
  map = created.map;
  await created.ready;

  // #region logs
  // The callback runs on this thread. The binding queues each call.
  setLogCallback((severity, _event, _code, message) => {
    writeLog(severity, message);
  });
  // #endregion logs
}
