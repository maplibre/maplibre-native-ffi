/**
 * Offline operations.
 *
 * Every offline operation is a command: the call reports acceptance and an id,
 * the completion arrives as a runtime event, and the result is taken afterwards.
 * These tests follow that whole shape rather than any one step of it.
 */

import {
  AmbientCacheOperation,
  MaplibreError,
  Maplibre,
  type Runtime,
  RuntimeEventType,
} from "../src/index.ts";
import { mkdtemp, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";
import { afterEach, describe, expect, it } from "vitest";

const maplibre = await Maplibre.load();

let runtime: Runtime | undefined;
let cacheDirectory: string | undefined;

afterEach(async () => {
  runtime?.close();
  runtime = undefined;
  if (cacheDirectory !== undefined) {
    await rm(cacheDirectory, { recursive: true, force: true });
    cacheDirectory = undefined;
  }
});

/** A runtime with a database of its own, which offline work needs. */
async function offlineRuntime(): Promise<Runtime> {
  cacheDirectory = await mkdtemp(path.join(tmpdir(), "maplibre-offline-"));
  runtime = maplibre.createRuntime({
    cachePath: path.join(cacheDirectory, "cache.db"),
  });
  return runtime;
}

/** Pumps until an operation reports that it finished. */
function awaitCompletion(created: Runtime): boolean {
  for (let attempt = 0; attempt < 200; attempt += 1) {
    created.pump(25);
    for (
      let event = created.pollEvent();
      event !== undefined;
      event = created.pollEvent()
    ) {
      if (event.type.equals(RuntimeEventType.offlineOperationCompleted)) {
        return true;
      }
    }
  }
  return false;
}

describe("offline operations", () => {
  it("lists an empty database through start, event, and take", async () => {
    const created = await offlineRuntime();
    const operation = created.startOfflineRegionList();
    expect(operation).toBeGreaterThan(0n);
    expect(awaitCompletion(created)).toBe(true);

    const regions = created.takeOfflineRegionList(operation);
    expect(regions).toEqual([]);

    // Ownership transferred, so the same result cannot be taken twice.
    expect(() => created.takeOfflineRegionList(operation)).toThrow(
      MaplibreError,
    );
  });

  it("runs an ambient cache operation", async () => {
    const created = await offlineRuntime();
    const operation = created.startAmbientCacheOperation(
      AmbientCacheOperation.clear,
    );
    expect(operation).toBeGreaterThan(0n);
    expect(awaitCompletion(created)).toBe(true);
  });

  it("discards an operation whose result nobody takes", async () => {
    const created = await offlineRuntime();
    const operation = created.startOfflineRegionList();
    expect(awaitCompletion(created)).toBe(true);
    created.discardOfflineOperation(operation);
    // The operation is gone, so taking its result now fails.
    expect(() => created.takeOfflineRegionList(operation)).toThrow(
      MaplibreError,
    );
  });

  it("reports an unknown operation id rather than inventing a result", async () => {
    const created = await offlineRuntime();
    try {
      created.takeOfflineRegionList(9_999_999n);
      expect.unreachable("an operation id nobody issued names nothing");
    } catch (error) {
      expect(error).toBeInstanceOf(MaplibreError);
      expect((error as MaplibreError).diagnostic).not.toBe("");
    }
  });
});
