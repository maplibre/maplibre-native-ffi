from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain
from conan.tools.gnu import PkgConfigDeps
from conan.tools.env import VirtualRunEnv


class MapLibreNativeCDependencies(ConanFile):
    name = "maplibre-native-c-dependencies"
    version = "0.1.0"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "render_backend": ["metal", "opengl", "vulkan"],
        "opengl_context_provider": ["none", "egl", "wgl"],
    }
    default_options = {
        "render_backend": "vulkan",
        "opengl_context_provider": "none",
        "*:shared": True,
        "spirv-cross/*:shared": False,
        "spirv-tools/*:shared": False,
        "libcurl/*:with_nghttp2": False,
        "libcurl/*:with_libidn": False,
        "libcurl/*:with_libpsl": False,
        "libcurl/*:with_libssh2": False,
        "libcurl/*:with_zstd": False,
        "libcurl/*:with_brotli": False,
        "libcurl/*:with_c_ares": False,
        "libjpeg-turbo/*:java": False,
        "libglvnd/*:glx": False,
        "libglvnd/*:x11": False,
        "vulkan-loader/*:with_wsi_xcb": False,
        "vulkan-loader/*:with_wsi_xlib": False,
        "xkbcommon/*:with_x11": False,
    }

    def layout(self):
        self.folders.generators = "generators"

    def configure(self):
        if self.settings.os == "Linux":
            self.options["sdl"].audio = False
            self.options["sdl"].camera = False
            self.options["sdl"].dialog = False
            self.options["sdl"].gpu = False
            self.options["sdl"].haptic = False
            self.options["sdl"].hidapi = False
            self.options["sdl"].joystick = False
            self.options["sdl"].power = False
            self.options["sdl"].render = False
            self.options["sdl"].sensor = False
            self.options["sdl"].tray = False
            self.options["sdl"].dbus = False
            self.options["sdl"].libudev = False
            self.options["sdl"].x11 = False
            self.options["sdl"].xcursor = False
            self.options["sdl"].xdbe = False
            self.options["sdl"].xfixes = False
            self.options["sdl"].xinput = False
            self.options["sdl"].xrandr = False
            self.options["sdl"].xscrnsaver = False
            self.options["sdl"].xshape = False
            self.options["sdl"].xsync = False

    def requirements(self):
        self.requires("zlib/1.3.2")
        self.requires("sdl/3.4.8")

        if self.settings.os in ["Linux", "Windows"]:
            self.requires("libcurl/8.20.0")
            self.requires("libjpeg-turbo/3.1.4.1")
            self.requires("libpng/1.6.58")
            self.requires("libuv/1.51.0")
            self.requires("libwebp/1.6.0")

        if self.settings.os == "Linux":
            self.requires("libxcrypt/4.4.36")
            if (
                self.options.render_backend == "opengl"
                and self.options.opengl_context_provider == "egl"
            ):
                self.requires("libglvnd/1.7.0")

        if self.options.render_backend == "vulkan":
            self.requires("vulkan-loader/1.4.313.0")
            if self.settings.os == "Macos":
                self.requires("moltenvk/1.3.0")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        pkg_config = PkgConfigDeps(self)
        pkg_config.generate()

        run_env = VirtualRunEnv(self)
        run_env.generate()

        toolchain = CMakeToolchain(self)
        toolchain.cache_variables["CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO"] = "Release"
        toolchain.generate()
