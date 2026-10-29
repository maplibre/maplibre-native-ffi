"""Consume the release's complete static C archive through CMakeDeps.

`backend=default` selects the platform's default renderer. Linux defaults to
`libc=gnu`; musl consumers must select `libc=musl`. Only OS/architecture and
these options select the prebuilt binary; the consumer's compiler and build
mode do not rebuild it.
"""

import json
import os
import shlex
from typing import ClassVar

from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.files import copy, get, load


class MaplibreNativeC(ConanFile):
    name = "maplibre-native-c"
    package_type = "static-library"
    license = "BSD-2-Clause"
    homepage = "https://github.com/maplibre/maplibre-native-ffi"
    description = "The C API for MapLibre Native, as a prebuilt static library"
    settings = "os", "arch", "compiler"
    options: ClassVar[dict[str, list[str]]] = {
        "backend": ["default", "metal", "opengl", "vulkan", "webgpu"],
        "libc": ["gnu", "musl"],
    }
    default_options: ClassVar[dict[str, str]] = {"backend": "default", "libc": "gnu"}

    def config_options(self):
        if self.settings.os != "Linux":
            self.options.rm_safe("libc")

    @property
    def _platform(self):
        platforms = {
            "Macos": "macos",
            "Linux": f"linux-{self.options.get_safe('libc', 'gnu')}",
            "Windows": "windows",
            "Android": "android",
            "FreeBSD": "freebsd",
            "iOS": "ios",
            "tvOS": "tvos",
            "visionOS": "visionos",
            "Emscripten": "emscripten",
        }
        architectures = {
            "x86": "x86",
            "x86_64": "x64",
            "armv8": "arm64",
            "armv7": "arm",
            "wasm": "wasm32",
        }
        platform = platforms.get(str(self.settings.os))
        arch = architectures.get(str(self.settings.arch))
        if platform is None or arch is None:
            raise ConanInvalidConfiguration(
                "No native archive for this OS/architecture"
            )
        if self.settings.get_safe("os.subsystem") == "catalyst":
            platform = "ios-maccatalyst"
        elif "simulator" in str(self.settings.get_safe("os.sdk", "")):
            platform += "-simulator"
        return f"{platform}-{arch}"

    def _source(self):
        backend = str(self.options.backend)
        if backend == "default":
            if self.settings.os in ("Macos", "iOS", "tvOS", "visionOS"):
                backend = "metal"
            elif self.settings.os in ("Linux", "Windows", "FreeBSD"):
                backend = "vulkan"
            elif self.settings.os == "Emscripten":
                backend = "webgpu"
            else:
                backend = "opengl"
        suffix = {"Windows": "wgl", "Emscripten": "webgl"}.get(
            str(self.settings.os), "egl"
        )
        key = f"{self._platform}-{suffix if backend == 'opengl' else backend}"
        try:
            return self.conan_data["sources"][str(self.version)][key]
        except KeyError:
            raise ConanInvalidConfiguration(f"No native archive for {key}") from None

    def validate(self):
        if (
            self.settings.os == "Windows"
            and self.settings.get_safe("compiler.runtime") != "static"
        ):
            raise ConanInvalidConfiguration(
                "Windows archives require -s compiler.runtime=static"
            )
        self._source()

    def _libraries(self):
        main = (
            "maplibre-native-c-static"
            if self.settings.os == "Windows"
            else "maplibre-native-c"
        )
        return [main, "mln_ffi_platform"]

    def package_id(self):
        self.info.settings.rm_safe("compiler")

    def build(self):
        get(self, **self._source(), strip_root=True)

    def package(self):
        for directory in ("include", "share"):
            copy(
                self,
                "*",
                src=os.path.join(self.build_folder, directory),
                dst=os.path.join(self.package_folder, directory),
            )
        # Where the Rust platform support is a separate archive, the static
        # library needs it too.
        for library in self._libraries():
            copy(
                self,
                f"{library}.lib"
                if self.settings.os == "Windows"
                else f"lib{library}.a",
                src=os.path.join(self.build_folder, "lib"),
                dst=os.path.join(self.package_folder, "lib"),
            )

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "maplibre-native-c")
        self.cpp_info.set_property(
            "cmake_target_name", "maplibre-native-c::maplibre-native-c"
        )
        self.cpp_info.libs = [
            library
            for library in self._libraries()
            if os.path.exists(
                os.path.join(
                    self.package_folder,
                    "lib",
                    f"{library}.lib"
                    if self.settings.os == "Windows"
                    else f"lib{library}.a",
                )
            )
        ]
        self.cpp_info.defines = ["MLN_STATIC"]
        descriptor = json.loads(
            load(
                self,
                os.path.join(
                    self.package_folder, "share/maplibre-native-c/artifact.json"
                ),
            )
        )
        self.cpp_info.system_libs = descriptor["link"]["libraries"]
        # The Rust platform archive unwinds through libgcc_s on glibc, which a
        # C consumer's compiler driver need not link.
        if self.settings.os == "Linux" and self.options.libc == "gnu":
            self.cpp_info.system_libs.append("gcc_s")
        self.cpp_info.frameworks = descriptor["link"]["frameworks"]
        # The archive's pkg-config file carries Emscripten's compile flags;
        # its installed header directory is supplied by Conan instead.
        if self.settings.os == "Emscripten":
            pc = load(
                self,
                os.path.join(
                    self.package_folder, "share/pkgconfig/maplibre-native-c.pc"
                ),
            )
            cflags = next(
                line.removeprefix("Cflags:").strip()
                for line in pc.splitlines()
                if line.startswith("Cflags:")
            )
            flags = [flag for flag in shlex.split(cflags) if not flag.startswith("-I")]
            self.cpp_info.cflags = flags
            self.cpp_info.cxxflags = flags
            link_flags = load(
                self,
                os.path.join(
                    self.package_folder,
                    "share/maplibre-native-c/emscripten-link-flags.txt",
                ),
            ).splitlines()
            self.cpp_info.exelinkflags = link_flags
            self.cpp_info.sharedlinkflags = link_flags
