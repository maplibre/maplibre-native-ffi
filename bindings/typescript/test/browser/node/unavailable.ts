// The node:fs, node:os, node:path, node:url, and node:child_process exports
// that the bundled cases import for their child-process variants, which do
// not run in a browser. Each one fails if a case reaches it.

function unavailable(): never {
  throw new Error("a page has no file system or process API");
}

export const cpSync = unavailable;
export const mkdirSync = unavailable;
export const mkdtempSync = unavailable;
export const rmSync = unavailable;
export const writeFileSync = unavailable;
export const tmpdir = unavailable;
export const basename = unavailable;
export const dirname = unavailable;
export const join = unavailable;
export const fileURLToPath = unavailable;
export const spawnSync = unavailable;
