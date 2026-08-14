#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/gfx/headless_backend.hpp>
#include <mbgl/gfx/renderable.hpp>
#include <mbgl/gl/context.hpp>
#include <mbgl/gl/defines.hpp>
#include <mbgl/gl/framebuffer.hpp>
#include <mbgl/gl/renderable_resource.hpp>
#include <mbgl/gl/renderbuffer_resource.hpp>
#include <mbgl/gl/renderer_backend.hpp>
#include <mbgl/gl/texture2d.hpp>
#include <mbgl/platform/gl_functions.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/size.hpp>

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
#include "render/opengl/wgl_common.hpp"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"

namespace {

constexpr auto opengl_texture_target = uint32_t{GL_TEXTURE_2D};
constexpr auto opengl_internal_format = uint32_t{GL_RGBA8};
constexpr auto opengl_pixel_format = uint32_t{GL_RGBA};
constexpr auto opengl_pixel_type = uint32_t{GL_UNSIGNED_BYTE};

#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
struct WebGLWorkerCall {
  std::function<void()> function;
};

auto run_webgl_worker(void* opaque) -> void* {
  auto call =
    std::unique_ptr<WebGLWorkerCall>{static_cast<WebGLWorkerCall*>(opaque)};
  call->function();
  return nullptr;
}

auto configure_transferred_webgl_worker(
  mln_render_session_object& session, std::string selector
) -> void {
  auto thread = std::make_shared<pthread_t>();
  session.start_worker =
    [thread, selector = std::move(selector)](std::function<void()> function) {
      auto attributes = pthread_attr_t{};
      if (pthread_attr_init(&attributes) != 0) {
        mln::core::set_thread_error("creating WebGL worker attributes failed");
        return MLN_STATUS_NATIVE_ERROR;
      }
      if (
        emscripten_pthread_attr_settransferredcanvases(
          &attributes, selector.c_str()
        ) != 0
      ) {
        pthread_attr_destroy(&attributes);
        mln::core::set_thread_error("transferring the WebGL canvas failed");
        return MLN_STATUS_NATIVE_ERROR;
      }
      auto call =
        std::make_unique<WebGLWorkerCall>(WebGLWorkerCall{std::move(function)});
      const auto result =
        pthread_create(thread.get(), &attributes, run_webgl_worker, call.get());
      pthread_attr_destroy(&attributes);
      if (result != 0) {
        mln::core::set_thread_error("creating the WebGL worker failed");
        return MLN_STATUS_NATIVE_ERROR;
      }
      static_cast<void>(call.release());
      return MLN_STATUS_OK;
    };
  session.join_worker = [thread]() {
    static_cast<void>(pthread_join(*thread, nullptr));
  };
}
#endif

[[noreturn]] void throw_opengl_framebuffer_error() {
  switch (mbgl::platform::glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
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
    mbgl::platform::glCheckFramebufferStatus(GL_FRAMEBUFFER) !=
    GL_FRAMEBUFFER_COMPLETE
  ) {
    throw_opengl_framebuffer_error();
  }
}

class OpenGLTextureRenderableResource final
    : public mbgl::gl::RenderableResource {
 public:
  OpenGLTextureRenderableResource(
    mbgl::gl::Context& context_, mbgl::Size size_, uint32_t borrowed_texture
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

  auto readStillImage() -> mbgl::PremultipliedImage {
    bind();
    return context.readFramebuffer<mbgl::PremultipliedImage>(size);
  }

  auto texture() -> uint32_t {
    ensure_resources();
    if (borrowed_texture_ != 0) {
      return borrowed_texture_;
    }
    return static_cast<mbgl::gl::Texture2D&>(*texture_).getTextureID();
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
        mbgl::gfx::TexturePixelType::RGBA,
        mbgl::gfx::TextureChannelDataType::UnsignedByte
      );
      texture_->setSamplerConfiguration(
        {.filter = mbgl::gfx::TextureFilterType::Linear,
         .wrapU = mbgl::gfx::TextureWrapType::Clamp,
         .wrapV = mbgl::gfx::TextureWrapType::Clamp}
      );
      texture_->create();
      texture_id = static_cast<mbgl::gl::Texture2D&>(*texture_).getTextureID();
    }

    depth_stencil_ =
      context
        .createRenderbuffer<mbgl::gfx::RenderbufferPixelType::DepthStencil>(
          size
        );
    auto framebuffer_id = mbgl::platform::GLuint{};
    mbgl::platform::glGenFramebuffers(1, &framebuffer_id);
    auto framebuffer = mbgl::gl::Framebuffer{
      .size = size,
      .framebuffer =
        mbgl::gl::UniqueFramebuffer{std::move(framebuffer_id), {&context}}
    };
    context.bindFramebuffer = framebuffer.framebuffer;
    mbgl::platform::glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_id, 0
    );
    auto& depth_stencil_resource =
      depth_stencil_->getResource<mbgl::gl::RenderbufferResource>();
#ifdef GL_DEPTH_STENCIL_ATTACHMENT
    mbgl::platform::glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
      depth_stencil_resource.renderbuffer
    );
#else
    mbgl::platform::glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
      depth_stencil_resource.renderbuffer
    );
    mbgl::platform::glFramebufferRenderbuffer(
      GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
      depth_stencil_resource.renderbuffer
    );
#endif
    check_opengl_framebuffer();
    framebuffer_ = std::move(framebuffer);
  }

  mbgl::gl::Context& context;
  mbgl::Size size;
  uint32_t borrowed_texture_ = 0;
  mbgl::gfx::Texture2DPtr texture_;
  std::optional<
    mbgl::gfx::Renderbuffer<mbgl::gfx::RenderbufferPixelType::DepthStencil>>
    depth_stencil_;
  std::optional<mbgl::gl::Framebuffer> framebuffer_;
};

class OpenGLTextureBackend final : public mbgl::gl::RendererBackend,
                                   public mbgl::gfx::HeadlessBackend {
 public:
  OpenGLTextureBackend(
    const mln_opengl_owned_texture_descriptor& descriptor, mbgl::Size size,
    std::size_t ring_depth
  )
      : mbgl::gl::RendererBackend(mln::core::opengl::session_context_mode),
        mbgl::gfx::HeadlessBackend(size),
        context_(descriptor.context),
        slot_resources_(ring_depth),
        slot_sizes_(ring_depth) {}

  OpenGLTextureBackend(
    const mln_opengl_borrowed_texture_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::gl::RendererBackend(mln::core::opengl::session_context_mode),
        mbgl::gfx::HeadlessBackend(size),
        context_(descriptor.context),
        borrowed_texture_(descriptor.texture) {}

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
      slot_resources_.clear();
      context.reset();
    };
    if (has_native_context()) {
      auto guard = mbgl::gfx::BackendScope{*this};
      cleanup();
    } else {
      cleanup();
    }
    getThreadPool().runRenderJobs(true);
    destroy_native_context();
  }

  auto getDefaultRenderable() -> mbgl::gfx::Renderable& override {
    const auto current_size = getSize();
    if (!resource || resource_size_ != current_size) {
      resource = std::make_unique<OpenGLTextureRenderableResource>(
        getContext<mbgl::gl::Context>(), current_size, borrowed_texture_
      );
      resource_size_ = current_size;
      if (!slot_sizes_.empty()) slot_sizes_[selected_slot_] = current_size;
    }
    return *this;
  }

  auto readStillImage() -> mbgl::PremultipliedImage override {
    auto& renderable =
      getDefaultRenderable().getResource<OpenGLTextureRenderableResource>();
    return renderable.readStillImage();
  }

  auto getRendererBackend() -> mbgl::gfx::RendererBackend* override {
    return this;
  }

  void updateAssumedState() override {
    assumeFramebufferBinding(
      mbgl::gl::RendererBackend::ImplicitFramebufferBinding
    );
  }

  auto texture() -> uint32_t {
    auto& renderable =
      getDefaultRenderable().getResource<OpenGLTextureRenderableResource>();
    return renderable.texture();
  }

  auto texture_at(std::size_t slot) -> uint32_t {
    if (slot >= slot_resources_.size()) return 0;
    if (slot == selected_slot_) return texture();
    if (slot_resources_[slot] == nullptr) {
      const auto previous = selected_slot_;
      if (!select_slot(slot)) return 0;
      const auto value = texture();
      static_cast<void>(select_slot(previous));
      return value;
    }
    return static_cast<OpenGLTextureRenderableResource*>(
             slot_resources_[slot].get()
    )
      ->texture();
  }

  auto select_slot(std::size_t slot) -> bool {
    if (slot >= slot_resources_.size()) return false;
    if (slot == selected_slot_) {
      if (resource != nullptr && slot_sizes_[slot] != size) resource.reset();
      return true;
    }
    slot_resources_[selected_slot_] = std::move(resource);
    if (slot_resources_[slot] != nullptr && slot_sizes_[slot] != size) {
      slot_resources_[slot].reset();
    }
    resource = std::move(slot_resources_[slot]);
    selected_slot_ = slot;
    resource_size_ = slot_sizes_[slot];
    return true;
  }

  void set_ring_size(mbgl::Size new_size) { size = new_size; }

  void finish_rendering() { getContext<mbgl::gl::Context>().finish(); }

  // Renders into a different caller-owned texture from here on, keeping the
  // session's context and everything the renderer built in it.
  void set_borrowed_texture(uint32_t texture, mbgl::Size new_size) {
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
    -> mbgl::gl::ProcAddress override {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    using GetProcAddressFunction = PROC(WINAPI*)(LPCSTR);
    auto* loader = reinterpret_cast<GetProcAddressFunction>(
      context_.data.wgl.get_proc_address
    );
    if (loader != nullptr) {
      auto* proc = loader(name);
      if (mln::core::opengl::is_valid_wgl_proc_address(proc)) {
        return reinterpret_cast<mbgl::gl::ProcAddress>(proc);
      }
    }
    auto* proc = wglGetProcAddress(name);
    if (mln::core::opengl::is_valid_wgl_proc_address(proc)) {
      return reinterpret_cast<mbgl::gl::ProcAddress>(proc);
    }
    return reinterpret_cast<mbgl::gl::ProcAddress>(
      mln::core::opengl::get_opengl32_proc_address(name)
    );
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    return reinterpret_cast<mbgl::gl::ProcAddress>(
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
      egl_context_.emplace(
        context_.data.egl, MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED
      );
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
  mbgl::Size resource_size_{};
  std::vector<std::unique_ptr<mbgl::gfx::RenderableResource>> slot_resources_;
  std::vector<mbgl::Size> slot_sizes_;
  std::size_t selected_slot_ = 0;

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
    const mln_opengl_owned_texture_descriptor& descriptor, mbgl::Size size,
    std::size_t ring_depth
  )
      : backend_(descriptor, size, ring_depth) {}

  OpenGLTextureSessionBackend(
    const mln_opengl_borrowed_texture_descriptor& descriptor, mbgl::Size size
  )
      : backend_(descriptor, size) {}

  auto headless_backend() -> mbgl::gfx::HeadlessBackend& override {
    return backend_;
  }
  void resize(mbgl::Size size) override { backend_.set_ring_size(size); }

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
      mbgl::Size{descriptor.physical_width, descriptor.physical_height}
    );
    return MLN_STATUS_OK;
  }

  auto after_render(mln_render_session_object& texture, bool& out_rendered)
    -> mln_status override {
    texture.texture.rendered_native_texture =
      reinterpret_cast<void*>(static_cast<uintptr_t>(backend_.texture()));
    out_rendered = true;
    return MLN_STATUS_OK;
  }

  auto select_render_slot(std::size_t slot) -> mln_status override {
    return backend_.select_slot(slot) ? MLN_STATUS_OK
                                      : MLN_STATUS_INVALID_ARGUMENT;
  }

  auto copy_slot_metadata(
    const mln_render_session_object& texture, std::size_t slot,
    std::any& out_metadata
  ) -> mln_status override {
    // CPU-complete producer synchronization requires all preceding writes to
    // finish before the texture name is published.
    auto guard = mbgl::gfx::BackendScope{backend_};
    backend_.finish_rendering();
    out_metadata = mln_opengl_owned_texture_frame{
      .size = sizeof(mln_opengl_owned_texture_frame),
      .generation = texture.generation,
      .width = texture.physical_width,
      .height = texture.physical_height,
      .scale_factor = texture.scale_factor,
      .frame_id = texture.frame_generation,
      .texture = backend_.texture_at(slot),
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
  mln_render_session* out_session, mln_operation* out_operation
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
  session->texture.api_kind = TextureSessionApi::OpenGL;
  session->texture.mode = TextureSessionMode::Owned;
  auto copied = *descriptor;
#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
  const auto transferred =
    copied.context.platform == MLN_OPENGL_CONTEXT_PLATFORM_WEBGL &&
    copied.context.data.webgl.kind == MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS;
  auto selector = std::string{};
  if (transferred) {
    const auto view = copied.context.data.webgl.canvas_selector;
    selector.assign(static_cast<const char*>(view.data), view.size);
    configure_transferred_webgl_worker(*session, selector);
  }
#else
  constexpr auto transferred = false;
  auto selector = std::string{};
#endif
  if (
    options != nullptr &&
    options->driver != (transferred ? MLN_RENDER_DRIVER_CORE_WORKER
                                    : MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD)
  ) {
    set_thread_error("OpenGL driver does not match its context placement");
    return MLN_STATUS_UNSUPPORTED;
  }
  const auto ring_depth = std::clamp(
    options == nullptr ? 1u : options->requested_texture_ring_depth, 1u, 3u
  );
  session->initialize_backend =
    [copied, selector = std::move(selector), transferred,
     ring_depth](mln_render_session_object& target) mutable {
      (void)selector;
      (void)transferred;
#if defined(MLN_FFI_OPENGL_PROVIDER_WEBGL)
      if (transferred) {
        auto attributes = EmscriptenWebGLContextAttributes{};
        emscripten_webgl_init_context_attributes(&attributes);
        attributes.majorVersion = 2;
        attributes.proxyContextToMainThread =
          EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;
        const auto context =
          emscripten_webgl_create_context(selector.c_str(), &attributes);
        if (context <= 0) {
          set_thread_error("creating the transferred WebGL 2 context failed");
          return MLN_STATUS_NATIVE_ERROR;
        }
        copied.context.data.webgl.context = context;
      }
#endif
      target.texture.backend = std::make_unique<OpenGLTextureSessionBackend>(
        copied, mbgl::Size{target.physical_width, target.physical_height},
        ring_depth
      );
      return MLN_STATUS_OK;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = options == nullptr ? 0u : options->driver,
    .texture_ring_depth = ring_depth,
    .flags = MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
             MLN_RENDER_SESSION_CAPABILITY_READBACK |
             MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, out_operation
  );
}

auto opengl_borrowed_texture_attach_start(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
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
  if (
    options != nullptr &&
    options->driver != MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  ) {
    set_thread_error(
      "borrowed OpenGL textures require the caller graphics thread driver"
    );
    return MLN_STATUS_UNSUPPORTED;
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
  session->texture.api_kind = TextureSessionApi::OpenGL;
  session->texture.mode = TextureSessionMode::Borrowed;
  const auto copied = *descriptor;
  session->initialize_backend = [copied](mln_render_session_object& target) {
    target.texture.backend = std::make_unique<OpenGLTextureSessionBackend>(
      copied, mbgl::Size{target.physical_width, target.physical_height}
    );
    return MLN_STATUS_OK;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = options == nullptr ? 0u : options->driver,
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_READBACK
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, out_operation
  );
}

auto opengl_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  mln_operation* out_operation
) -> mln_status {
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
    session, RENDER_OPERATION_MAINTENANCE,
    [copied](mln_render_session_object& target) {
      return render_session_set_target(
        target.self, RetargetTargetKind::BorrowedTexture, copied.extent,
        copied.physical_width, copied.physical_height,
        [&copied](mln_render_session_object& live) {
          return live.texture.backend->set_opengl_borrowed_target(copied);
        }
      );
    },
    out_operation
  );
}

}  // namespace mln::core
