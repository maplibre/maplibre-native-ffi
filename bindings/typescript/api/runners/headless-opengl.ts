/**
 * A headless OpenGL context for the runners that load the Node-API addon.
 *
 * The render-session cases need a live graphics context, which is the host's to
 * supply. A browser has WebGL and hands over a context the module already
 * numbers; a Node, Bun, or Deno process has nothing until it asks EGL for one.
 * This asks, through the same driver the C API will call, so what those cases
 * exercise is this binding writing a real EGL descriptor for a real driver
 * rather than a shape nobody reads.
 *
 * The context is only ever borrowed: MapLibre creates its own context sharing
 * with the one made here, and this process keeps its handles alive for as long
 * as a session might use them.
 */

import type { NativePointer, OpenGlContext } from "../src/render.ts";
import { nativePointer } from "../src/render.ts";

const EGL_NONE = 0x3038;
const EGL_OPENGL_ES_API = 0x30a0;
const EGL_OPENGL_ES3_BIT = 0x0040;
const EGL_PBUFFER_BIT = 0x0001;
const EGL_SURFACE_TYPE = 0x3033;
const EGL_RENDERABLE_TYPE = 0x3040;
const EGL_RED_SIZE = 0x3024;
const EGL_GREEN_SIZE = 0x3023;
const EGL_BLUE_SIZE = 0x3022;
const EGL_ALPHA_SIZE = 0x3021;
const EGL_DEPTH_SIZE = 0x3025;
const EGL_STENCIL_SIZE = 0x3026;
const EGL_WIDTH = 0x3057;
const EGL_HEIGHT = 0x3056;
const EGL_CONTEXT_CLIENT_VERSION = 0x3098;
const EGL_DRAW = 0x3059;
const EGL_READ = 0x305a;
const EGL_PLATFORM_SURFACELESS_MESA = 0x31dd;
const EGL_PLATFORM_ANGLE_ANGLE = 0x3202;
const EGL_PLATFORM_ANGLE_TYPE_ANGLE = 0x3203;
const EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE = 0x3489;
const EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE = 0x3209;
const EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE = 0x320a;

const GL_TEXTURE_2D = 0x0de1;
const GL_RGBA = 0x1908;
const GL_RGBA8 = 0x8058;
const GL_UNSIGNED_BYTE = 0x1401;
const GL_TEXTURE_MIN_FILTER = 0x2801;
const GL_TEXTURE_MAG_FILTER = 0x2800;
const GL_TEXTURE_WRAP_S = 0x2802;
const GL_TEXTURE_WRAP_T = 0x2803;
const GL_LINEAR = 0x2601;
const GL_CLAMP_TO_EDGE = 0x812f;

/**
 * What this file offers a runner that could reach a driver.
 *
 * There is nothing to give back: one context serves every case, and it lives
 * as long as the process that asked for it, exactly as the browser runner's
 * WebGL context does.
 */
export interface HeadlessOpenGl {
  /** The context every session in this process attaches through. */
  readonly context: OpenGlContext;
  /**
   * A texture this process owns and keeps, for a caller-owned render target.
   *
   * Made in the context above, so a session that shares with it can see the
   * name, and left bound to nothing so the session's own state is undisturbed.
   */
  texture(width: number, height: number): { texture: number; target: number };
  /**
   * The pbuffer this fixture already made, for a session that presents.
   *
   * A surface session needs somewhere to present, and a headless host has this
   * rather than a window. It is the fixture's own, so a session that takes it
   * borrows it and must not destroy it.
   */
  readonly surface: NativePointer;
}

/** Reported when a driver is present but would not produce a context. */
class HeadlessOpenGlError extends Error {}

/**
 * The argument and result shapes these fixtures need.
 *
 * Every address is a `u64` rather than a pointer of the FFI's own kind: the
 * three runtimes disagree about what a pointer value is, and the C API takes
 * addresses as integers anyway, so nothing here has to unwrap one. `buffer`
 * is the exception, for the arrays EGL writes into and reads out of.
 */
type Kind = "u64" | "i32" | "u32" | "buffer" | "void";

/** An argument as the shared wrapper takes it, before the FFI's own form. */
type Argument = bigint | number | ArrayBufferView | null;

type Bound = (...args: Argument[]) => bigint | number | undefined;

/** What one symbol takes and answers. */
interface Declaration {
  readonly result: Kind;
  readonly parameters: readonly Kind[];
}

/**
 * Opens the first of these libraries that carries every symbol named.
 *
 * Reports `undefined` when none of them does, which is a host that cannot make
 * a context rather than a host that failed to.
 */
type Loader = (
  names: readonly string[],
  declarations: Record<string, Declaration>,
) => Record<string, Bound> | undefined;

/**
 * The FFI of whichever runtime is running.
 *
 * Node has none of its own and reaches a driver through koffi, which is a
 * development dependency of this package. Bun and Deno each ship one and use
 * it, which is not a preference: koffi declares its functions through Node-API
 * references that Bun aborts on when the collector releases them, so under Bun
 * the runtime's own FFI is the only one that survives a garbage collection.
 */
async function loaderFor(): Promise<Loader> {
  const global = globalThis as {
    Bun?: unknown;
    Deno?: { dlopen: DenoDlopen };
  };
  if (global.Bun !== undefined) {
    // Named indirectly, because Node resolves nothing under this scheme and
    // would fail on the specifier while loading this file.
    const specifier = "bun:ffi";
    return bunLoader((await import(specifier)) as BunFfi);
  }
  if (global.Deno !== undefined) {
    return denoLoader(global.Deno.dlopen);
  }
  return koffiLoader((await import("koffi")).default);
}

/** Tries each name in turn, and reports what the first one that opened gave. */
function firstThatOpens<T>(
  names: readonly string[],
  open: (name: string) => T,
): T | undefined {
  for (const name of names) {
    try {
      return open(name);
    } catch {
      // The next name, or none at all.
    }
  }
  return undefined;
}

const KOFFI_KINDS: Record<Kind, string> = {
  u64: "uint64_t",
  i32: "int32_t",
  u32: "uint32_t",
  buffer: "void *",
  void: "void",
};

function koffiLoader(koffi: typeof import("koffi").default): Loader {
  return (names, declarations) =>
    firstThatOpens(names, (name) => {
      const library = koffi.load(name);
      return Object.fromEntries(
        Object.entries(declarations).map(([symbol, declaration]) => {
          const parameters = declaration.parameters
            .map((kind) => KOFFI_KINDS[kind])
            .join(", ");
          const result = KOFFI_KINDS[declaration.result];
          return [
            symbol,
            library.func(`${result} ${symbol}(${parameters})`) as Bound,
          ];
        }),
      );
    });
}

/** What `bun:ffi` offers, named without importing a module Node cannot see. */
interface BunFfi {
  dlopen(
    path: string,
    symbols: Record<string, { args: readonly string[]; returns: string }>,
  ): { symbols: Record<string, (...args: unknown[]) => unknown> };
  ptr(view: ArrayBufferView): number | bigint;
}

const BUN_KINDS: Record<Kind, string> = {
  u64: "u64",
  i32: "i32",
  u32: "u32",
  buffer: "ptr",
  void: "void",
};

function bunLoader(ffi: BunFfi): Loader {
  return (names, declarations) =>
    firstThatOpens(names, (name) => {
      const { symbols } = ffi.dlopen(
        name,
        Object.fromEntries(
          Object.entries(declarations).map(([symbol, declaration]) => [
            symbol,
            {
              args: declaration.parameters.map((kind) => BUN_KINDS[kind]!),
              returns: BUN_KINDS[declaration.result]!,
            },
          ]),
        ),
      );
      return Object.fromEntries(
        Object.entries(declarations).map(([symbol, declaration]) => {
          const bound = symbols[symbol]!;
          // Bun takes an address rather than the view itself, and the view
          // stays reachable in the caller's frame across the call.
          const call: Bound = (...args) =>
            bound(
              ...args.map((value, index) =>
                declaration.parameters[index] === "buffer" && value !== null
                  ? ffi.ptr(value as ArrayBufferView)
                  : value,
              ),
            ) as bigint | number | undefined;
          return [symbol, call];
        }),
      );
    });
}

/** What `Deno.dlopen` is, named without a Deno type declaration. */
type DenoDlopen = (
  path: string,
  symbols: Record<string, { parameters: readonly string[]; result: string }>,
) => { symbols: Record<string, (...args: unknown[]) => unknown> };

const DENO_KINDS: Record<Kind, string> = {
  u64: "u64",
  i32: "i32",
  u32: "u32",
  buffer: "buffer",
  void: "void",
};

function denoLoader(dlopen: DenoDlopen): Loader {
  return (names, declarations) =>
    firstThatOpens(names, (name) => {
      const { symbols } = dlopen(
        name,
        Object.fromEntries(
          Object.entries(declarations).map(([symbol, declaration]) => [
            symbol,
            {
              parameters: declaration.parameters.map(
                (kind) => DENO_KINDS[kind]!,
              ),
              result: DENO_KINDS[declaration.result]!,
            },
          ]),
        ),
      );
      return symbols as Record<string, Bound>;
    });
}

/**
 * The EGL and OpenGL ES libraries, by the names each platform installs them
 * under.
 *
 * macOS has no system EGL: the packaged ANGLE runtime is what an EGL build
 * links against there, so it is loaded from the install directory the build
 * staged rather than from a system path. A Windows OpenGL build takes its
 * context from a window rather than from EGL, and nothing here makes one, so
 * that host reports no context at all.
 */
function libraryNames(): { egl: string[]; gles: string[] } {
  if (process.platform === "darwin") {
    const installDirectory = process.env["MAPLIBRE_NATIVE_C_INSTALL_DIR"];
    if (installDirectory === undefined) {
      return { egl: [], gles: [] };
    }
    return {
      egl: [`${installDirectory}/lib/libEGL.dylib`],
      gles: [`${installDirectory}/lib/libGLESv2.dylib`],
    };
  }
  if (process.platform === "win32") {
    return { egl: [], gles: [] };
  }
  return {
    egl: ["libEGL.so.1", "libEGL.so"],
    gles: ["libGLESv2.so.2", "libGLESv2.so"],
  };
}

/**
 * The EGL calls these fixtures make.
 *
 * The version eglInitialize would report is not read back, and EGL allows a
 * null for each, which is why those two are addresses rather than buffers.
 */
const EGL_SYMBOLS = {
  eglGetDisplay: { result: "u64", parameters: ["u64"] },
  // EGL 1.5 core. A platform display is what asks for a headless device by
  // name rather than taking whatever the environment happens to point at.
  eglGetPlatformDisplay: {
    result: "u64",
    parameters: ["u32", "u64", "buffer"],
  },
  eglInitialize: { result: "i32", parameters: ["u64", "u64", "u64"] },
  eglBindAPI: { result: "i32", parameters: ["u32"] },
  eglChooseConfig: {
    result: "i32",
    parameters: ["u64", "buffer", "buffer", "i32", "buffer"],
  },
  eglCreateContext: {
    result: "u64",
    parameters: ["u64", "u64", "u64", "buffer"],
  },
  eglCreatePbufferSurface: {
    result: "u64",
    parameters: ["u64", "u64", "buffer"],
  },
  eglMakeCurrent: { result: "i32", parameters: ["u64", "u64", "u64", "u64"] },
  eglGetCurrentDisplay: { result: "u64", parameters: [] },
  eglGetCurrentContext: { result: "u64", parameters: [] },
  eglGetCurrentSurface: { result: "u64", parameters: ["i32"] },
  eglDestroySurface: { result: "i32", parameters: ["u64", "u64"] },
  eglDestroyContext: { result: "i32", parameters: ["u64", "u64"] },
  eglTerminate: { result: "i32", parameters: ["u64"] },
  eglGetError: { result: "i32", parameters: [] },
  eglGetProcAddress: { result: "u64", parameters: ["buffer"] },
} as const satisfies Record<string, Declaration>;

/** The OpenGL ES calls a host texture needs. */
const GLES_SYMBOLS = {
  glGenTextures: { result: "void", parameters: ["i32", "buffer"] },
  glBindTexture: { result: "void", parameters: ["u32", "u32"] },
  glTexImage2D: {
    result: "void",
    parameters: ["u32", "i32", "i32", "i32", "i32", "i32", "u32", "u32", "u64"],
  },
  glTexParameteri: { result: "void", parameters: ["u32", "u32", "i32"] },
  glGetError: { result: "u32", parameters: [] },
} as const satisfies Record<string, Declaration>;

type Egl = Record<keyof typeof EGL_SYMBOLS, Bound>;
type Gles = Record<keyof typeof GLES_SYMBOLS, Bound>;

/** A NUL-terminated name, for the one call that takes a string. */
function cString(text: string): Uint8Array {
  return new TextEncoder().encode(`${text}\0`);
}

/**
 * A display that initialized, asked for in the order that keeps a headless host
 * working.
 *
 * The default display follows `EGL_PLATFORM`, which is what CI and the
 * devcontainer set, and is what every other binding's fixture asks for. A host
 * that set nothing gets an X11 display it cannot open, so the surfaceless
 * platform is named outright as the fallback rather than leaving the whole
 * suite to a stray environment variable.
 */
function openDisplay(egl: Egl): bigint {
  const attributes = (values: readonly number[]): BigInt64Array =>
    BigInt64Array.from(values, BigInt);
  const candidates: (() => bigint)[] =
    process.platform === "darwin"
      ? [
          () =>
            BigInt(
              egl.eglGetPlatformDisplay(
                EGL_PLATFORM_ANGLE_ANGLE,
                0n,
                attributes([
                  EGL_PLATFORM_ANGLE_TYPE_ANGLE,
                  EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
                  EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE,
                  EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE,
                  EGL_NONE,
                ]),
              )!,
            ),
        ]
      : [
          () => BigInt(egl.eglGetDisplay(0n)!),
          () =>
            BigInt(
              egl.eglGetPlatformDisplay(
                EGL_PLATFORM_SURFACELESS_MESA,
                0n,
                attributes([EGL_NONE]),
              )!,
            ),
        ];

  for (const candidate of candidates) {
    let display: bigint;
    try {
      display = candidate();
    } catch {
      continue;
    }
    if (display !== 0n && egl.eglInitialize(display, 0n, 0n) !== 0) {
      return display;
    }
  }
  throw new HeadlessOpenGlError("no EGL display initialized");
}

/**
 * Opens the one context this process renders through, or reports why it could
 * not.
 *
 * Reports `undefined` when there is no driver to ask at all, which is a host
 * that does not run the render cases. A driver that answers with a failure is
 * thrown instead: that is a host that should have worked.
 */
export async function openHeadlessOpenGl(): Promise<
  HeadlessOpenGl | undefined
> {
  const load = await loaderFor();
  const names = libraryNames();
  const egl = load(names.egl, EGL_SYMBOLS) as Egl | undefined;
  const gles = load(names.gles, GLES_SYMBOLS) as Gles | undefined;
  if (egl === undefined || gles === undefined) {
    return undefined;
  }

  const display = openDisplay(egl);
  if (egl.eglBindAPI(EGL_OPENGL_ES_API) === 0) {
    throw new HeadlessOpenGlError("eglBindAPI(EGL_OPENGL_ES_API) refused");
  }

  const configs = new BigUint64Array(1);
  const configCount = new Int32Array(1);
  const chosen = egl.eglChooseConfig(
    display,
    Int32Array.of(
      EGL_SURFACE_TYPE,
      EGL_PBUFFER_BIT,
      EGL_RENDERABLE_TYPE,
      EGL_OPENGL_ES3_BIT,
      EGL_RED_SIZE,
      8,
      EGL_GREEN_SIZE,
      8,
      EGL_BLUE_SIZE,
      8,
      EGL_ALPHA_SIZE,
      8,
      EGL_DEPTH_SIZE,
      24,
      EGL_STENCIL_SIZE,
      8,
      EGL_NONE,
    ),
    configs,
    1,
    configCount,
  );
  const config = configs[0]!;
  if (chosen === 0 || configCount[0] === 0 || config === 0n) {
    egl.eglTerminate(display);
    throw new HeadlessOpenGlError("no EGL pbuffer config carries OpenGL ES 3");
  }

  const context = BigInt(
    egl.eglCreateContext(
      display,
      config,
      0n,
      Int32Array.of(EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE),
    )!,
  );
  if (context === 0n) {
    egl.eglTerminate(display);
    throw new HeadlessOpenGlError(
      `eglCreateContext refused: 0x${Number(egl.eglGetError()).toString(16)}`,
    );
  }

  // Textures are made in this context, and a context needs a surface to become
  // current against. Nothing is ever drawn into it: the sessions draw into
  // targets of their own.
  const surface = BigInt(
    egl.eglCreatePbufferSurface(
      display,
      config,
      Int32Array.of(EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE),
    )!,
  );
  if (surface === 0n) {
    egl.eglDestroyContext(display, context);
    egl.eglTerminate(display);
    throw new HeadlessOpenGlError(
      `eglCreatePbufferSurface refused: 0x${Number(egl.eglGetError()).toString(16)}`,
    );
  }

  const descriptor: OpenGlContext = {
    platform: "egl",
    display: nativePointer(display),
    config: nativePointer(config),
    shareContext: nativePointer(context),
    getProcAddress: nativePointer(
      BigInt(egl.eglGetProcAddress(cString("eglGetProcAddress"))!),
    ),
  };

  /**
   * Runs `body` with this context current, and puts back whatever was current
   * before.
   *
   * A session made its own context current on this thread when it last
   * rendered, and is entitled to find it still current, so making one current
   * here is undone rather than left.
   */
  const withCurrent = <T>(body: () => T): T => {
    const previousDisplay = BigInt(egl.eglGetCurrentDisplay()!);
    const previousContext = BigInt(egl.eglGetCurrentContext()!);
    const previousDraw = BigInt(egl.eglGetCurrentSurface(EGL_DRAW)!);
    const previousRead = BigInt(egl.eglGetCurrentSurface(EGL_READ)!);
    if (egl.eglMakeCurrent(display, surface, surface, context) === 0) {
      throw new HeadlessOpenGlError(
        `eglMakeCurrent refused: 0x${Number(egl.eglGetError()).toString(16)}`,
      );
    }
    try {
      return body();
    } finally {
      if (previousDisplay === 0n) {
        egl.eglMakeCurrent(display, 0n, 0n, 0n);
      } else {
        egl.eglMakeCurrent(
          previousDisplay,
          previousDraw,
          previousRead,
          previousContext,
        );
      }
    }
  };

  return {
    context: descriptor,
    surface: surface as NativePointer,
    texture(width, height) {
      return withCurrent(() => {
        const made = new Uint32Array(1);
        gles.glGenTextures(1, made);
        const texture = made[0]!;
        if (texture === 0) {
          throw new HeadlessOpenGlError("glGenTextures produced no name");
        }
        gles.glBindTexture(GL_TEXTURE_2D, texture);
        gles.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        gles.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        gles.glTexParameteri(
          GL_TEXTURE_2D,
          GL_TEXTURE_WRAP_S,
          GL_CLAMP_TO_EDGE,
        );
        gles.glTexParameteri(
          GL_TEXTURE_2D,
          GL_TEXTURE_WRAP_T,
          GL_CLAMP_TO_EDGE,
        );
        gles.glTexImage2D(
          GL_TEXTURE_2D,
          0,
          GL_RGBA8,
          width,
          height,
          0,
          GL_RGBA,
          GL_UNSIGNED_BYTE,
          0n,
        );
        gles.glBindTexture(GL_TEXTURE_2D, 0);
        const error = Number(gles.glGetError());
        if (error !== 0) {
          throw new HeadlessOpenGlError(
            `making a host texture reported 0x${error.toString(16)}`,
          );
        }
        return { texture, target: GL_TEXTURE_2D };
      });
    },
  };
}
