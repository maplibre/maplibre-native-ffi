// Loads the addon on Node, Bun, and Deno.
//
// The application installs the facade and one runtime package. The runtime
// package depends on one addon package per platform, which the package
// manager installs only on its own platform, so the loader resolves the
// runtime package first and the addon package from there. The names follow
// scripts/pack.ts.

import { createRequire } from "node:module";
import { resolve } from "node:path";
import process from "node:process";

import type { Addon } from "./addon.ts";

const SCOPE = "@maplibre/native-ffi";
const RUNTIMES = ["runtime", "runtime-mingw"];

function addonPackage(runtime: string): string {
  let libc = process.platform === "linux" ? "-gnu" : "";
  // Deno publishes GNU Linux binaries; its getReport() needs --allow-sys.
  if (process.platform === "linux" && !("Deno" in globalThis)) {
    const report = process.report.getReport() as {
      header: { glibcVersionRuntime?: string };
    };
    libc = report.header.glibcVersionRuntime ? "-gnu" : "-musl";
  }
  return `${SCOPE}-${runtime}-${process.platform}-${process.arch}${libc}`;
}

function isNotFound(error: unknown): boolean {
  return (error as { code?: unknown }).code === "MODULE_NOT_FOUND";
}

/** The manifest path of each runtime package that is installed. */
function installedRuntimes(
  require: ReturnType<typeof createRequire>,
): [string, string][] {
  return RUNTIMES.flatMap((runtime): [string, string][] => {
    try {
      return [[runtime, require.resolve(`${SCOPE}-${runtime}/package.json`)]];
    } catch (error) {
      if (isNotFound(error)) {
        return [];
      }
      throw error;
    }
  });
}

/**
 * Loads the addon of the one installed runtime package.
 *
 * `MAPLIBRE_NATIVE_FFI_ADDON` is for the binding's own suites: it names an
 * addon file, relative to the working directory, to load instead.
 */
export function loadAddon(): Addon {
  const require = createRequire(import.meta.url);
  const path = process.env.MAPLIBRE_NATIVE_FFI_ADDON;
  if (path) {
    return require(resolve(path)) as Addon;
  }
  const installed = installedRuntimes(require);
  if (installed.length === 0) {
    const packages = RUNTIMES.map((runtime) => `${SCOPE}-${runtime}`);
    throw new Error(
      `no MapLibre Native runtime package is installed; install one of ${packages.join(", ")}`,
    );
  }
  if (installed.length > 1) {
    const packages = installed.map(([runtime]) => `${SCOPE}-${runtime}`);
    throw new Error(
      `${packages.join(" and ")} are installed; install exactly one runtime package`,
    );
  }
  const [runtime, manifest] = installed[0]!;
  const addon = `${addonPackage(runtime)}/maplibre_native_ffi.node`;
  try {
    return createRequire(manifest)(addon) as Addon;
  } catch (error) {
    if (isNotFound(error)) {
      throw new Error(
        `${SCOPE}-${runtime} has no addon for ${process.platform}-${process.arch}, or its optional dependencies were not installed`,
        { cause: error },
      );
    }
    throw error;
  }
}
