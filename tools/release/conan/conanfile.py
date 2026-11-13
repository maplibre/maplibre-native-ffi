"""Consume the release's complete static C archive through CMakeDeps.

`backend=default` selects the platform's default renderer. Linux defaults to
`libc=gnu`; musl consumers must select `libc=musl`. Only OS/architecture and
these options select the prebuilt binary; the consumer's compiler and build
mode do not rebuild it.
"""

import json
import os
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
        }
        architectures = {
            "x86": "x86",
            "x86_64": "x64",
            "armv8": "arm64",
            "armv7": "arm",
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
            else:
                backend = "opengl"
        suffix = {"Windows": "wgl"}.get(str(self.settings.os), "egl")
        key = f"{self._platform}-{suffix if backend == 'opengl' else backend}"
        try:
            return self.conan_data["sources"][str(self.version)][key]
        except KeyError:
            raise ConanInvalidConfiguration(f"No native archive for {key}") from None

    def validate(self):
        self._source()

    def _libraries(self):
        main = (
            "maplibre-native-c-static"
            if self.settings.os == "Windows"
            else "maplibre-native-c"
        )
        return [main]

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
        # An executable that links the archive imports the Visual C++ runtime,
        # which a consumer deploys from here.
        if self.settings.os == "Windows":
            for pattern in ("vcruntime140*.dll", "msvcp140*.dll"):
                copy(
                    self,
                    pattern,
                    src=os.path.join(self.build_folder, "bin"),
                    dst=os.path.join(self.package_folder, "bin"),
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
