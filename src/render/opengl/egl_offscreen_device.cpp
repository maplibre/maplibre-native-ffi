#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <mln/util/logging.hpp>

#include <EGL/egl.h>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/offscreen_device.hpp"
#include "render/opengl/egl_context.hpp"
#include "render/opengl/opengl_texture_session.hpp"
#include "render/render_session_common.hpp"

// Android and OpenHarmony define __linux__ too, but their EGL serves their
// own window systems through the default display and lists no devices.
#if (defined(__linux__) || defined(__FreeBSD__)) && !defined(__ANDROID__) && \
  !defined(__OHOS__)
#define MLN_FFI_EGL_DEVICE_PLATFORMS 1
#endif

namespace mln::core {
namespace {

// Extension values, spelled out because the EGL headers that a platform ships
// differ in which extensions they declare.
#if defined(MLN_FFI_EGL_DEVICE_PLATFORMS)
constexpr auto egl_platform_device = EGLenum{0x313F};
constexpr auto egl_platform_surfaceless_mesa = EGLenum{0x31DD};
#elif defined(__APPLE__)
constexpr auto egl_platform_angle = EGLenum{0x3202};
constexpr auto egl_platform_angle_type = EGLint{0x3203};
constexpr auto egl_platform_angle_type_metal = EGLint{0x3489};
constexpr auto egl_platform_angle_device_type = EGLint{0x3209};
constexpr auto egl_platform_angle_device_type_hardware = EGLint{0x320A};
#endif

using GetPlatformDisplay = EGLDisplay (*)(
  EGLenum platform, void* native_display, const EGLint* attributes
);
#if defined(MLN_FFI_EGL_DEVICE_PLATFORMS)
using QueryDevices =
  EGLBoolean (*)(EGLint max_devices, void** devices, EGLint* device_count);
#endif

template <typename Function>
auto egl_extension(const char* name) -> Function {
  // EGL returns extension functions as untyped procedure pointers.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
  return reinterpret_cast<Function>(eglGetProcAddress(name));
}

auto api_name(EGLenum api) -> const char* {
  return api == EGL_OPENGL_API ? "desktop OpenGL" : "OpenGL ES 3";
}

// A display that initializes and has a config with pbuffers that renders
// `api`, which a dedicated session needs to make its context current.
auto usable_config(EGLDisplay display, EGLenum api) -> EGLConfig {
  if (display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr)) {
    return nullptr;
  }
  const auto attributes = std::array<EGLint, 17>{
    EGL_SURFACE_TYPE,
    EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE,
    api == EGL_OPENGL_API ? EGL_OPENGL_BIT : EGL_OPENGL_ES3_BIT,
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
  };
  auto config = EGLConfig{};
  auto count = EGLint{};
  if (
    !eglChooseConfig(display, attributes.data(), &config, 1, &count) ||
    count == 0
  ) {
    return nullptr;
  }
  return config;
}

struct DisplayChoice {
  EGLDisplay display = EGL_NO_DISPLAY;
  EGLConfig config = nullptr;
  const char* platform = "";
};

auto choose(EGLDisplay display, const char* platform, EGLenum api)
  -> DisplayChoice {
  auto* const config = usable_config(display, api);
  if (config == nullptr) {
    return {};
  }
  return DisplayChoice{
    .display = display, .config = config, .platform = platform
  };
}

#if defined(MLN_FFI_EGL_DEVICE_PLATFORMS)
auto device_display(uint32_t index, EGLenum api, DisplayChoice& out_choice)
  -> mln_status {
  const auto get_platform_display =
    egl_extension<GetPlatformDisplay>("eglGetPlatformDisplayEXT");
  const auto query_devices = egl_extension<QueryDevices>("eglQueryDevicesEXT");
  auto count = EGLint{};
  if (
    get_platform_display == nullptr || query_devices == nullptr ||
    !query_devices(0, nullptr, &count)
  ) {
    set_thread_error("EGL cannot list its devices");
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (index >= static_cast<uint32_t>(count)) {
    const auto message = "adapter_index " + std::to_string(index) +
                         " is past the " + std::to_string(count) +
                         " EGL devices";
    set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto devices = std::vector<void*>(static_cast<std::size_t>(count));
  if (
    !query_devices(count, devices.data(), &count) ||
    index >= static_cast<uint32_t>(count)
  ) {
    set_thread_error("EGL cannot list its devices");
    return MLN_STATUS_NATIVE_ERROR;
  }
  out_choice = choose(
    get_platform_display(egl_platform_device, devices[index], nullptr),
    "device", api
  );
  if (out_choice.display == EGL_NO_DISPLAY) {
    const auto message =
      std::string{"the EGL device at adapter_index has no "} + api_name(api) +
      " pbuffer config";
    set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  return MLN_STATUS_OK;
}
#endif

// Linux and FreeBSD try the surfaceless platform first: the default display
// resolves to whatever libEGL was built for, commonly X11, which fails without
// a display server. A driver without the surfaceless platform, such as
// NVIDIA's, takes its first device instead. Apple uses ANGLE on Metal, and
// Android and OpenHarmony keep the default display, which serves their own
// window systems.
auto default_display(EGLenum api) -> DisplayChoice {
#if defined(MLN_FFI_EGL_DEVICE_PLATFORMS)
  if (
    const auto get_platform_display =
      egl_extension<GetPlatformDisplay>("eglGetPlatformDisplayEXT");
    get_platform_display != nullptr
  ) {
    if (
      auto choice = choose(
        get_platform_display(egl_platform_surfaceless_mesa, nullptr, nullptr),
        "surfaceless", api
      );
      choice.display != EGL_NO_DISPLAY
    ) {
      return choice;
    }
    if (
      auto choice = DisplayChoice{};
      device_display(0, api, choice) == MLN_STATUS_OK
    ) {
      return choice;
    }
  }
#elif defined(__APPLE__)
  const auto get_platform_display =
    egl_extension<GetPlatformDisplay>("eglGetPlatformDisplayEXT");
  if (get_platform_display != nullptr) {
    const auto attributes = std::array<EGLint, 5>{
      egl_platform_angle_type,
      egl_platform_angle_type_metal,
      egl_platform_angle_device_type,
      egl_platform_angle_device_type_hardware,
      EGL_NONE,
    };
    // ANGLE's headers define EGL_DEFAULT_DISPLAY as an integer, so the null
    // native display is spelled out.
    return choose(
      get_platform_display(egl_platform_angle, nullptr, attributes.data()),
      "ANGLE Metal", api
    );
  }
#endif
  return choose(eglGetDisplay(EGL_DEFAULT_DISPLAY), "default", api);
}

// Binding an API is per thread, so the calling thread keeps the one it had.
auto create_single_context(
  const DisplayChoice& choice, EGLenum api,
  std::shared_ptr<opengl::EglSingleContext>& out_single
) -> mln_status {
  const auto previous_api = eglQueryAPI();
  if (eglBindAPI(api) == EGL_FALSE) {
    const auto message =
      std::string{"binding the EGL "} + api_name(api) + " API failed";
    set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto* const context = opengl::create_egl_context(
    choice.display, choice.config, EGL_NO_CONTEXT, api
  );
  const auto error = eglGetError();
  eglBindAPI(previous_api);
  if (context == EGL_NO_CONTEXT) {
    const auto message = "creating the single EGL context failed (EGL error " +
                         std::to_string(error) + ")";
    set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  out_single =
    std::make_shared<opengl::EglSingleContext>(choice.display, context);
  return MLN_STATUS_OK;
}

// A display stays initialized for the rest of the process. EGL returns one
// display for every request with the same platform and device, so the host's
// own code, or a later device, may be using it: terminating it would end their
// contexts too.
class EglOffscreenDevice final : public OffscreenDevice {
 public:
  EglOffscreenDevice(
    const DisplayChoice& choice, mln_open_gl_client_api client_api,
    std::shared_ptr<opengl::EglSingleContext> single
  )
      : single_(std::move(single)),
        context_{
          .platform = MLN_OPEN_GL_CONTEXT_PLATFORM_EGL,
          .ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED,
          .data = {
            .egl = {
              .display = choice.display,
              .config = choice.config,
              .share_context = nullptr,
              .client_api = client_api,
              .get_proc_address = nullptr,
            },
          },
        } {}

  auto create_texture_backend(
    mln::Size size, std::unique_ptr<TextureSessionBackend>& out_backend
  ) -> mln_status override {
    return create_opengl_offscreen_texture_backend(
      context_, size, out_backend, single_
    );
  }

 private:
  std::shared_ptr<opengl::EglSingleContext> single_;
  mln_open_gl_context_descriptor context_;
};

}  // namespace

#if defined(MLN_FFI_EGL_DEVICE_PLATFORMS)
namespace opengl {

auto default_egl_display(
  mln_open_gl_client_api client_api, DefaultEglDisplay& out_display
) -> mln_status {
  const auto api = egl_api(client_api);
  const auto choice = default_display(api);
  if (choice.display == EGL_NO_DISPLAY) {
    const auto message = std::string{"no EGL display has a "} + api_name(api) +
                         " pbuffer config (EGL error " +
                         std::to_string(eglGetError()) + ")";
    set_thread_error(message.c_str());
    return MLN_STATUS_NATIVE_ERROR;
  }
  out_display =
    DefaultEglDisplay{.display = choice.display, .config = choice.config};
  return MLN_STATUS_OK;
}

}  // namespace opengl
#endif

auto create_backend_offscreen_device(
  const mln_offscreen_device_options& options,
  std::shared_ptr<OffscreenDevice>& out_device
) -> mln_status {
  const auto client_api = options.client_api == MLN_OPEN_GL_CLIENT_API_GL
                            ? MLN_OPEN_GL_CLIENT_API_GL
                            : MLN_OPEN_GL_CLIENT_API_GLES;
#if !defined(MLN_FFI_EGL_DEVICE_PLATFORMS)
  if (client_api == MLN_OPEN_GL_CLIENT_API_GL) {
    set_thread_error(
      "this platform's EGL implements OpenGL ES only, not desktop OpenGL"
    );
    return MLN_STATUS_UNSUPPORTED;
  }
#endif
  const auto api = opengl::egl_api(client_api);
  auto choice = DisplayChoice{};
  if ((options.fields & MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX) == 0U) {
    choice = default_display(api);
    if (choice.display == EGL_NO_DISPLAY) {
      const auto message = std::string{"no EGL display has a "} +
                           api_name(api) + " pbuffer config (EGL error " +
                           std::to_string(eglGetError()) + ")";
      set_thread_error(message.c_str());
      return MLN_STATUS_NATIVE_ERROR;
    }
  } else {
#if defined(MLN_FFI_EGL_DEVICE_PLATFORMS)
    const auto status = device_display(options.adapter_index, api, choice);
    if (status != MLN_STATUS_OK) {
      return status;
    }
#else
    // These platforms list no EGL devices, so their one adapter is the
    // default display.
    if (options.adapter_index != 0) {
      const auto message = "adapter_index " +
                           std::to_string(options.adapter_index) +
                           " is past the one EGL display on this platform";
      set_thread_error(message.c_str());
      return MLN_STATUS_NATIVE_ERROR;
    }
    choice = default_display(api);
    if (choice.display == EGL_NO_DISPLAY) {
      const auto message = std::string{"the EGL display has no "} +
                           api_name(api) + " pbuffer config";
      set_thread_error(message.c_str());
      return MLN_STATUS_NATIVE_ERROR;
    }
#endif
  }
  auto single = std::shared_ptr<opengl::EglSingleContext>{};
  if (options.single_context) {
    const auto status = create_single_context(choice, api, single);
    if (status != MLN_STATUS_OK) {
      return status;
    }
  }
  mln::Log::Info(
    mln::Event::Render, std::string{"offscreen device uses the EGL "} +
                          choice.platform + " platform with " + api_name(api)
  );
  out_device =
    std::make_shared<EglOffscreenDevice>(choice, client_api, std::move(single));
  return MLN_STATUS_OK;
}

}  // namespace mln::core
