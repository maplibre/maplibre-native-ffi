#pragma once

#include <stdint.h>

typedef void* EGLDisplay;
typedef void* EGLConfig;
typedef void* EGLSurface;
typedef void* EGLContext;
typedef unsigned int EGLBoolean;
typedef int32_t EGLint;
typedef unsigned int EGLenum;
typedef intptr_t EGLAttrib;

#define EGL_FALSE 0
#define EGL_TRUE 1
#define EGL_NONE 0x3038
#define EGL_SURFACE_TYPE 0x3033
#define EGL_PBUFFER_BIT 0x0001
#define EGL_RENDERABLE_TYPE 0x3040
#define EGL_OPENGL_ES3_BIT 0x00000040
#define EGL_RED_SIZE 0x3024
#define EGL_GREEN_SIZE 0x3023
#define EGL_BLUE_SIZE 0x3022
#define EGL_ALPHA_SIZE 0x3021
#define EGL_DEPTH_SIZE 0x3025
#define EGL_STENCIL_SIZE 0x3026
#define EGL_CONTEXT_CLIENT_VERSION 0x3098
#define EGL_OPENGL_ES_API 0x30A0
#define EGL_WIDTH 0x3057
#define EGL_HEIGHT 0x3056
#define EGL_PLATFORM_SURFACELESS_MESA 0x31DD

#define EGL_DEFAULT_DISPLAY ((EGLDisplay)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_SURFACE ((EGLSurface)0)

EGLDisplay eglGetDisplay(EGLDisplay display_id);
EGLDisplay eglGetPlatformDisplay(
  EGLenum platform, void* native_display, const EGLAttrib* attrib_list
);
EGLBoolean eglInitialize(EGLDisplay dpy, EGLint* major, EGLint* minor);
EGLBoolean eglTerminate(EGLDisplay dpy);
EGLBoolean eglBindAPI(EGLenum api);
EGLBoolean eglChooseConfig(
  EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs,
  EGLint config_size, EGLint* num_config
);
EGLContext eglCreateContext(
  EGLDisplay dpy, EGLConfig config, EGLContext share_context,
  const EGLint* attrib_list
);
EGLSurface eglCreatePbufferSurface(
  EGLDisplay dpy, EGLConfig config, const EGLint* attrib_list
);
EGLBoolean eglMakeCurrent(
  EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx
);
EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface);
EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx);
