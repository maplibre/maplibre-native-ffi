#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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
#include "render/opengl/opengl_texture_session.hpp"
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

using mln::core::OpenGLBorrowedTarget;

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

  // Every texture, the caller's or the session's, reaches the host through an
  // acquired frame, and recording the frame's metadata completes the rendering
  // first.
  void swap() override {}

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
    const mln_open_gl_owned_texture_descriptor& descriptor, mln::Size size,
    std::size_t ring_depth
  )
      : mln::gl::RendererBackend(mln::core::opengl::session_context_mode),
        mln::gfx::HeadlessBackend(size),
        context_(descriptor.context),
        ring_(ring_depth) {}

  OpenGLTextureBackend(const OpenGLBorrowedTarget& target, mln::Size size)
      : mln::gl::RendererBackend(mln::core::opengl::session_context_mode),
        mln::gfx::HeadlessBackend(size),
        context_(target.descriptor.context),
        borrowed_textures_(borrowed_texture_names(target)),
        ring_(target.textures.size()) {}

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
        getContext<mln::gl::Context>(), current_size,
        borrowed_textures_.empty() ? 0U : borrowed_textures_[ring_.selected()]
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

#if defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  // Runs when another pooled session rendered in this context since this one
  // did and left its own GL state behind. A new gl::Context needs none of it,
  // since it starts with every state dirty. The renderbuffer binding is
  // private, but MapLibre unbinds every renderbuffer it creates before it
  // returns, so it is still zero.
  static void forget_gl_state(mln::gl::Context& gl) {
    gl.setDirtyState();
    gl.bindFramebuffer.setDirty();
    gl.viewport.setDirty();
    // DrawableGL::draw compares the program with getCurrentValue(), which
    // ignores the dirty flag, so only a program that no drawable uses makes
    // it call glUseProgram again, until maplibre/maplibre-native#4866 lands.
    gl.program = 0;
  }
#endif

  auto texture() -> uint32_t {
    auto& renderable =
      getDefaultRenderable().getResource<OpenGLTextureRenderableResource>();
    return renderable.texture();
  }

  auto select_slot(std::size_t slot) -> bool {
    if (!ring_.select(slot, getSize(), resource)) return false;
    resource_size_ = ring_.selected_size();
    return true;
  }

  void set_ring_size(mln::Size new_size) { setRenderableSize(new_size); }

  void finish_rendering() { getContext<mln::gl::Context>().finish(); }

  auto supports_fence_sync() -> bool {
    if (
      context_.ownership != MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED ||
      context_.platform == MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL
    )
      return false;
    auto guard = mln::gfx::BackendScope{*this};
    for (const auto* name :
         {"glFenceSync", "glWaitSync", "glDeleteSync", "glIsSync"}) {
      if (getExtensionFunctionPointer(name) == nullptr) return false;
    }
    auto major = mln::platform::GLint{};
    auto minor = mln::platform::GLint{};
    mln::platform::glGetIntegerv(GL_MAJOR_VERSION, &major);
    mln::platform::glGetIntegerv(GL_MINOR_VERSION, &minor);
    const auto* version =
      reinterpret_cast<const char*>(mln::platform::glGetString(GL_VERSION));
    const auto gles =
      version != nullptr &&
      std::string_view{version}.find("OpenGL ES") != std::string_view::npos;
    if (major > 3 || (major == 3 && (gles || minor >= 2))) return true;
    auto count = mln::platform::GLint{};
    mln::platform::glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (auto index = mln::platform::GLint{}; index < count; ++index) {
      const auto* extension = reinterpret_cast<const char*>(
        mln::platform::glGetStringi(GL_EXTENSIONS, static_cast<uint32_t>(index))
      );
      if (extension != nullptr && std::string_view{extension} == "GL_ARB_sync")
        return true;
    }
    return false;
  }

  void set_borrowed_textures(const OpenGLBorrowedTarget& target) {
    borrowed_textures_ = borrowed_texture_names(target);
    // setSize() drops the selected renderable unconditionally, and the reset
    // drops the parked ones, so every slot rebuilds its framebuffer against
    // its new texture even when the size is unchanged. No context is made
    // current: the framebuffer and renderbuffer names go to the context's
    // abandoned lists and are deleted on the next render.
    setSize(
      mln::Size{
        target.descriptor.physical_width, target.descriptor.physical_height
      }
    );
    ring_.reset(borrowed_textures_.size());
    resource_size_ = {};
  }

  [[nodiscard]] auto context_descriptor() const
    -> const mln_open_gl_context_descriptor& {
    return context_;
  }

  [[nodiscard]] auto renders_borrowed_textures() const -> bool {
    return !borrowed_textures_.empty();
  }

 private:
  [[nodiscard]] auto has_native_context() const -> bool {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    return render_context_ != nullptr;
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    return egl_context_.has_value();
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    return context_.data.web_gl.context > 0;
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
    if (dedicated()) {
      activate_dedicated_wgl();
      return;
    }
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
    if (egl_context_->take_state_lost() && context) {
      forget_gl_state(static_cast<mln::gl::Context&>(*context));
    }
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
    previous_webgl_context_ = emscripten_webgl_get_current_context();
    if (
      emscripten_webgl_make_context_current(context_.data.web_gl.context) !=
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
    // A dedicated context stays current between renders.
    if (dedicated()) {
      return;
    }
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
  [[nodiscard]] auto dedicated() const -> bool {
    return context_.ownership == MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
  }

  // Makes this session's context current and leaves it current. A render that
  // finds it still current makes no WGL call. Another session or the host may
  // have made its own context current on this thread since the last render, so
  // this asks WGL instead of remembering.
  void activate_dedicated_wgl() {
    auto* const device_context =
      static_cast<HDC>(context_.data.wgl.device_context);
    if (
      render_context_ != nullptr && wglGetCurrentContext() == render_context_ &&
      wglGetCurrentDC() == device_context
    ) {
      return;
    }
    if (render_context_ == nullptr) {
      create_wgl_context();
    }
    if (
      wglMakeCurrent(device_context, static_cast<HGLRC>(render_context_)) == 0
    ) {
      throw std::runtime_error("Switching OpenGL WGL context failed");
    }
    // A context that fails validation must not stay current, or the next
    // activation would find it current and skip validating it again.
    try {
      validate_wgl_context_support();
    } catch (...) {
      (void)wglMakeCurrent(nullptr, nullptr);
      throw;
    }
  }

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
      // Teardown leaves a dedicated context current on this thread, and WGL
      // only retires it once nothing holds it.
      if (dedicated() && wglGetCurrentContext() == render_context_) {
        (void)wglMakeCurrent(nullptr, nullptr);
      }
      wglDeleteContext(static_cast<HGLRC>(render_context_));
      render_context_ = nullptr;
    }
  }
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  void destroy_native_context() { egl_context_.reset(); }
#elif defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  void destroy_native_context() {
    if (
      context_.data.web_gl.kind == MLN_WEB_GL_CONTEXT_TRANSFERRED_CANVAS &&
      context_.data.web_gl.context > 0
    ) {
      emscripten_webgl_destroy_context(context_.data.web_gl.context);
      context_.data.web_gl.context = 0;
    }
  }
#else
  void destroy_native_context() {}
#endif

  static auto borrowed_texture_names(const OpenGLBorrowedTarget& target)
    -> std::vector<uint32_t> {
    auto names = std::vector<uint32_t>{};
    names.reserve(target.textures.size());
    for (const auto& entry : target.textures) names.push_back(entry.texture);
    return names;
  }

  mln_open_gl_context_descriptor context_{};
  // The caller's texture for each slot, or empty for a session-owned ring.
  std::vector<uint32_t> borrowed_textures_;
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
    const mln_open_gl_owned_texture_descriptor& descriptor, mln::Size size,
    std::size_t ring_depth, mln_gpu_sync_kind producer_sync_kind
  )
      : backend_(descriptor, size, ring_depth),
        producer_sync_kind_(producer_sync_kind) {}

  OpenGLTextureSessionBackend(
    const OpenGLBorrowedTarget& target, mln::Size size,
    mln_gpu_sync_kind producer_sync_kind
  )
      : backend_(target, size), producer_sync_kind_(producer_sync_kind) {}

  ~OpenGLTextureSessionBackend() noexcept override {
    try {
      if (has_producer_fences_) {
        auto guard = mln::gfx::BackendScope{backend_};
        for (auto fence : producer_fences_) {
          if (fence != nullptr) mln::platform::glDeleteSync(fence);
        }
      }
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error("destroying OpenGL producer fences failed");
    }
  }

  auto initialize_sync() -> mln_status {
    fence_sync_supported_ = backend_.supports_fence_sync();
    if (
      producer_sync_kind_ == MLN_GPU_SYNC_OPEN_GL_FENCE &&
      !fence_sync_supported_
    ) {
      mln::core::set_thread_error(
        "OpenGL context does not support shared GLsync objects"
      );
      return MLN_STATUS_UNSUPPORTED;
    }
    return MLN_STATUS_OK;
  }

  auto supports_consumer_sync(mln_gpu_sync_kind kind) -> bool override {
    return kind == MLN_GPU_SYNC_CPU_COMPLETE ||
           (kind == MLN_GPU_SYNC_OPEN_GL_FENCE && fence_sync_supported_);
  }

  auto release_consumer_sync(const mln_gpu_sync& sync) -> mln_status override {
    if (sync.kind == MLN_GPU_SYNC_CPU_COMPLETE) return MLN_STATUS_OK;
    if (!supports_consumer_sync(sync.kind)) return MLN_STATUS_UNSUPPORTED;
    try {
      auto guard = mln::gfx::BackendScope{backend_};
      auto* fence = reinterpret_cast<mln::platform::GLsync>(
        static_cast<uintptr_t>(sync.handle)
      );
      if (mln::platform::glIsSync(fence) == GL_FALSE) {
        mln::core::set_thread_error(
          "consumer fence is not in the session share group"
        );
        return MLN_STATUS_INVALID_ARGUMENT;
      }
      mln::platform::glWaitSync(fence, 0, GL_TIMEOUT_IGNORED);
      mln::platform::glDeleteSync(fence);
      if (mln::platform::glGetError() != GL_NO_ERROR) {
        mln::core::set_thread_error(
          "waiting for the consumer OpenGL fence failed"
        );
        return MLN_STATUS_NATIVE_ERROR;
      }
      return MLN_STATUS_OK;
    } catch (const std::exception& exception) {
      mln::core::set_thread_error(exception);
    } catch (...) {
      mln::core::set_thread_error(
        "waiting for the consumer OpenGL fence failed"
      );
    }
    return MLN_STATUS_TARGET_LOST;
  }

  auto producer_sync() const -> mln_gpu_sync override { return producer_sync_; }

  auto headless_backend() -> mln::gfx::HeadlessBackend& override {
    return backend_;
  }
  void resize(mln::Size size) override { backend_.set_ring_size(size); }

  auto set_opengl_borrowed_target(const OpenGLBorrowedTarget& target)
    -> mln_status override {
    if (!mln::core::opengl_context_matches(
          backend_.context_descriptor(), target.descriptor.context,
          mln::core::OpenGLContextMatch::Exact
        )) {
      mln::core::set_thread_error(
        "OpenGL texture target must name the context this session attached with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    backend_.set_borrowed_textures(target);
    return MLN_STATUS_OK;
  }

  auto select_render_slot(std::size_t slot) -> mln_status override {
    return backend_.select_slot(slot) ? MLN_STATUS_OK
                                      : MLN_STATUS_INVALID_ARGUMENT;
  }

  auto record_frame_metadata(
    const mln::core::RenderFrameMetadata& frame, std::any& out_metadata
  ) -> mln_status override {
    auto guard = mln::gfx::BackendScope{backend_};
    const auto texture = backend_.texture();
    if (texture == 0) {
      mln::core::set_thread_error("rendered OpenGL texture is not available");
      return MLN_STATUS_NOT_READY;
    }
    if (producer_sync_kind_ == MLN_GPU_SYNC_OPEN_GL_FENCE) {
      auto& fence = producer_fences_.at(frame.slot);
      if (fence != nullptr) mln::platform::glDeleteSync(fence);
      fence = mln::platform::glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
      if (fence == nullptr) {
        mln::core::set_thread_error(
          "creating the producer OpenGL fence failed"
        );
        return MLN_STATUS_NATIVE_ERROR;
      }
      has_producer_fences_ = true;
      mln::platform::glFlush();
      producer_sync_ = mln_gpu_sync{
        sizeof(mln_gpu_sync), MLN_GPU_SYNC_OPEN_GL_FENCE,
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(fence))
      };
    } else {
      backend_.finish_rendering();
    }
    // The host chose a borrowed texture's format, which the session cannot
    // query before OpenGL ES 3.1.
    const auto owned = !backend_.renders_borrowed_textures();
    out_metadata = mln_open_gl_texture_frame{
      .size = sizeof(mln_open_gl_texture_frame),
      .generation = frame.generation,
      .width = frame.physical_width,
      .height = frame.physical_height,
      .scale_factor = frame.scale_factor,
      .frame_id = frame.frame_id,
      .slot = frame.slot,
      .texture = texture,
      .target = opengl_texture_target,
      .internal_format = owned ? opengl_internal_format : 0U,
      .format = owned ? opengl_pixel_format : 0U,
      .type = owned ? opengl_pixel_type : 0U,
    };
    return MLN_STATUS_OK;
  }

 private:
  OpenGLTextureBackend backend_;
  mln_gpu_sync_kind producer_sync_kind_;
  mln_gpu_sync producer_sync_{
    sizeof(mln_gpu_sync), MLN_GPU_SYNC_CPU_COMPLETE, 0
  };
  std::array<mln::platform::GLsync, mln::core::max_texture_ring_depth>
    producer_fences_{};
  bool has_producer_fences_ = false;
  bool fence_sync_supported_ = false;
};

}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_OPEN_GL;
}

auto create_opengl_owned_texture_backend(
  const mln_open_gl_owned_texture_descriptor& descriptor, mln::Size size,
  std::size_t ring_depth, std::unique_ptr<TextureSessionBackend>& out_backend
) -> mln_status {
  auto backend = std::make_unique<OpenGLTextureSessionBackend>(
    descriptor, size, ring_depth, MLN_GPU_SYNC_CPU_COMPLETE
  );
  const auto status = backend->initialize_sync();
  out_backend = std::move(backend);
  return status;
}

auto opengl_owned_texture_attach_start(
  mln_map map, const mln_open_gl_owned_texture_descriptor* descriptor,
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
  if (
    options != nullptr &&
    descriptor->context.ownership == MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED &&
    descriptor->context.platform != MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL &&
    options->producer_sync_kind == MLN_GPU_SYNC_OPEN_GL_FENCE
  ) {
    session->producer_sync_kind = MLN_GPU_SYNC_OPEN_GL_FENCE;
  }
  auto copied = *descriptor;
  const auto private_target =
    copied.context.ownership != MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED;
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
      auto backend = std::make_unique<OpenGLTextureSessionBackend>(
        copied, mln::Size{target.physical_width, target.physical_height},
        ring_depth, target.producer_sync_kind
      );
      const auto status = backend->initialize_sync();
      target.texture.backend = std::move(backend);
      return status;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = ring_depth,
    .flags = MLN_RENDER_SESSION_CAPABILITY_READBACK |
             (private_target ? 0u
                             : MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
                                 MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC)
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_open_gl_owned_texture>()
  );
}

auto opengl_borrowed_texture_attach_start(
  mln_map map, const mln_open_gl_borrowed_texture_descriptor* descriptor,
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
  if (
    options != nullptr &&
    descriptor->context.platform != MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL &&
    options->producer_sync_kind == MLN_GPU_SYNC_OPEN_GL_FENCE
  ) {
    session->producer_sync_kind = MLN_GPU_SYNC_OPEN_GL_FENCE;
  }
  session->initialize_backend = [target = OpenGLBorrowedTarget{*descriptor}](
                                  mln_render_session_object& live
                                ) {
    auto backend = std::make_unique<OpenGLTextureSessionBackend>(
      target, mln::Size{live.physical_width, live.physical_height},
      live.producer_sync_kind
    );
    const auto status = backend->initialize_sync();
    live.texture.backend = std::move(backend);
    return status;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = static_cast<uint32_t>(descriptor->texture_count),
    .flags = MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
             MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_open_gl_borrowed_texture>()
  );
}

auto opengl_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_open_gl_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status {
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::BorrowedTexture, completion, checked_completion
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
  return enqueue_borrowed_texture_retarget(
    session, descriptor->texture_count, descriptor->extent,
    descriptor->physical_width, descriptor->physical_height,
    [target =
       OpenGLBorrowedTarget{*descriptor}](mln_render_session_object& live) {
      return live.texture.backend->set_opengl_borrowed_target(target);
    },
    std::move(checked_completion),
    valueless_completion<
      &mln_render_session_set_open_gl_borrowed_texture_target>()
  );
}

}  // namespace mln::core
