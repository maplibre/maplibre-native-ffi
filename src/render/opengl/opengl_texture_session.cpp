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
#endif

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c/base.h"
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

auto validate_metal_owned_descriptor(
  const mln_metal_owned_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_metal_owned_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_metal_owned_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return mln::core::validate_metal_context(descriptor->context, true);
}

auto validate_metal_borrowed_descriptor(
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_metal_borrowed_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_metal_borrowed_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  if (descriptor->texture == nullptr) {
    mln::core::set_thread_error("Metal texture must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_vulkan_owned_descriptor(
  const mln_vulkan_owned_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_vulkan_owned_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_vulkan_owned_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return mln::core::validate_vulkan_context(
    descriptor->context, "Vulkan handles must not be null"
  );
}

auto validate_vulkan_borrowed_descriptor(
  const mln_vulkan_borrowed_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_vulkan_borrowed_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_vulkan_borrowed_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status = mln::core::validate_vulkan_context(
    descriptor->context, "Vulkan handles must not be null"
  );
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->image == nullptr || descriptor->image_view == nullptr) {
    mln::core::set_thread_error("Vulkan handles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->format == 0 || descriptor->final_layout == 0) {
    mln::core::set_thread_error(
      "Vulkan format and final_layout must be specified"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_opengl_owned_descriptor(
  const mln_opengl_owned_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_opengl_owned_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_opengl_owned_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return mln::core::validate_opengl_context(descriptor->context, true);
}

auto validate_opengl_borrowed_descriptor(
  const mln_opengl_borrowed_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_opengl_borrowed_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_opengl_borrowed_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status =
    mln::core::validate_opengl_context(descriptor->context, true);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->texture == 0 || descriptor->target == 0) {
    mln::core::set_thread_error("OpenGL texture and target must be specified");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->target != opengl_texture_target) {
    mln::core::set_thread_error("OpenGL texture target must be GL_TEXTURE_2D");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_frame(
  mln_render_session* texture, const void* frame, size_t frame_size,
  const char* message
) -> mln_status {
  const auto status = mln::core::validate_texture(texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (frame == nullptr || frame_size == 0) {
    mln::core::set_thread_error(message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

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

  void swap() override { context.finish(); }

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
    const mln_opengl_owned_texture_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::gl::RendererBackend(mbgl::gfx::ContextMode::Shared),
        mbgl::gfx::HeadlessBackend(size),
        context_(descriptor.context) {}

  OpenGLTextureBackend(
    const mln_opengl_borrowed_texture_descriptor& descriptor, mbgl::Size size
  )
      : mbgl::gl::RendererBackend(mbgl::gfx::ContextMode::Shared),
        mbgl::gfx::HeadlessBackend(size),
        context_(descriptor.context),
        borrowed_texture_(descriptor.texture) {}

  OpenGLTextureBackend(const OpenGLTextureBackend&) = delete;
  auto operator=(const OpenGLTextureBackend&) -> OpenGLTextureBackend& = delete;
  OpenGLTextureBackend(OpenGLTextureBackend&&) = delete;
  auto operator=(OpenGLTextureBackend&&) -> OpenGLTextureBackend& = delete;

  ~OpenGLTextureBackend() override {
    auto cleanup = [this] {
      resource.reset();
      context.reset();
    };
    if (has_native_context()) {
      auto guard = mbgl::gfx::BackendScope{
        *this, mbgl::gfx::BackendScope::ScopeType::Implicit
      };
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

 private:
  [[nodiscard]] auto has_native_context() const -> bool {
#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
    return render_context_ != nullptr;
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
    return egl_context_.has_value();
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
      egl_context_.emplace(context_.data.egl);
    }
    egl_context_->activate_pbuffer();
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
#else
  void destroy_native_context() {}
#endif

  mln_opengl_context_descriptor context_{};
  uint32_t borrowed_texture_ = 0;
  mbgl::Size resource_size_{};

#if defined(MLN_FFI_OPENGL_PROVIDER_WGL)
  void* render_context_ = nullptr;
  void* previous_device_context_ = nullptr;
  void* previous_render_context_ = nullptr;
#elif defined(MLN_FFI_OPENGL_PROVIDER_EGL)
  std::optional<mln::core::opengl::EglSharedContext> egl_context_;
#endif
};

class OpenGLTextureSessionBackend final
    : public mln::core::TextureSessionBackend {
 public:
  OpenGLTextureSessionBackend(
    const mln_opengl_owned_texture_descriptor& descriptor, mbgl::Size size
  )
      : backend_(descriptor, size) {}

  OpenGLTextureSessionBackend(
    const mln_opengl_borrowed_texture_descriptor& descriptor, mbgl::Size size
  )
      : backend_(descriptor, size) {}

  auto headless_backend() -> mbgl::gfx::HeadlessBackend& override {
    return backend_;
  }

  auto after_render(mln_render_session& texture) -> mln_status override {
    texture.texture.rendered_native_texture =
      reinterpret_cast<void*>(static_cast<uintptr_t>(backend_.texture()));
    return MLN_STATUS_OK;
  }

  auto acquire_opengl_owned_frame(
    const mln_render_session& texture, mln_opengl_owned_texture_frame& out_frame
  ) -> mln_status override {
    out_frame = mln_opengl_owned_texture_frame{
      .size = sizeof(mln_opengl_owned_texture_frame),
      .generation = texture.generation,
      .width = texture.physical_width,
      .height = texture.physical_height,
      .scale_factor = texture.scale_factor,
      .frame_id = texture.texture.next_frame_id,
      .texture = backend_.texture(),
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

auto fill_opengl_frame(
  mln_render_session* texture, mln_opengl_owned_texture_frame* out_frame
) -> mln_status {
  return texture->texture.backend->acquire_opengl_owned_frame(
    *texture, *out_frame
  );
}

}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_OPENGL;
}

auto metal_owned_texture_descriptor_default() noexcept
  -> mln_metal_owned_texture_descriptor {
  return mln_metal_owned_texture_descriptor{
    .size = sizeof(mln_metal_owned_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_metal_context_descriptor{
      .size = sizeof(mln_metal_context_descriptor),
      .device = nullptr,
    },
  };
}

auto metal_borrowed_texture_descriptor_default() noexcept
  -> mln_metal_borrowed_texture_descriptor {
  return mln_metal_borrowed_texture_descriptor{
    .size = sizeof(mln_metal_borrowed_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .texture = nullptr,
  };
}

auto vulkan_owned_texture_descriptor_default() noexcept
  -> mln_vulkan_owned_texture_descriptor {
  return mln_vulkan_owned_texture_descriptor{
    .size = sizeof(mln_vulkan_owned_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_vulkan_context_descriptor{
      .size = sizeof(mln_vulkan_context_descriptor),
      .instance = nullptr,
      .physical_device = nullptr,
      .device = nullptr,
      .graphics_queue = nullptr,
      .graphics_queue_family_index = 0,
      .get_instance_proc_addr = nullptr,
      .get_device_proc_addr = nullptr,
    },
  };
}

auto vulkan_borrowed_texture_descriptor_default() noexcept
  -> mln_vulkan_borrowed_texture_descriptor {
  return mln_vulkan_borrowed_texture_descriptor{
    .size = sizeof(mln_vulkan_borrowed_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context =
      mln_vulkan_context_descriptor{
        .size = sizeof(mln_vulkan_context_descriptor),
        .instance = nullptr,
        .physical_device = nullptr,
        .device = nullptr,
        .graphics_queue = nullptr,
        .graphics_queue_family_index = 0,
        .get_instance_proc_addr = nullptr,
        .get_device_proc_addr = nullptr,
      },
    .image = nullptr,
    .image_view = nullptr,
    .format = 0,
    .initial_layout = 0,
    .final_layout = 5,
  };
}

auto metal_owned_texture_attach(
  mln_map* map, const mln_metal_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_metal_owned_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_borrowed_texture_attach(
  mln_map* map, const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_metal_borrowed_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_owned_texture_attach(
  mln_map* map, const mln_vulkan_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_vulkan_owned_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_borrowed_texture_attach(
  mln_map* map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_vulkan_borrowed_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto opengl_owned_texture_attach(
  mln_map* map, const mln_opengl_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_opengl_owned_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_unique<mln_render_session>();
  session->map = map;
  session->owner_thread = map_owner_thread(map);
  set_session_extent(*session, descriptor->extent);
  session->texture.api_kind = TextureSessionApi::OpenGL;
  session->texture.mode = TextureSessionMode::Owned;
  session->texture.backend = std::make_unique<OpenGLTextureSessionBackend>(
    *descriptor, mbgl::Size{session->physical_width, session->physical_height}
  );
  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Texture,
    RenderSessionAttachMessages{
      .null_session = "texture session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

auto opengl_borrowed_texture_attach(
  mln_map* map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_opengl_borrowed_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_unique<mln_render_session>();
  session->map = map;
  session->owner_thread = map_owner_thread(map);
  set_session_extent(*session, descriptor->extent);
  session->texture.api_kind = TextureSessionApi::OpenGL;
  session->texture.mode = TextureSessionMode::Borrowed;
  session->texture.backend = std::make_unique<OpenGLTextureSessionBackend>(
    *descriptor, mbgl::Size{session->physical_width, session->physical_height}
  );
  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Texture,
    RenderSessionAttachMessages{
      .null_session = "texture session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

auto metal_owned_texture_acquire_frame(
  mln_render_session* texture, mln_metal_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_frame(
    texture, out_frame, out_frame == nullptr ? 0 : out_frame->size,
    "out_frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_frame->size < sizeof(mln_metal_owned_texture_frame)) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_owned_texture_release_frame(
  mln_render_session* texture, const mln_metal_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_frame(
    texture, frame, frame == nullptr ? 0 : frame->size,
    "frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (frame->size < sizeof(mln_metal_owned_texture_frame)) {
    set_thread_error("frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_owned_texture_acquire_frame(
  mln_render_session* texture, mln_vulkan_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_frame(
    texture, out_frame, out_frame == nullptr ? 0 : out_frame->size,
    "out_frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_frame->size < sizeof(mln_vulkan_owned_texture_frame)) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_owned_texture_release_frame(
  mln_render_session* texture, const mln_vulkan_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_frame(
    texture, frame, frame == nullptr ? 0 : frame->size,
    "frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (frame->size < sizeof(mln_vulkan_owned_texture_frame)) {
    set_thread_error("frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto opengl_owned_texture_acquire_frame(
  mln_render_session* texture, mln_opengl_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_live_attached_texture(texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_frame == nullptr ||
    out_frame->size < sizeof(mln_opengl_owned_texture_frame)
  ) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (texture->texture.acquired) {
    set_thread_error("a texture frame is already acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (texture->rendered_generation != texture->generation) {
    set_thread_error("no rendered frame is available for this generation");
    return MLN_STATUS_INVALID_STATE;
  }
  if (
    texture->texture.mode != TextureSessionMode::Owned ||
    texture->texture.api_kind != TextureSessionApi::OpenGL
  ) {
    set_thread_error("texture session cannot expose an OpenGL texture frame");
    return MLN_STATUS_UNSUPPORTED;
  }

  const auto frame_status = fill_opengl_frame(texture, out_frame);
  if (frame_status != MLN_STATUS_OK) {
    return frame_status;
  }
  texture->texture.acquired_native_texture =
    reinterpret_cast<void*>(static_cast<uintptr_t>(out_frame->texture));
  texture->texture.acquired = true;
  texture->texture.acquired_frame_id = out_frame->frame_id;
  texture->texture.acquired_frame_kind = TextureSessionFrameKind::OpenGLOwned;
  ++texture->texture.next_frame_id;
  return MLN_STATUS_OK;
}

auto opengl_owned_texture_release_frame(
  mln_render_session* texture, const mln_opengl_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_texture(texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    frame == nullptr || frame->size < sizeof(mln_opengl_owned_texture_frame)
  ) {
    set_thread_error("frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    !texture->texture.acquired ||
    texture->texture.acquired_frame_kind != TextureSessionFrameKind::OpenGLOwned
  ) {
    set_thread_error("no texture frame is currently acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (frame->generation != texture->generation) {
    set_thread_error("frame generation does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (frame->frame_id != texture->texture.acquired_frame_id) {
    set_thread_error("frame identity does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  texture->texture.acquired = false;
  texture->texture.acquired_frame_id = 0;
  texture->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  texture->texture.acquired_native_texture = nullptr;
  return MLN_STATUS_OK;
}

}  // namespace mln::core
