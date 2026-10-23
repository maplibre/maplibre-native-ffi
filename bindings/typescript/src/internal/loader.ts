// Loads the addon on Node, Bun, and Deno.
//
// The application installs the facade and one backend package. The backend
// package depends on one addon package per platform, which the package
// manager installs only on its own platform, so the loader resolves the
// backend package first and the addon package from there. The names follow
// scripts/pack.ts.

import { createRequire } from "node:module";
import { resolve } from "node:path";
import process from "node:process";

import type { Addon } from "./addon.ts";

const SCOPE = "@maplibre/native-ffi";
const BACKENDS = ["opengl", "metal", "vulkan"];

/** The addon package of a backend for this platform and architecture. */
function addonPackage(backend: string): string {
  const libc = process.platform === "linux" ? "-gnu" : "";
  return `${SCOPE}-${backend}-${process.platform}-${process.arch}${libc}`;
}

function isNotFound(error: unknown): boolean {
  return (error as { code?: unknown }).code === "MODULE_NOT_FOUND";
}

/** The manifest path of each backend package that is installed. */
function installedBackends(
  require: ReturnType<typeof createRequire>,
): [string, string][] {
  return BACKENDS.flatMap((backend): [string, string][] => {
    try {
      return [[backend, require.resolve(`${SCOPE}-${backend}/package.json`)]];
    } catch (error) {
      if (isNotFound(error)) {
        return [];
      }
      throw error;
    }
  });
}

/**
 * Loads the addon of the one installed backend package.
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
  const installed = installedBackends(require);
  if (installed.length === 0) {
    const packages = BACKENDS.map((backend) => `${SCOPE}-${backend}`);
    throw new Error(
      `no MapLibre Native backend package is installed; install one of ${packages.join(", ")}`,
    );
  }
  if (installed.length > 1) {
    const packages = installed.map(([backend]) => `${SCOPE}-${backend}`);
    throw new Error(
      `${packages.join(" and ")} are installed; install exactly one backend package`,
    );
  }
  const [backend, manifest] = installed[0]!;
  const addon = `${addonPackage(backend)}/maplibre_native_ffi.node`;
  try {
    return createRequire(manifest)(addon) as Addon;
  } catch (error) {
    if (isNotFound(error)) {
      throw new Error(
        `${SCOPE}-${backend} has no addon for ${process.platform}-${process.arch}, or its optional dependencies were not installed`,
        { cause: error },
      );
    }
    throw error;
  }
}
