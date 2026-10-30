// How the suite reaches each host capability, or that the host lacks it.
//
// A case declares the capabilities it needs. The harness leaves a case out
// only for a capability that `unavailable` names, and fails the run when a
// case would be left out for any other reason. This module serves Node, Bun,
// and Deno; the ArkTS bundle replaces it with test/arkts/host-capabilities.ts,
// and the browser bundle with test/browser/host-capabilities.ts.

import { spawn } from "node:child_process";
import { createServer } from "node:http";
import { createRequire } from "node:module";
import type { AddressInfo } from "node:net";
import process from "node:process";
import { fileURLToPath } from "node:url";

export type Host = "node" | "bun" | "deno" | "arkts" | "browser" | "hermes";

export type Capability =
  | "httpOrigin"
  | "realmSpawn"
  | "forcedGc"
  | "childProcess"
  | "processExit"
  | "bufferTransfer"
  | "disposers"
  | "beforeLoad";

export interface ChildResult {
  readonly code: number | null;
  readonly signal: string | null;
  readonly stdout: string;
  readonly stderr: string;
}

export interface ChildOptions {
  /** Variables that replace or, when undefined, remove inherited ones. */
  readonly env?: Readonly<Record<string, string | undefined>>;
  readonly args?: readonly string[];
  /** Kills the child after this long; it then reports its signal. */
  readonly milliseconds?: number;
}

export interface OriginRequest {
  readonly path: string;
  readonly headers: Readonly<Record<string, string | string[] | undefined>>;
}

/** An HTTP origin that serves fixed bodies by path and records requests. */
export interface Origin {
  readonly url: string;
  /** The requests that the origin has received, in order. */
  requests(): Promise<readonly OriginRequest[]>;
  close(): Promise<void>;
}

/** A spawned realm: a worker that runs test/fixtures/realm.ts. */
export interface Realm {
  /** Resolves with the next message that the realm posts. */
  next(): Promise<unknown>;
  post(message: unknown): void;
  /**
   * Terminates the realm. Node, Bun, and ArkTS resolve once its environment
   * has ended; Deno resolves before the realm's teardown runs.
   */
  terminate(): Promise<void>;
}

export interface Capabilities {
  readonly host: Host;
  /** Capabilities that this host cannot provide at all. */
  readonly unavailable: readonly Capability[];
  readonly httpOrigin?: (
    routes: Readonly<Record<string, string>>,
  ) => Promise<Origin>;
  /** Starts test/fixtures/realm.ts in a new realm and sends it `data` first. */
  readonly realmSpawn?: (data: unknown) => Realm;
  /** Runs one full garbage collection. */
  readonly forcedGc?: () => void;
  /** Runs a script under the current runtime and collects its output. */
  readonly childProcess?: (
    script: string | URL,
    options?: ChildOptions,
  ) => Promise<ChildResult>;
  /** Ends the process from any realm, as `process.exit()` does. */
  readonly processExit?: (code: number) => never;
  /**
   * Moves a buffer's contents into a new buffer of `length` bytes, as
   * `ArrayBuffer.prototype.transfer` does, which detaches the old one.
   */
  readonly bufferTransfer?: (buffer: ArrayBuffer, length: number) => void;
  /** The host defines `Symbol.dispose` and `Symbol.asyncDispose`. */
  readonly disposers?: true;
  /**
   * What a call made before the browser module loaded threw, and how many
   * entries it left pending. Other hosts load the addon as they import the
   * binding, so no call precedes the load.
   */
  readonly beforeLoad?: () => {
    readonly error: unknown;
    readonly pending: number;
  };
  /** Loads the test-only addon over tests/graphics. */
  readonly testGraphics: () => unknown;
  /** Keeps a timer from holding the event loop open. */
  readonly unrefTimer: (timer: ReturnType<typeof setTimeout>) => void;
}

declare const Deno:
  | {
      execPath(): string;
      unrefTimer(id: number): void;
    }
  | undefined;
declare const Bun: { gc(force: boolean): void } | undefined;

const host: Host =
  typeof Deno !== "undefined"
    ? "deno"
    : typeof Bun !== "undefined"
      ? "bun"
      : "node";

function runtimeCommand(script: string, args: readonly string[]) {
  if (host === "deno") {
    return {
      command: Deno!.execPath(),
      // Loading the addon needs these permissions and no others.
      args: [
        "run",
        "--allow-ffi",
        "--allow-read",
        "--allow-env",
        "--v8-flags=--expose-gc",
        script,
        ...args,
      ],
    };
  }
  // A Bun script outside any node_modules installs a package it cannot
  // resolve from the registry, which would find the published backends.
  const flags = host === "node" ? ["--expose-gc"] : ["--no-install"];
  return {
    command: process.execPath,
    args: [...flags, script, ...args],
  };
}

function childProcess(
  script: string | URL,
  options: ChildOptions = {},
): Promise<ChildResult> {
  const { command, args } = runtimeCommand(
    typeof script === "string" ? script : fileURLToPath(script.href),
    options.args ?? [],
  );
  const env: Record<string, string> = {};
  for (const [name, value] of Object.entries({
    ...process.env,
    ...options.env,
  })) {
    if (value !== undefined) {
      env[name] = value;
    }
  }
  return new Promise((resolve, reject) => {
    const child = spawn(command, args, {
      env,
      stdio: ["ignore", "pipe", "pipe"],
    });
    let stdout = "";
    let stderr = "";
    child.stdout
      .setEncoding("utf8")
      .on("data", (chunk: string) => (stdout += chunk));
    child.stderr
      .setEncoding("utf8")
      .on("data", (chunk: string) => (stderr += chunk));
    // A child that hangs would hold this process through its pipes. The kill
    // says so in stderr, which the cases' assertions report, beside the
    // output that shows how far the child got.
    const milliseconds = options.milliseconds ?? 60_000;
    let killed = false;
    const timer = setTimeout(() => {
      killed = true;
      child.kill("SIGKILL");
    }, milliseconds);
    child.once("error", reject);
    child.once("close", (code, signal) => {
      clearTimeout(timer);
      if (killed) {
        stderr += `\nkilled after ${milliseconds} ms; stdout was:\n${stdout}`;
      }
      resolve({ code, signal, stdout, stderr });
    });
  });
}

function httpOrigin(routes: Readonly<Record<string, string>>): Promise<Origin> {
  const requests: OriginRequest[] = [];
  const server = createServer((request, response) => {
    const path = request.url ?? "/";
    requests.push({ path, headers: request.headers });
    const body = routes[path];
    if (body === undefined) {
      response.writeHead(404).end();
    } else {
      response.writeHead(200, { "content-type": "application/json" }).end(body);
    }
  });
  return new Promise((resolve, reject) => {
    server.once("error", reject);
    server.listen(0, "127.0.0.1", () => {
      // The origin answers native requests; it never keeps the test alive.
      server.unref();
      const { port } = server.address() as AddressInfo;
      resolve({
        url: `http://127.0.0.1:${port}`,
        requests: () => Promise.resolve(requests),
        close: () =>
          new Promise<void>((done) => {
            server.closeAllConnections();
            server.close(() => done());
          }),
      });
    });
  });
}

/** A queue of a realm's messages, which next() takes in order. */
function inbox(): { push(message: unknown): void; next(): Promise<unknown> } {
  const messages: unknown[] = [];
  const waiters: ((message: unknown) => void)[] = [];
  return {
    push(message) {
      const waiter = waiters.shift();
      if (waiter) {
        waiter(message);
      } else {
        messages.push(message);
      }
    },
    next() {
      if (messages.length > 0) {
        return Promise.resolve(messages.shift());
      }
      return new Promise((resolve) => waiters.push(resolve));
    },
  };
}

const realmScript = new URL("../fixtures/realm.ts", import.meta.url);

async function nodeWorkerThreads() {
  const { Worker } = await import("node:worker_threads");
  return (data: unknown): Realm => {
    const worker = new Worker(fileURLToPath(realmScript.href));
    const messages = inbox();
    const exited = new Promise<void>((resolve) =>
      worker.once("exit", () => resolve()),
    );
    worker.on("message", (message) => messages.push(message));
    worker.on("error", (error) => messages.push({ error: String(error) }));
    worker.postMessage(data);
    return {
      next: () => messages.next(),
      post: (message) => worker.postMessage(message),
      terminate: async () => {
        await worker.terminate();
        await exited;
      },
    };
  };
}

declare const Worker:
  | (new (
      script: URL,
      options: { type: "module" },
    ) => {
      onmessage: ((event: { data: unknown }) => void) | null;
      onerror: ((event: { message: string }) => void) | null;
      postMessage(message: unknown): void;
      terminate(): void;
    })
  | undefined;

function denoWorker(data: unknown): Realm {
  const worker = new Worker!(realmScript, { type: "module" });
  const messages = inbox();
  worker.onmessage = (event) => messages.push(event.data);
  worker.onerror = (event) => messages.push({ error: event.message });
  worker.postMessage(data);
  return {
    next: () => messages.next(),
    post: (message) => worker.postMessage(message),
    terminate: () => {
      worker.terminate();
      return Promise.resolve();
    },
  };
}

function forcedGc(): (() => void) | undefined {
  if (host === "bun") {
    return () => Bun!.gc(true);
  }
  const gc = (globalThis as { gc?: () => void }).gc;
  return gc === undefined ? undefined : () => gc();
}

export const capabilities: Capabilities = {
  host,
  unavailable: ["beforeLoad"],
  httpOrigin,
  realmSpawn: host === "deno" ? denoWorker : await nodeWorkerThreads(),
  forcedGc: forcedGc(),
  childProcess,
  processExit: (code) => process.exit(code),
  bufferTransfer: (buffer, length) => void buffer.transfer(length),
  disposers: true,
  testGraphics: () => {
    const path = process.env.MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS;
    if (!path) {
      throw new Error(
        "MAPLIBRE_NATIVE_FFI_TEST_GRAPHICS names the graphics addon",
      );
    }
    return createRequire(import.meta.url)(path);
  },
  unrefTimer:
    host === "deno"
      ? (timer) => Deno!.unrefTimer(timer as unknown as number)
      : (timer) => (timer as { unref?: () => void }).unref?.(),
};
