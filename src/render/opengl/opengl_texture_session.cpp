#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <mln/gfx/backend_scope.hpp>
#include <mln/gfx/headless_backend.hpp>
#include <mln/gfx/renderable.hpp>
#include <mln/gl/context.hpp>
#include <mln/gl/defines.hpp>
#include <mln/gl/framebuffer.hpp>
#include <mln/gl/renderable_resource.hpp>
#include <mln/gl/renderbuffer_resource.hpp>
#include <mln/gl/renderer_backend.hpp>
#include <mln/gl/texture2d.hpp>
#include <mln/platform/gl_functions.hpp>
#include <mln/util/image.hpp>
#include <mln/util/size.hpp>

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#include <EGL/egl.h>
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
#include <emscripten/html5.h>
#include <emscripten/threading.h>
#include <pthread.h>
#endif

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c/base.h"
#include "render/opengl/context_mode.hpp"
#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)
#include "render/opengl/egl_context.hpp"
#endif
#include "render/opengl/webgl_worker.hpp"
#include "render/opengl/wgl_common.hpp"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"

namespace {

constexpr auto opengl_texture_target = uint32_t{GL_TEXTURE_2D};
constexpr auto opengl_internal_format = uint32_t{GL_RGBA8};
constexpr auto opengl_pixel_format = uint32_t{GL_RGBA};
constexpr auto opengl_pixel_type = uint32_t{GL_UNSIGNED_BYTE};

[[noreturn]] void throw_opengl_framebuffer_error() {
  switch (mln::platform::glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
    case GL_FRAMEBUFFER_COMPLETE:
      break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
      throw std::runtime_error("OpenGL framebuffer has incomplete attachment");
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
      throw std::runtime_error("OpenGL framebuffer has no attachment");
    case GL_FRAMEBUFFER_UNSUPPORTED:
      throw std::runtime_error(
        "OpenGL framebuffer configuration is unsupported"
      );
    default:
      throw std::runtime_error("OpenGL framebuffer is incomplete");
  }
  throw std::runtime_error("OpenGL framebuffer check unexpectedly succeeded");
}

auto check_opengl_framebuffer() -> void {
  if (
    mln::platform::glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
    GL_FRAMEBUFFER_COMPLETE
  ) {
    throw_opengl_framebuffer_error();
  }
}

class OpenGLTextureRenderableResource final
    : public mln::gl::RenderableResource {
 public:
  OpenGLTextureRenderableResource(
    mln::gl::Context& context_, mln::Size size_, uint32_t borrowed_texture
  )
      : context(context_), size(size_), borrowed_texture_(borrowed_texture) {}

  ~OpenGLTextureRenderableResource() noexcept override = default;

  void bind() override {
    try {
      ensure_resources();
      context.bindFramebuffer = framebuffer_->framebuffer;
      context.scissorTest = {0, 0, 0, 0};
      context.viewport = {0, 0, size};
    } catch (const std::exception& exception) {
      throw std::runtime_error(
        std::string{"binding OpenGL texture renderable: "} + exception.what()
      );
    }
  }

  // A caller-owned texture returns to its owner as soon as the render update
  // returns, so it completes every frame. A session-owned texture is handed
  // over at acquire-frame, which completes the rendering itself.
  void swap() override {
    if (borrowed_texture_ != 0) {
      context.finish();
    }
  }

  auto readStillImage() -> mln::PremultipliedImage {
    bind();
    return context.readFramebuffer<mln::PremultipliedImage>(size);
  }

  auto texture() -> uint32_t {
    ensure_resources();
    if (borrowed_texture_ != 0) {
      return borrowed_texture_;
    }
    return static_cast<mln::gl::Texture2D&>(*texture_).getTextureID();
  }

 private:
  void ensure_resources() {
    if (framebuffer_) {
      return;
    }

    auto texture_id = borrowed_texture_;
    if (borrowed_texture_ == 0) {
      texture_ = context.createTexture2D();
      texture_->setSize(size);
      texture_->setFormat(
        mln::gfx::TexturePixelType::RGBA,
        mln::gfx::TextureChannelDataType::UnsignedByte
      );
      texture_->setSamplerConfiguration(
        {.filter = mln::gfx::TextureFilterType::Linear,
         .wrapU = mln::gfx::TextureWrapType::Clamp,
         .wrapV = mln::gfx::TextureWrapType::Clamp}
      );
      texture_->create();
      texture_id = static_cast<mln::gl::Texture2D&>(*texture_).getTextureID();
    }

    depth_stencil_ =
      context.createRenderbuffer<mln::gfx::RenderbufferPixelType::DepthStencil>(
        size
      );
    auto framebuffer_id = mln::platform::GLuint{};
    mln::platform::glGenFramebuffers(1, &framebuffer_id);
    auto framebuffer = mln::gl::Framebuffer{
      .size = size,
      .framebuffer =
        mln::gl::UniqueFramebuffer{std::move(framebuffer_id), {&context}}
    };
    context.bindFramebuffer = framebuffer.framebuffer;
    mln::platform::glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0
    );
    auto& depth_stencil_resource =
      depth_stencil_->getResource<mln::gl::RenderbufferResource>();
#ifdef GL_DEPTH_STENCIL_ATTACHMENT
    mln::platform::glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
      depth_stencil_resource.renderbuffer
    );
#else
    mln::platform::glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
      depth_stencil_resource.renderbuffer
    );
    mln::platform::glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
      depth_stencil_resource.renderbuffer
    );
#endif
    check_opengl_framebuffer();
    framebuffer_ = std::move(framebuffer);
  }

  mln::gl::Context& context;
  mln::Size size;
  uint32_t borrowed_texture_ = 0;
  mln::gfx::Texture2DPtr texture_;
  std::optional<
    mln::gfx::Renderbuffer<mln::gfx::RenderbufferPixelType::DepthStencil>>
    depth_stencil_;
  std::optional<mln::gl::Framebuffer> framebuffer_;
};

class OpenGLTextureBackend final : public mln::gl::RendererBackend,
                                   public mln::gfx::HeadlessBackend {
 public:
  OpenGLTextureBackend(
    const mln_opengl_owned_texture_descriptor& descriptor, mln::Size size,
    std::size_t ring_depth
  )
      : mln::gl::RendererBackend(mln::core::opengl::session_context_mode),
        mln::gfx::HeadlessBackend(size),
        context_(descriptor.context),
        ring_(ring_depth) {}

  OpenGLTextureBackend(
    const mln_opengl_borrowed_texture_descriptor& descriptor, mln::Size size
  )
      : mln::gl::RendererBackend(mln::core::opengl::session_context_mode),
        mln::gfx::HeadlessBackend(size),
        context_(descriptor.context),
        borrowed_texture_(descriptor.texture),
        ring_(0) {}

  OpenGLTextureBackend(const OpenGLTextureBackend&) = delete;
  auto operator=(const OpenGLTextureBackend&) -> OpenGLTextureBackend& = delete;
  OpenGLTextureBackend(OpenGLTextureBackend&&) = delete;
  auto operator=(OpenGLTextureBackend&&) -> OpenGLTextureBackend& = delete;

  ~OpenGLTextureBackend() noexcept override {
    try {
      destroy_backend();
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error("destroying OpenGL texture backend failed");
    }
  }

  void destroy_backend() {
    auto cleanup = [this] {
      resource.reset();
      ring_.clear();
      context.reset();
    };
    if (has_native_context()) {
      auto guard = mln::gfx::BackendScope{*this};
      cleanup();
    } else {
      cleanup();
    }
    getThreadPool().runRenderJobs(true);
    destroy_native_context();
  }

  auto getDefaultRenderable() -> mln::gfx::Renderable& override {
    const auto current_size = getSize();
    if (!resource || resource_size_ != current_size) {
      resource = std::make_unique<OpenGLTextureRenderableResource>(
        getContext<mln::gl::Context>(), current_size, borrowed_texture_
      );
      resource_size_ = current_size;
      // Recorded with the resource it describes, so a slot that keeps an older
      // resource keeps the size that resource was built for.
      ring_.record_size(current_size);
    }
    return *this;
  }

  auto readStillImage() -> mln::PremultipliedImage override {
    auto& renderable =
      getDefaultRenderable().getResource<OpenGLTextureRenderableResource>();
    return renderable.readStillImage();
  }

  auto getRendererBackend() -> mln::gfx::RendererBackend* override {
    return this;
  }

  void updateAssumedState() override {
    assumeFramebufferBinding(
      mln::gl::RendererBackend::ImplicitFramebufferBinding
    );
  }

  auto texture() -> uint32_t {
    auto& renderable =
      getDefaultRenderable().getResource<OpenGLTextureRenderableResource>();
    return renderable.texture();
  }

  auto select_slot(std::size_t slot) -> bool {
    if (!ring_.select(slot, size, resource)) return false;
    resource_size_ = ring_.selected_size();
    return true;
  }

  void set_ring_size(mln::Size new_size) { size = new_size; }

  void finish_rendering() { getContext<mln::gl::Context>().finish(); }

  // Renders into a different caller-owned texture from here on, keeping the
  // session's context and everything the renderer built in it.
  void set_borrowed_texture(uint32_t texture, mln::Size new_size) {
    borrowed_texture_ = texture;
    // setSize() drops the renderable unconditionally, rebuilding the
    // framebuffer against the new texture even when the size is unchanged. No
    // context is made current: the framebuffer and renderbuffer names go to the
    // context's abandoned lists and are deleted on the next render.
    setSize(new_size);
  }

  [[nodiscard]] auto context_descriptor() const
    -> const mln_opengl_context_descriptor& {
    return context_;
  }

 private:
  [[nodiscard]] auto has_native_context() const -> bool {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    return render_context_ != nullptr;
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    return egl_context_.has_value();
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    return context_.data.webgl.context > 0;
#else
    return false;
#endif
  }

  auto getExtensionFunctionPointer(const char* name)
    -> mln::gl::ProcAddress override {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    using GetProcAddressFunction = PROC(WINAPI*)(LPCSTR);
    auto* loader = reinterpret_cast<GetProcAddressFunction>(
      context_.data.wgl.get_proc_address
    );
    if (loader != nullptr) {
      auto* proc = loader(name);
      if (mln::core::opengl::is_valid_wgl_proc_address(proc)) {
        return reinterpret_cast<mln::gl::ProcAddress>(proc);
      }
    }
    auto* proc = wglGetProcAddress(name);
    if (mln::core::opengl::is_valid_wgl_proc_address(proc)) {
      return reinterpret_cast<mln::gl::ProcAddress>(proc);
    }
    return reinterpret_cast<mln::gl::ProcAddress>(
      mln::core::opengl::get_opengl32_proc_address(name)
    );
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    return reinterpret_cast<mln::gl::ProcAddress>(
      mln::core::opengl::get_egl_proc_address(
        context_.data.egl, name,
        egl_context_ ? egl_context_->active_api() : EGL_NONE
      )
    );
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    // Emscripten resolves GLES entry points at link time.
    (void)name;
    return nullptr;
#else
    (void)name;
    return nullptr;
#endif
  }

  void activate() override {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    previous_device_context_ = wglGetCurrentDC();
    previous_render_context_ = wglGetCurrentContext();
    try {
      if (render_context_ == nullptr) {
        create_wgl_context();
      }
      if (
        wglMakeCurrent(
          static_cast<HDC>(context_.data.wgl.device_context),
          static_cast<HGLRC>(render_context_)
        ) == 0
      ) {
        throw std::runtime_error("Switching OpenGL WGL context failed");
      }
      validate_wgl_context_support();
    } catch (...) {
      (void)wglMakeCurrent(
        static_cast<HDC>(previous_device_context_),
        static_cast<HGLRC>(previous_render_context_)
      );
      previous_device_context_ = nullptr;
      previous_render_context_ = nullptr;
      throw;
    }
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    if (!egl_context_) {
      egl_context_.emplace(context_.data.egl, context_.ownership);
    }
    egl_context_->activate_pbuffer();
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    previous_webgl_context_ = emscripten_webgl_get_current_context();
    if (
      emscripten_webgl_make_context_current(context_.data.webgl.context) !=
      EMSCRIPTEN_RESULT_SUCCESS
    ) {
      throw std::runtime_error("Switching WebGL context failed");
    }
#else
    throw std::runtime_error("OpenGL context provider is unsupported");
#endif
  }

  void deactivate() override {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    wglMakeCurrent(
      static_cast<HDC>(previous_device_context_),
      static_cast<HGLRC>(previous_render_context_)
    );
    previous_device_context_ = nullptr;
    previous_render_context_ = nullptr;
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    egl_context_->deactivate();
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    (void)emscripten_webgl_make_context_current(previous_webgl_context_);
    previous_webgl_context_ = 0;
#endif
  }

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  void create_wgl_context() {
    auto* const device_context =
      static_cast<HDC>(context_.data.wgl.device_context);
    auto* const share_context =
      static_cast<HGLRC>(context_.data.wgl.share_context);
    auto* context_attribs =
      reinterpret_cast<mln::core::opengl::WglCreateContextAttribs>(
        getExtensionFunctionPointer("wglCreateContextAttribsARB")
      );
    render_context_ = mln::core::opengl::create_shared_wgl_context(
      device_context, share_context,
      static_cast<HGLRC>(previous_render_context_), context_attribs
    );
  }

  void validate_wgl_context_support() {
    mln::core::opengl::validate_required_wgl_proc_addresses(
      [this](const char* name) { return getExtensionFunctionPointer(name); }
    );
  }

  void destroy_native_context() {
    if (render_context_ != nullptr) {
      wglDeleteContext(static_cast<HGLRC>(render_context_));
      render_context_ = nullptr;
    }
  }
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  void destroy_native_context() { egl_context_.reset(); }
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  void destroy_native_context() {
    if (
      context_.data.webgl.kind == MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS &&
      context_.data.webgl.context > 0
    ) {
      emscripten_webgl_destroy_context(context_.data.webgl.context);
      context_.data.webgl.context = 0;
    }
  }
#else
  void destroy_native_context() {}
#endif

  mln_opengl_context_descriptor context_{};
  uint32_t borrowed_texture_ = 0;
  mln::Size resource_size_{};
  mln::core::RenderableSlotRing ring_;

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  void* render_context_ = nullptr;
  void* previous_device_context_ = nullptr;
  void* previous_render_context_ = nullptr;
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  std::optional<mln::core::opengl::EglSharedContext> egl_context_;
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE previous_webgl_context_ = 0;
#endif
};

class OpenGLTextureSessionBackend final
    : public mln::core::TextureSessionBackend {
 public:
  OpenGLTextureSessionBackend(
    const mln_opengl_owned_texture_descriptor& descriptor, mln::Size size,
    std::size_t ring_depth
  )
      : backend_(descriptor, size, ring_depth) {}

  OpenGLTextureSessionBackend(
    const mln_opengl_borrowed_texture_descriptor& descriptor, mln::Size size
  )
      : backend_(descriptor, size) {}

  auto headless_backend() -> mln::gfx::HeadlessBackend& override {
    return backend_;
  }
  void resize(mln::Size size) override { backend_.set_ring_size(size); }

  auto set_opengl_borrowed_target(
    const mln_opengl_borrowed_texture_descriptor& descriptor
  ) -> mln_status override {
    if (!mln::core::opengl_context_matches(
          backend_.context_descriptor(), descriptor.context,
          mln::core::OpenGLContextMatch::Exact
        )) {
      mln::core::set_thread_error(
        "OpenGL texture target must name the context this session attached with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    backend_.set_borrowed_texture(
      descriptor.texture,
      mln::Size{descriptor.physical_width, descriptor.physical_height}
    );
    return MLN_STATUS_OK;
  }

  auto select_render_slot(std::size_t slot) -> mln_status override {
    return backend_.select_slot(slot) ? MLN_STATUS_OK
                                      : MLN_STATUS_INVALID_ARGUMENT;
  }

  auto record_frame_metadata(
    const mln::core::RenderFrameMetadata& frame, std::any& out_metadata
  ) -> mln_status override {
    // CPU-complete producer synchronization requires all preceding writes to
    // finish before the texture name is published. The flush runs here, on the
    // driver thread, rather than on the host thread that acquires the frame.
    auto guard = mln::gfx::BackendScope{backend_};
    const auto texture = backend_.texture();
    if (texture == 0) {
      mln::core::set_thread_error("rendered OpenGL texture is not available");
      return MLN_STATUS_NOT_READY;
    }
    backend_.finish_rendering();
    out_metadata = mln_opengl_owned_texture_frame{
      .size = sizeof(mln_opengl_owned_texture_frame),
      .generation = frame.generation,
      .width = frame.physical_width,
      .height = frame.physical_height,
      .scale_factor = frame.scale_factor,
      .frame_id = frame.frame_id,
      .texture = texture,
      .target = opengl_texture_target,
      .internal_format = opengl_internal_format,
      .format = opengl_pixel_format,
      .type = opengl_pixel_type,
    };
    return MLN_STATUS_OK;
  }

 private:
  OpenGLTextureBackend backend_;
};

}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_OPENGL;
}

auto opengl_owned_texture_attach_start(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_opengl_owned_texture_descriptor(descriptor, true);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  session->texture.mode = TextureSessionMode::Owned;
  auto copied = *descriptor;
  const auto private_target =
    copied.context.ownership == MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED;
  const auto transferred = opengl::is_transferred_webgl_canvas(copied.context);
  auto selector =
    transferred ? opengl::webgl_canvas_selector(copied.context) : std::string{};
  if (transferred) {
    opengl::configure_transferred_webgl_worker(*session, selector);
  }
  const auto driver_status = require_render_driver(
    options,
    private_target ? MLN_RENDER_DRIVER_CORE_WORKER
                   : MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    "OpenGL driver does not match its context placement"
  );
  if (driver_status != MLN_STATUS_OK) {
    return driver_status;
  }
  const auto ring_depth = private_target ? 1U : attach_ring_depth(options);
  session->initialize_backend =
    [copied, selector = std::move(selector), transferred,
     ring_depth](mln_render_session_object& target) mutable {
      if (transferred) {
        const auto context_status =
          opengl::create_transferred_webgl_context(copied.context, selector);
        if (context_status != MLN_STATUS_OK) {
          return context_status;
        }
      }
      target.texture.backend = std::make_unique<OpenGLTextureSessionBackend>(
        copied, mln::Size{target.physical_width, target.physical_height},
        ring_depth
      );
      return MLN_STATUS_OK;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = 0,
    .texture_ring_depth = ring_depth,
    .flags = MLN_RENDER_SESSION_CAPABILITY_READBACK |
             (private_target ? 0u
                             : MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
                                 MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC)
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion
  );
}

auto opengl_borrowed_texture_attach_start(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_opengl_borrowed_texture_descriptor(descriptor, true);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto driver_status = require_render_driver(
    options, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    "borrowed OpenGL textures require the caller graphics thread driver"
  );
  if (driver_status != MLN_STATUS_OK) {
    return driver_status;
  }
  if (descriptor->target != opengl_texture_target) {
    set_thread_error("OpenGL texture target must be GL_TEXTURE_2D");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_borrowed_session_extent(
    *session, descriptor->extent, descriptor->physical_width,
    descriptor->physical_height
  );
  session->texture.mode = TextureSessionMode::Borrowed;
  const auto copied = *descriptor;
  session->initialize_backend = [copied](mln_render_session_object& target) {
    target.texture.backend = std::make_unique<OpenGLTextureSessionBackend>(
      copied, mln::Size{target.physical_width, target.physical_height}
    );
    return MLN_STATUS_OK;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = 0,
    .texture_ring_depth = 0,
    .flags = 0
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion
  );
}

auto opengl_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status {
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::BorrowedTexture, completion
  );
  if (submission_status != MLN_STATUS_OK) {
    return submission_status;
  }
  const auto descriptor_status =
    validate_opengl_borrowed_texture_descriptor(descriptor, true);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  if (descriptor->target != opengl_texture_target) {
    set_thread_error("OpenGL texture target must be GL_TEXTURE_2D");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  const auto copied = *descriptor;
  return enqueue_driver_operation(
    session,
    [copied](mln_render_session_object& target) {
      return render_session_set_target(
        target.self, RetargetTargetKind::BorrowedTexture, copied.extent,
        copied.physical_width, copied.physical_height,
        [&copied](mln_render_session_object& live) {
          return live.texture.backend->set_opengl_borrowed_target(copied);
        }
      );
    },
    completion
  );
}

}  // namespace mln::core
