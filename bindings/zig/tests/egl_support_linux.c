/*
 * tests/c/egl_support_linux.c
 *
 * Creates a real EGL display, config, context, and pbuffer surface for the EGL
 * surface session lifecycle test. Uses dlopen/dlsym rather than linking -lEGL
 * directly so the test binary shares the same EGL instance that was already
 * loaded as a dependency of libmaplibre-native-c.so. Linking a second -lEGL
 * would risk pulling in the system GLVND dispatcher as a separate instance,
 * splitting EGL state and causing eglMakeCurrent to fail.
 *
 * RTLD_NOLOAD is tried first so that if libEGL.so.1 is already in the process
 * (as a transitive dependency of the shared library) we get that exact handle.
 * If it is not yet loaded we fall back to a normal RTLD_NOW load.
 */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* ── Minimal EGL type/constant redefinitions ─────────────────────────────── */
/* Avoids including system EGL headers, which would require -I flags or       */
/* might resolve to pixi-bundled headers rather than the system headers.      */

typedef void* EGLDisplay;
typedef void* EGLConfig;
typedef void* EGLContext;
typedef void* EGLSurface;
typedef int EGLint;
typedef int EGLBoolean;

#define EGL_DEFAULT_DISPLAY ((void*)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_SURFACE ((EGLSurface)0)

#define EGL_TRUE 1
#define EGL_FALSE 0
#define EGL_NONE 0x3038

#define EGL_SURFACE_TYPE 0x3033
#define EGL_PBUFFER_BIT 0x0001
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_OPENGL_ES2_BIT 0x0004
#define EGL_RED_SIZE 0x3024
#define EGL_GREEN_SIZE 0x3023
#define EGL_BLUE_SIZE 0x3022
#define EGL_ALPHA_SIZE 0x3021
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_WIDTH 0x3057
#define EGL_HEIGHT 0x3056

/* ── EGL function pointer types ─────────────────────────────────────────── */

typedef EGLDisplay (*PFN_eglGetDisplay)(void*);
typedef EGLBoolean (*PFN_eglInitialize)(EGLDisplay, EGLint*, EGLint*);
typedef EGLBoolean (*PFN_eglChooseConfig)(
  EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*
);
typedef EGLContext (*PFN_eglCreateContext)(
  EGLDisplay, EGLConfig, EGLContext, const EGLint*
);
typedef EGLSurface (*PFN_eglCreatePbufferSurface)(
  EGLDisplay, EGLConfig, const EGLint*
);
typedef EGLBoolean (*PFN_eglDestroyContext)(EGLDisplay, EGLContext);
typedef EGLBoolean (*PFN_eglDestroySurface)(EGLDisplay, EGLSurface);
typedef EGLBoolean (*PFN_eglTerminate)(EGLDisplay);

/* ── Public context struct (layout must match egl_support.zig) ───────────── */

typedef struct mln_test_egl_context {
  void* egl_lib;
  EGLDisplay display;
  EGLContext context;
  EGLSurface surface;
  PFN_eglDestroyContext pfn_destroy_context;
  PFN_eglDestroySurface pfn_destroy_surface;
  PFN_eglTerminate pfn_terminate;
} mln_test_egl_context;

/* ── mln_test_egl_create ─────────────────────────────────────────────────── */

bool mln_test_egl_create(
  uint32_t width, uint32_t height, mln_test_egl_context* out
) {
  memset(out, 0, sizeof(*out));

  /* Prefer the already-loaded instance to avoid a second EGL. */
  void* lib = dlopen("libEGL.so.1", RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL);
  if (!lib) {
    lib = dlopen("libEGL.so.1", RTLD_NOW | RTLD_GLOBAL);
  }
  if (!lib) {
    fprintf(stderr, "egl_support: dlopen(libEGL.so.1): %s\n", dlerror());
    return false;
  }
  out->egl_lib = lib;

  PFN_eglGetDisplay pfn_get_display =
    (PFN_eglGetDisplay)dlsym(lib, "eglGetDisplay");
  PFN_eglInitialize pfn_initialize =
    (PFN_eglInitialize)dlsym(lib, "eglInitialize");
  PFN_eglChooseConfig pfn_choose_config =
    (PFN_eglChooseConfig)dlsym(lib, "eglChooseConfig");
  PFN_eglCreateContext pfn_create_context =
    (PFN_eglCreateContext)dlsym(lib, "eglCreateContext");
  PFN_eglCreatePbufferSurface pfn_create_pbuffer =
    (PFN_eglCreatePbufferSurface)dlsym(lib, "eglCreatePbufferSurface");
  out->pfn_destroy_context =
    (PFN_eglDestroyContext)dlsym(lib, "eglDestroyContext");
  out->pfn_destroy_surface =
    (PFN_eglDestroySurface)dlsym(lib, "eglDestroySurface");
  out->pfn_terminate = (PFN_eglTerminate)dlsym(lib, "eglTerminate");

  if (
    !pfn_get_display || !pfn_initialize || !pfn_choose_config ||
    !pfn_create_context || !pfn_create_pbuffer || !out->pfn_destroy_context ||
    !out->pfn_destroy_surface || !out->pfn_terminate
  ) {
    fprintf(stderr, "egl_support: missing EGL symbols\n");
    dlclose(lib);
    out->egl_lib = NULL;
    return false;
  }

  /* EGL_PLATFORM=surfaceless is set by CI; eglGetDisplay respects it via
   * the GLVND platform-selection mechanism. */
  out->display = pfn_get_display(EGL_DEFAULT_DISPLAY);
  if (out->display == EGL_NO_DISPLAY) {
    fprintf(stderr, "egl_support: eglGetDisplay failed\n");
    return false;
  }

  EGLint major = 0, minor = 0;
  if (!pfn_initialize(out->display, &major, &minor)) {
    fprintf(stderr, "egl_support: eglInitialize failed\n");
    return false;
  }

  /* Request a pbuffer-capable GLES2 config. */
  const EGLint config_attribs[] = {
    EGL_SURFACE_TYPE,
    EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE,
    EGL_OPENGL_ES2_BIT,
    EGL_RED_SIZE,
    8,
    EGL_GREEN_SIZE,
    8,
    EGL_BLUE_SIZE,
    8,
    EGL_ALPHA_SIZE,
    8,
    EGL_NONE,
  };
  EGLConfig config;
  EGLint num_configs = 0;
  if (
    !pfn_choose_config(
      out->display, config_attribs, &config, 1, &num_configs
    ) ||
    num_configs == 0
  ) {
    fprintf(stderr, "egl_support: eglChooseConfig found no suitable config\n");
    return false;
  }

  const EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION,
    2,
    EGL_NONE,
  };
  out->context =
    pfn_create_context(out->display, config, EGL_NO_CONTEXT, ctx_attribs);
  if (out->context == EGL_NO_CONTEXT) {
    fprintf(stderr, "egl_support: eglCreateContext failed\n");
    return false;
  }

  const EGLint pbuffer_attribs[] = {
    EGL_WIDTH, (EGLint)width, EGL_HEIGHT, (EGLint)height, EGL_NONE,
  };
  out->surface = pfn_create_pbuffer(out->display, config, pbuffer_attribs);
  if (out->surface == EGL_NO_SURFACE) {
    fprintf(stderr, "egl_support: eglCreatePbufferSurface failed\n");
    return false;
  }

  return true;
}

/* ── mln_test_egl_destroy ────────────────────────────────────────────────── */

void mln_test_egl_destroy(mln_test_egl_context* ctx) {
  if (ctx->surface) {
    ctx->pfn_destroy_surface(ctx->display, ctx->surface);
  }
  if (ctx->context) {
    ctx->pfn_destroy_context(ctx->display, ctx->context);
  }
  if (ctx->display) {
    ctx->pfn_terminate(ctx->display);
  }
  if (ctx->egl_lib) {
    dlclose(ctx->egl_lib);
  }
  memset(ctx, 0, sizeof(*ctx));
}
