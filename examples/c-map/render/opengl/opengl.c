// The OpenGL render target: an SDL GL context bridged to the C API through
// EGL handles, a GL ES 3.0 fullscreen-triangle compositor, and the three
// render-target modes behind the uniform interface in render.h.
//
// This build targets EGL on Linux; the WGL arm returns with Windows support.

#include <GLES3/gl3.h>
#include <SDL3/SDL.h>
#include <maplibre_native_c.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../diagnostics.h"
#include "../../render_target.h"
#include "../../types.h"
#include "../render.h"

#define GL_PROC_LIST(X) \
  X(ActiveTexture)      \
  X(AttachShader)       \
  X(BindFramebuffer)    \
  X(BindTexture)        \
  X(BindVertexArray)    \
  X(Clear)              \
  X(ClearColor)         \
  X(CompileShader)      \
  X(CreateProgram)      \
  X(CreateShader)       \
  X(DeleteProgram)      \
  X(DeleteShader)       \
  X(DeleteTextures)     \
  X(DeleteVertexArrays) \
  X(Disable)            \
  X(DrawArrays)         \
  X(Finish)             \
  X(GenTextures)        \
  X(GenVertexArrays)    \
  X(GetError)           \
  X(GetProgramInfoLog)  \
  X(GetProgramiv)       \
  X(GetShaderInfoLog)   \
  X(GetShaderiv)        \
  X(GetUniformLocation) \
  X(LinkProgram)        \
  X(ShaderSource)       \
  X(TexImage2D)         \
  X(TexParameteri)      \
  X(Uniform1i)          \
  X(UseProgram)         \
  X(Viewport)

/// The compositor's GL entry points, loaded through SDL's GL loader rather
/// than linked, so the example needs no GLES library at link time. The
/// prototypes in GLES3/gl3.h supply the pointer types.
typedef struct gl_procs {
#define GL_PROC_MEMBER(name) typeof(gl##name)* name;
  GL_PROC_LIST(GL_PROC_MEMBER)
#undef GL_PROC_MEMBER
} gl_procs;

static constexpr GLenum gl_texture_target = GL_TEXTURE_2D;
static constexpr GLint gl_internal_format = GL_RGBA;
static constexpr GLenum gl_pixel_format = GL_RGBA;
static constexpr GLenum gl_pixel_type = GL_UNSIGNED_BYTE;

static const char* const gles_texture_vertex_shader =
  "#version 300 es\n"
  "out vec2 out_uv;\n"
  "const vec2 positions[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), "
  "vec2(-1.0, 3.0));\n"
  "const vec2 uvs[3] = vec2[3](vec2(0.0, 0.0), vec2(2.0, 0.0), vec2(0.0, "
  "2.0));\n"
  "void main() {\n"
  "  gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);\n"
  "  out_uv = uvs[gl_VertexID];\n"
  "}\n";

static const char* const gles_texture_fragment_shader =
  "#version 300 es\n"
  "precision mediump float;\n"
  "uniform sampler2D map_texture;\n"
  "in vec2 out_uv;\n"
  "out vec4 out_color;\n"
  "void main() {\n"
  "  out_color = texture(map_texture, out_uv);\n"
  "}\n";

static void log_sdl_error(const char* message) {
  const char* details = SDL_GetError();
  fprintf(stderr, "%s: %s\n", message, details != nullptr ? details : "");
}

static app_error load_gl_procs(gl_procs* procs) {
#define GL_PROC_LOAD(name)                                        \
  {                                                               \
    SDL_FunctionPointer proc = SDL_GL_GetProcAddress("gl" #name); \
    if (proc == nullptr) {                                        \
      log_sdl_error("SDL_GL_GetProcAddress failed for gl" #name); \
      return APP_ERROR_BACKEND_SETUP_FAILED;                      \
    }                                                             \
    procs->name = (typeof(procs->name))proc;                      \
  }
  GL_PROC_LIST(GL_PROC_LOAD)
#undef GL_PROC_LOAD
  return APP_OK;
}

static app_error check_gl_error(const gl_procs* procs, const char* operation) {
  const GLenum gl_error = procs->GetError();
  if (gl_error == GL_NO_ERROR) {
    return APP_OK;
  }
  fprintf(stderr, "%s failed with OpenGL error 0x%x\n", operation, gl_error);
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

static void clear_gl_errors(const gl_procs* procs) {
  while (procs->GetError() != GL_NO_ERROR) {
  }
}

static void log_shader_info_log(
  const gl_procs* procs, GLuint shader, const char* name
) {
  GLchar buffer[1024];
  GLsizei length = 0;
  procs->GetShaderInfoLog(shader, sizeof(buffer), &length, buffer);
  fprintf(
    stderr, "OpenGL compositor %s compile failed: %.*s\n", name, (int)length,
    buffer
  );
}

static void log_program_info_log(
  const gl_procs* procs, GLuint program, const char* message
) {
  GLchar buffer[1024];
  GLsizei length = 0;
  procs->GetProgramInfoLog(program, sizeof(buffer), &length, buffer);
  fprintf(stderr, "%s: %.*s\n", message, (int)length, buffer);
}

static app_error compile_shader(
  const gl_procs* procs, GLenum kind, const char* source, const char* name,
  GLuint* out_shader
) {
  const GLuint shader = procs->CreateShader(kind);
  if (shader == 0) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  procs->ShaderSource(shader, 1, &source, nullptr);
  procs->CompileShader(shader);
  GLint compiled = GL_FALSE;
  procs->GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (compiled == GL_FALSE) {
    log_shader_info_log(procs, shader, name);
    procs->DeleteShader(shader);
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  *out_shader = shader;
  return APP_OK;
}

static app_error create_texture_program(
  const gl_procs* procs, GLuint* out_program
) {
  GLuint vertex = 0;
  app_error error = compile_shader(
    procs, GL_VERTEX_SHADER, gles_texture_vertex_shader,
    "texture vertex shader", &vertex
  );
  if (error != APP_OK) {
    return error;
  }
  GLuint fragment = 0;
  error = compile_shader(
    procs, GL_FRAGMENT_SHADER, gles_texture_fragment_shader,
    "texture fragment shader", &fragment
  );
  if (error != APP_OK) {
    procs->DeleteShader(vertex);
    return error;
  }

  const GLuint program = procs->CreateProgram();
  error = APP_ERROR_BACKEND_SETUP_FAILED;
  if (program != 0) {
    procs->AttachShader(program, vertex);
    procs->AttachShader(program, fragment);
    procs->LinkProgram(program);
    GLint linked = GL_FALSE;
    procs->GetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      log_program_info_log(
        procs, program, "OpenGL compositor program link failed"
      );
      procs->DeleteProgram(program);
    } else {
      *out_program = program;
      error = APP_OK;
    }
  }
  procs->DeleteShader(vertex);
  procs->DeleteShader(fragment);
  return error;
}

/// The graphics context: the SDL GL context plus the EGL handles the C API
/// descriptor names.
typedef struct opengl_context {
  SDL_Window* window;
  SDL_GLContext context;
  void* egl_display;
  void* egl_config;
  void* egl_surface;
} opengl_context;

static app_error platform_context(opengl_context* context) {
  void* display = SDL_EGL_GetCurrentDisplay();
  if (display == nullptr) {
    log_sdl_error("SDL_EGL_GetCurrentDisplay failed");
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  void* config = SDL_EGL_GetCurrentConfig();
  if (config == nullptr) {
    log_sdl_error("SDL_EGL_GetCurrentConfig failed");
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  void* surface = SDL_EGL_GetWindowSurface(context->window);
  if (surface == nullptr) {
    log_sdl_error("SDL_EGL_GetWindowSurface failed");
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  context->egl_display = display;
  context->egl_config = config;
  context->egl_surface = surface;
  return APP_OK;
}

static app_error opengl_context_init(
  opengl_context* context, SDL_Window* window
) {
  *context = (opengl_context){.window = window};
  context->context = SDL_GL_CreateContext(window);
  if (context->context == nullptr) {
    log_sdl_error("SDL_GL_CreateContext failed");
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  if (!SDL_GL_MakeCurrent(window, context->context)) {
    log_sdl_error("SDL_GL_MakeCurrent failed");
    SDL_GL_DestroyContext(context->context);
    context->context = nullptr;
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  const app_error error = platform_context(context);
  if (error != APP_OK) {
    SDL_GL_DestroyContext(context->context);
    context->context = nullptr;
    return error;
  }
  return APP_OK;
}

static void opengl_context_deinit(opengl_context* context) {
  if (context->context == nullptr) {
    return;
  }
  SDL_GL_MakeCurrent(context->window, nullptr);
  SDL_GL_DestroyContext(context->context);
  context->context = nullptr;
}

static app_error opengl_context_make_current(const opengl_context* context) {
  if (!SDL_GL_MakeCurrent(context->window, context->context)) {
    log_sdl_error("SDL_GL_MakeCurrent failed");
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  return APP_OK;
}

static app_error opengl_context_swap_window(const opengl_context* context) {
  if (!SDL_GL_SwapWindow(context->window)) {
    log_sdl_error("SDL_GL_SwapWindow failed");
    return APP_ERROR_BACKEND_DRAW_FAILED;
  }
  return APP_OK;
}

static mln_opengl_context_descriptor opengl_context_descriptor(
  const opengl_context* context
) {
  return (mln_opengl_context_descriptor){
    .size = sizeof(mln_opengl_context_descriptor),
    .platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL,
    .data.egl = {
      .size = sizeof(mln_egl_context_descriptor),
      .display = context->egl_display,
      .config = context->egl_config,
      .share_context = context->context,
      .get_proc_address = nullptr,
    },
  };
}

/// Re-reads the platform surface for this window.
///
/// SDL can hand back a different EGL window surface once the window has been
/// resized, and a session still presenting through the old one draws nowhere.
/// Reports whether the handle changed.
static app_error opengl_context_refresh_platform_surface(
  opengl_context* context, bool* out_replaced
) {
  void* previous = context->egl_surface;
  const app_error error = platform_context(context);
  if (error != APP_OK) {
    return error;
  }
  *out_replaced = context->egl_surface != previous;
  return APP_OK;
}

/// The compositor: a fullscreen-triangle pass that samples the map texture
/// into the window's default framebuffer.
typedef struct opengl_compositor {
  opengl_context context;
  viewport vp;
  gl_procs procs;
  GLuint program;
  GLuint vertex_array;
} opengl_compositor;

static app_error opengl_compositor_init(
  opengl_compositor* compositor, SDL_Window* window, viewport current_viewport
) {
  *compositor = (opengl_compositor){.vp = current_viewport};
  app_error error = opengl_context_init(&compositor->context, window);
  if (error != APP_OK) {
    return error;
  }
  error = load_gl_procs(&compositor->procs);
  if (error == APP_OK) {
    error = create_texture_program(&compositor->procs, &compositor->program);
  }
  if (error == APP_OK) {
    compositor->procs.GenVertexArrays(1, &compositor->vertex_array);
    if (compositor->vertex_array == 0) {
      error = APP_ERROR_BACKEND_SETUP_FAILED;
    }
  }
  if (error == APP_OK) {
    compositor->procs.UseProgram(compositor->program);
    const GLint sampler =
      compositor->procs.GetUniformLocation(compositor->program, "map_texture");
    if (sampler >= 0) {
      compositor->procs.Uniform1i(sampler, 0);
    }
    compositor->procs.UseProgram(0);
    error = check_gl_error(
      &compositor->procs, "initialize OpenGL texture compositor"
    );
  }
  if (error != APP_OK) {
    if (compositor->vertex_array != 0) {
      compositor->procs.DeleteVertexArrays(1, &compositor->vertex_array);
    }
    if (compositor->program != 0) {
      compositor->procs.DeleteProgram(compositor->program);
    }
    opengl_context_deinit(&compositor->context);
    return error;
  }
  return APP_OK;
}

static void opengl_compositor_deinit(opengl_compositor* compositor) {
  if (opengl_context_make_current(&compositor->context) == APP_OK) {
    compositor->procs.Finish();
    if (compositor->vertex_array != 0) {
      compositor->procs.DeleteVertexArrays(1, &compositor->vertex_array);
      compositor->vertex_array = 0;
    }
    if (compositor->program != 0) {
      compositor->procs.DeleteProgram(compositor->program);
      compositor->program = 0;
    }
  }
  opengl_context_deinit(&compositor->context);
}

static app_error opengl_compositor_resize(
  opengl_compositor* compositor, viewport current_viewport
) {
  const app_error error = opengl_context_make_current(&compositor->context);
  if (error != APP_OK) {
    return error;
  }
  compositor->vp = current_viewport;
  return APP_OK;
}

static app_error opengl_compositor_finish_frame(opengl_compositor* compositor) {
  const app_error error = opengl_context_make_current(&compositor->context);
  if (error != APP_OK) {
    return error;
  }
  compositor->procs.Finish();
  return APP_OK;
}

static app_error opengl_compositor_draw_texture_quad(
  opengl_compositor* compositor, GLuint texture
) {
  const gl_procs* procs = &compositor->procs;
  clear_gl_errors(procs);
  procs->BindFramebuffer(GL_FRAMEBUFFER, 0);
  procs->Disable(GL_CULL_FACE);
  procs->Disable(GL_DEPTH_TEST);
  procs->Disable(GL_SCISSOR_TEST);
  procs->Viewport(
    0, 0, (GLsizei)compositor->vp.physical_width,
    (GLsizei)compositor->vp.physical_height
  );
  procs->ClearColor(0.08f, 0.09f, 0.11f, 1.0f);
  procs->Clear(GL_COLOR_BUFFER_BIT);
  procs->UseProgram(compositor->program);
  procs->BindVertexArray(compositor->vertex_array);
  procs->ActiveTexture(GL_TEXTURE0);
  procs->BindTexture(gl_texture_target, texture);
  procs->TexParameteri(gl_texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  procs->TexParameteri(gl_texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  procs->DrawArrays(GL_TRIANGLES, 0, 3);
  procs->BindTexture(gl_texture_target, 0);
  procs->BindVertexArray(0);
  procs->UseProgram(0);
  return check_gl_error(procs, "draw OpenGL texture");
}

static app_error opengl_compositor_draw_texture(
  opengl_compositor* compositor, GLuint texture
) {
  app_error error = opengl_context_make_current(&compositor->context);
  if (error != APP_OK) {
    return error;
  }
  error = opengl_compositor_draw_texture_quad(compositor, texture);
  if (error != APP_OK) {
    return error;
  }
  return opengl_context_swap_window(&compositor->context);
}

static app_error borrowed_texture_create(
  const opengl_context* context, const gl_procs* procs,
  viewport current_viewport, GLuint* out_texture
) {
  const app_error error = opengl_context_make_current(context);
  if (error != APP_OK) {
    return error;
  }
  GLuint texture = 0;
  procs->GenTextures(1, &texture);
  procs->BindTexture(gl_texture_target, texture);
  procs->TexParameteri(gl_texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  procs->TexParameteri(gl_texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  procs->TexImage2D(
    gl_texture_target, 0, gl_internal_format,
    (GLsizei)current_viewport.physical_width,
    (GLsizei)current_viewport.physical_height, 0, gl_pixel_format,
    gl_pixel_type, nullptr
  );
  procs->BindTexture(gl_texture_target, 0);
  const app_error gl_error =
    check_gl_error(procs, "create OpenGL borrowed texture");
  if (gl_error != APP_OK) {
    procs->DeleteTextures(1, &texture);
    return gl_error;
  }
  *out_texture = texture;
  return APP_OK;
}

static void borrowed_texture_destroy(
  const opengl_context* context, const gl_procs* procs, GLuint* texture
) {
  if (*texture == 0) {
    return;
  }
  if (opengl_context_make_current(context) == APP_OK) {
    procs->DeleteTextures(1, texture);
  }
  *texture = 0;
}

struct render_target {
  render_target_mode mode;
  union {
    struct {
      opengl_compositor compositor;
      render_session session;
    } owned;
    struct {
      opengl_compositor compositor;
      render_session session;
      GLuint texture;
    } borrowed;
    struct {
      opengl_context context;
      gl_procs procs;
      render_session session;
    } surface;
  } as;
};

SDL_WindowFlags render_target_window_flags(void) { return SDL_WINDOW_OPENGL; }

app_error render_target_init(
  render_target** out_target, SDL_Window* window, viewport current_viewport,
  render_target_mode mode
) {
  render_target* target = calloc(1, sizeof(render_target));
  if (target == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  target->mode = mode;

  app_error error = APP_OK;
  switch (mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      error = opengl_compositor_init(
        &target->as.owned.compositor, window, current_viewport
      );
      break;
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      error = opengl_compositor_init(
        &target->as.borrowed.compositor, window, current_viewport
      );
      if (error == APP_OK) {
        error = borrowed_texture_create(
          &target->as.borrowed.compositor.context,
          &target->as.borrowed.compositor.procs, current_viewport,
          &target->as.borrowed.texture
        );
        if (error != APP_OK) {
          opengl_compositor_deinit(&target->as.borrowed.compositor);
        }
      }
      break;
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      error = opengl_context_init(&target->as.surface.context, window);
      if (error == APP_OK) {
        error = load_gl_procs(&target->as.surface.procs);
        if (error != APP_OK) {
          opengl_context_deinit(&target->as.surface.context);
        }
      }
      break;
  }
  if (error != APP_OK) {
    free(target);
    return error;
  }
  *out_target = target;
  return APP_OK;
}

static mln_opengl_borrowed_texture_descriptor borrowed_texture_descriptor(
  render_target* target, viewport current_viewport
) {
  mln_opengl_borrowed_texture_descriptor descriptor =
    mln_opengl_borrowed_texture_descriptor_default();
  descriptor.extent = render_target_extent(current_viewport);
  descriptor.physical_width = current_viewport.physical_width;
  descriptor.physical_height = current_viewport.physical_height;
  descriptor.context =
    opengl_context_descriptor(&target->as.borrowed.compositor.context);
  descriptor.texture = target->as.borrowed.texture;
  descriptor.target = gl_texture_target;
  return descriptor;
}

static mln_opengl_surface_descriptor surface_descriptor(
  render_target* target, viewport current_viewport
) {
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent = render_target_extent(current_viewport);
  descriptor.context = opengl_context_descriptor(&target->as.surface.context);
  descriptor.surface = target->as.surface.context.egl_surface;
  return descriptor;
}

app_error render_target_attach(
  render_target* target, mln_map map, viewport current_viewport
) {
  mln_render_session session = MLN_HANDLE_NULL;
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE: {
      mln_opengl_owned_texture_descriptor descriptor =
        mln_opengl_owned_texture_descriptor_default();
      descriptor.extent = render_target_extent(current_viewport);
      descriptor.context =
        opengl_context_descriptor(&target->as.owned.compositor.context);
      const mln_status status =
        mln_opengl_owned_texture_attach(map, &descriptor, &session);
      if (status != MLN_STATUS_OK) {
        diagnostics_log_status("OpenGL texture attach failed", status);
        return APP_ERROR_TEXTURE_ATTACH_FAILED;
      }
      target->as.owned.session =
        (render_session){.kind = RENDER_SESSION_TEXTURE, .handle = session};
      return APP_OK;
    }
    case RENDER_TARGET_MODE_BORROWED_TEXTURE: {
      const mln_opengl_borrowed_texture_descriptor descriptor =
        borrowed_texture_descriptor(target, current_viewport);
      const mln_status status =
        mln_opengl_borrowed_texture_attach(map, &descriptor, &session);
      if (status != MLN_STATUS_OK) {
        diagnostics_log_status("OpenGL borrowed texture attach failed", status);
        return APP_ERROR_TEXTURE_ATTACH_FAILED;
      }
      target->as.borrowed.session =
        (render_session){.kind = RENDER_SESSION_TEXTURE, .handle = session};
      return APP_OK;
    }
    case RENDER_TARGET_MODE_NATIVE_SURFACE: {
      const mln_opengl_surface_descriptor descriptor =
        surface_descriptor(target, current_viewport);
      const mln_status status =
        mln_opengl_surface_attach(map, &descriptor, &session);
      if (status != MLN_STATUS_OK) {
        diagnostics_log_status("OpenGL surface attach failed", status);
        return APP_ERROR_SURFACE_ATTACH_FAILED;
      }
      target->as.surface.session =
        (render_session){.kind = RENDER_SESSION_SURFACE, .handle = session};
      return APP_OK;
    }
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

void render_target_deinit(render_target* target) {
  if (target == nullptr) {
    return;
  }
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      render_session_close(&target->as.owned.session);
      opengl_compositor_deinit(&target->as.owned.compositor);
      break;
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      render_session_close(&target->as.borrowed.session);
      borrowed_texture_destroy(
        &target->as.borrowed.compositor.context,
        &target->as.borrowed.compositor.procs, &target->as.borrowed.texture
      );
      opengl_compositor_deinit(&target->as.borrowed.compositor);
      break;
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      render_session_close(&target->as.surface.session);
      opengl_context_deinit(&target->as.surface.context);
      break;
  }
  free(target);
}

/// Follows a resized window in borrowed-texture mode.
///
/// This example sizes the borrowed texture, not the session, so following a
/// resize means allocating one at the new size and handing it to the live
/// session. The session stays live, which is what keeps the map from going
/// cold on every resize.
static app_error resize_borrowed(
  render_target* target, viewport current_viewport
) {
  if (target->as.borrowed.session.kind != RENDER_SESSION_TEXTURE) {
    return APP_ERROR_TEXTURE_RESIZE_FAILED;
  }
  app_error error =
    opengl_compositor_resize(&target->as.borrowed.compositor, current_viewport);
  if (error != APP_OK) {
    return error;
  }

  const GLuint previous = target->as.borrowed.texture;
  GLuint replacement = 0;
  error = borrowed_texture_create(
    &target->as.borrowed.compositor.context,
    &target->as.borrowed.compositor.procs, current_viewport, &replacement
  );
  if (error != APP_OK) {
    return error;
  }
  target->as.borrowed.texture = replacement;
  const mln_opengl_borrowed_texture_descriptor descriptor =
    borrowed_texture_descriptor(target, current_viewport);
  const mln_status status = mln_opengl_borrowed_texture_set_target(
    target->as.borrowed.session.handle, &descriptor
  );
  if (status != MLN_STATUS_OK) {
    // A native error may mean the session took the replacement before
    // failing, and nothing here can tell that apart from a rejection that
    // came first, so detach before either target is released.
    mln_render_session_detach(target->as.borrowed.session.handle);
    diagnostics_log_status("OpenGL borrowed texture set target failed", status);
    target->as.borrowed.texture = previous;
    borrowed_texture_destroy(
      &target->as.borrowed.compositor.context,
      &target->as.borrowed.compositor.procs, &replacement
    );
    return APP_ERROR_TEXTURE_RESIZE_FAILED;
  }
  // Only once the session has taken the replacement, so a rejected one leaves
  // this target on the texture it already had.
  GLuint old = previous;
  borrowed_texture_destroy(
    &target->as.borrowed.compositor.context,
    &target->as.borrowed.compositor.procs, &old
  );
  return APP_OK;
}

/// Follows a resized window in native-surface mode.
///
/// SDL can hand back a different EGL window surface for the resized window.
/// The live session takes that replacement rather than being closed and
/// attached again, so it keeps its renderer either way.
static app_error resize_surface(
  render_target* target, viewport current_viewport
) {
  bool replaced = false;
  app_error error = opengl_context_refresh_platform_surface(
    &target->as.surface.context, &replaced
  );
  if (error != APP_OK) {
    // SDL owns the surfaces and may already have dropped the one the session
    // presents through, so detach rather than leave it naming a surface that
    // is gone.
    if (target->as.surface.session.kind == RENDER_SESSION_SURFACE) {
      mln_render_session_detach(target->as.surface.session.handle);
    }
    return APP_ERROR_SURFACE_ATTACH_FAILED;
  }
  if (!replaced) {
    return render_session_resize(&target->as.surface.session, current_viewport);
  }
  if (target->as.surface.session.kind != RENDER_SESSION_SURFACE) {
    return APP_ERROR_SURFACE_ATTACH_FAILED;
  }
  const mln_opengl_surface_descriptor descriptor =
    surface_descriptor(target, current_viewport);
  const mln_status status = mln_opengl_surface_set_target(
    target->as.surface.session.handle, &descriptor
  );
  if (status != MLN_STATUS_OK) {
    // A native error may mean the session took the replacement before
    // failing, and nothing here can tell that apart from a rejection that
    // came first. SDL owns both surfaces and already dropped the outgoing
    // one, so detaching is the only way to stop the session naming either.
    mln_render_session_detach(target->as.surface.session.handle);
    diagnostics_log_status("OpenGL surface set target failed", status);
    return APP_ERROR_SURFACE_ATTACH_FAILED;
  }
  return APP_OK;
}

app_error render_target_resize(
  render_target* target, viewport current_viewport
) {
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE: {
      const app_error error = opengl_compositor_resize(
        &target->as.owned.compositor, current_viewport
      );
      if (error != APP_OK) {
        return error;
      }
      return render_session_resize(&target->as.owned.session, current_viewport);
    }
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      return resize_borrowed(target, current_viewport);
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return resize_surface(target, current_viewport);
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

app_error render_target_finish_frame(render_target* target) {
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      return opengl_compositor_finish_frame(&target->as.owned.compositor);
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      return opengl_compositor_finish_frame(&target->as.borrowed.compositor);
    case RENDER_TARGET_MODE_NATIVE_SURFACE: {
      const app_error error =
        opengl_context_make_current(&target->as.surface.context);
      if (error != APP_OK) {
        return error;
      }
      target->as.surface.procs.Finish();
      return APP_OK;
    }
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

static app_error render_update_owned(
  render_target* target, bool* out_rendered
) {
  bool rendered = false;
  app_error error =
    render_session_render_update(&target->as.owned.session, &rendered);
  if (error != APP_OK || !rendered) {
    return error;
  }

  mln_opengl_owned_texture_frame frame = {.size = sizeof(frame)};
  const mln_status status = mln_opengl_owned_texture_acquire_frame(
    target->as.owned.session.handle, &frame
  );
  if (status == MLN_STATUS_INVALID_STATE) {
    return APP_OK;
  }
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("OpenGL texture acquire failed", status);
    return APP_ERROR_BACKEND_DRAW_FAILED;
  }

  error =
    opengl_compositor_draw_texture(&target->as.owned.compositor, frame.texture);
  const mln_status release_status = mln_opengl_owned_texture_release_frame(
    target->as.owned.session.handle, &frame
  );
  if (release_status != MLN_STATUS_OK) {
    diagnostics_log_status("OpenGL texture release failed", release_status);
  }
  if (error != APP_OK) {
    return error;
  }
  *out_rendered = true;
  return APP_OK;
}

app_error render_target_render_update(
  render_target* target, [[maybe_unused]] viewport current_viewport,
  bool* out_rendered
) {
  *out_rendered = false;
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      return render_update_owned(target, out_rendered);
    case RENDER_TARGET_MODE_BORROWED_TEXTURE: {
      bool rendered = false;
      const app_error error =
        render_session_render_update(&target->as.borrowed.session, &rendered);
      if (error != APP_OK || !rendered) {
        return error;
      }
      const app_error draw_error = opengl_compositor_draw_texture(
        &target->as.borrowed.compositor, target->as.borrowed.texture
      );
      if (draw_error != APP_OK) {
        return draw_error;
      }
      *out_rendered = true;
      return APP_OK;
    }
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return render_session_render_update(
        &target->as.surface.session, out_rendered
      );
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}
