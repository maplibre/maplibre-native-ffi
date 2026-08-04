/**
 * What a conformance case is written against.
 *
 * Every runtime this binding supports brings its own test framework, and none
 * agrees with the others about how a test is declared or how an assertion is
 * spelled. A case is therefore a named body written against the assertions
 * here, and each runtime's runner registers the same tree in whatever its
 * framework expects. A case that passes under Node and fails under Bun is then
 * a real difference between the runtimes rather than between two suites.
 */

import type { MaplibreError } from "../errors.ts";
import { RuntimeEventType } from "../events.ts";
import { nativeOf } from "../internal/private.ts";
import type { Transport } from "../internal/transport.ts";
import type { Map } from "../map.ts";
import type { Maplibre } from "../maplibre.ts";
import type { OpenGlContext } from "../render.ts";
import type { Runtime } from "../runtime.ts";

/** Assertions a case uses, which each runner maps onto its own framework. */
export interface Expect {
  equal<T>(actual: T, expected: T, what: string): void;
  notEqual<T>(actual: T, unexpected: T, what: string): void;
  ok(actual: boolean, what: string): void;
  /** Compares within `digits` decimal places, for values native computes. */
  closeTo(actual: number, expected: number, digits: number, what: string): void;
  /** Asserts a value is present, and narrows it for the rest of the case. */
  defined<T>(actual: T | undefined | null, what: string): T;
  absent(actual: unknown, what: string): void;
  contains(haystack: string, needle: string, what: string): void;
  /** Asserts the body throws the binding's error, and reports which one. */
  throws(body: () => void, what: string): MaplibreError;
  /**
   * Asserts the body throws at all.
   *
   * The layers beneath the public API report their own error types — a bad
   * allocation is not a MapLibre status — so a case about those asks only that
   * the failure surfaced.
   */
  throwsAny(body: () => void, what: string): Error;
  fail(what: string): never;
}

export interface CaseContext {
  readonly maplibre: Maplibre;
  readonly expect: Expect;
  /**
   * A directory this case may write a cache database into.
   *
   * Offline work needs one, and every runtime spells its filesystem
   * differently, so the runner supplies the path rather than the suite reaching
   * for an API only one of them has.
   */
  cacheDirectory(): Promise<string>;
  /**
   * Loads this package as a consumer would, in the module format named.
   *
   * Which formats a runtime resolves, and how, is the runtime's own business,
   * so the runner performs the load and the case checks what came back.
   */
  loadPackage(format: "esm" | "cjs"): Promise<typeof import("../index.ts")>;
  /**
   * A live graphics context to attach a render session to.
   *
   * Only a runner that declared `renderContext` is asked for one, so this may
   * throw where no host context exists rather than returning something every
   * case would have to check.
   */
  renderContext(): OpenGlContext;
  /**
   * A texture the host owns and keeps, for a caller-owned render target.
   *
   * The session draws into it and must not release or renumber it, which is
   * what the caller-owned cases check, so the host makes it and reads it back
   * afterwards.
   */
  hostTexture(
    width: number,
    height: number,
  ): { texture: number; target: number };
}

export interface ConformanceCase {
  readonly name: string;
  /**
   * The binding specification cases this proves, so coverage is countable
   * rather than asserted.
   */
  readonly spec?: readonly string[];
  /**
   * Transports this case applies to. Absent means every transport.
   *
   * A case names one only when the behavior genuinely belongs to a transport,
   * never to avoid a failure.
   */
  readonly transports?: readonly ("node-api" | "wasm")[];
  /**
   * Runtime capabilities this case needs, which not every runtime has.
   *
   * `packageResolution` means the runtime finds a module by package name.
   * `renderContext` means the host can hand the binding a live graphics
   * context to attach a render session to, which is a property of the host
   * rather than of the transport: a browser has WebGL, and a bare Node process
   * has none at all.
   * ArkTS resolves neither packages nor paths — an application reaches this
   * binding as a bundle its own build produced — so a case about how the
   * published package is laid out has nothing to look at there. This states
   * that as a property of the case rather than leaving a runner to skip a
   * failure by name.
   */
  readonly needs?: readonly Capability[];
  run(context: CaseContext): Promise<void> | void;
}

export interface ConformanceGroup {
  readonly name: string;
  readonly cases: readonly ConformanceCase[];
}

/** What a runner can offer beyond the transport it loaded. */
export type Capability = "packageResolution" | "renderContext";

/**
 * Whether a runner should register this case.
 *
 * Every runner asks the same question here rather than each writing its own
 * filter, because a runner that quietly disagreed would report a smaller suite
 * as a full pass.
 */
export function runsHere(
  entry: ConformanceCase,
  host: {
    readonly transport: "node-api" | "wasm";
    readonly capabilities: readonly Capability[];
  },
): boolean {
  if (
    entry.transports !== undefined &&
    !entry.transports.includes(host.transport)
  ) {
    return false;
  }
  return (entry.needs ?? []).every((needed) =>
    host.capabilities.includes(needed),
  );
}

/**
 * The groups a runner should register, with the cases it should run.
 *
 * A group whose every case belongs to another host is dropped rather than
 * registered empty: an empty suite is an error in some frameworks and a silent
 * pass in others, and neither is what "this host does not run these" means.
 */
export function groupsFor(
  groups: readonly ConformanceGroup[],
  host: {
    readonly transport: "node-api" | "wasm";
    readonly capabilities: readonly Capability[];
  },
): readonly { name: string; cases: readonly ConformanceCase[] }[] {
  return groups
    .map((group) => ({
      name: group.name,
      cases: group.cases.filter((entry) => runsHere(entry, host)),
    }))
    .filter((group) => group.cases.length > 0);
}

/** A style with no sources, so a load completes without touching the network. */
export const EMPTY_STYLE = JSON.stringify({
  version: 8,
  name: "empty",
  sources: {},
  layers: [],
});

/**
 * The transport a loaded library sits on.
 *
 * The bindability cases reach it directly, which is what lets the same rules be
 * checked against the Node-API addon and the WebAssembly module. It stays out
 * of the public API: this reads the association the package keeps privately,
 * rather than the facade publishing one.
 */
export function transportOf(maplibre: Maplibre): Transport {
  return nativeOf(maplibre).transport;
}

/** Runs `body` with a runtime, closing it and its maps however the body ends. */
export function withRuntime<T>(
  maplibre: Maplibre,
  body: (
    runtime: Runtime,
    open: (options?: { width: number; height: number }) => Map,
  ) => T,
): T {
  const runtime = maplibre.createRuntime();
  const maps: Map[] = [];
  try {
    return body(runtime, (options = { width: 256, height: 256 }) => {
      const map = runtime.createMap(options);
      maps.push(map);
      return map;
    });
  } finally {
    // Maps keep their runtime valid, so they close first however the body
    // ended.
    while (maps.length > 0) {
      maps.pop()!.close();
    }
    runtime.close();
  }
}

/** Pumps until an event of this type arrives, and reports the one that did. */
export function pumpFor(
  runtime: Runtime,
  type: RuntimeEventType,
  attempts = 200,
): ReturnType<Runtime["pollEvent"]> {
  for (let attempt = 0; attempt < attempts; attempt += 1) {
    runtime.pump(25);
    for (
      let event = runtime.pollEvent();
      event !== undefined;
      event = runtime.pollEvent()
    ) {
      if (event.type.equals(type)) {
        return event;
      }
    }
  }
  return undefined;
}

/** Loads a style into a map and waits for the load to report. */
export function loadStyle(
  runtime: Runtime,
  map: Map,
  style = EMPTY_STYLE,
): boolean {
  map.setStyleJson(style);
  return pumpFor(runtime, RuntimeEventType.mapStyleLoaded) !== undefined;
}

/** Drains everything queued, for a case that only needs the queue empty. */
export function drain(runtime: Runtime, maplibre: Maplibre, rounds = 40): void {
  for (let attempt = 0; attempt < rounds; attempt += 1) {
    runtime.pump(25);
    maplibre.deliverCallbacks();
    while (runtime.pollEvent() !== undefined) {
      // Drained so the queue does not hold the pump open.
    }
  }
}
