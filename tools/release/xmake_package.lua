package("maplibre-native-c")
    set_homepage("https://maplibre.org/maplibre-native-ffi/")
    set_description("A C API for MapLibre Native")
    set_license("BSD-2-Clause")
    set_kind("library")

    add_configs("backend", {
        description = "Render backend (defaults to the platform's backend)",
        default = "default", type = "string",
        values = {"default", "egl", "vulkan", "wgl", "metal", "webgl", "webgpu"}
    })
    add_configs("libc", {
        description = "Linux C library", default = "gnu", type = "string",
        values = {"gnu", "musl"}
    })
    add_configs("shared", {
        description = "Use the release's shared library",
        default = not is_plat("wasm"), type = "boolean", readonly = true
    })

    -- RELEASE_ARCHIVES

    on_load(function (package)
        import("core.project.config")
        local platforms = {
            macosx = "macos", windows = "windows", android = "android",
            harmony = "ohos", bsd = "freebsd",
            iphoneos = "ios", appletvos = "tvos", applexros = "visionos"
        }
        local platform = platforms[package:plat()]
        if package:is_plat("linux") then
            platform = "linux-" .. package:config("libc")
        end
        assert(platform, "unsupported platform: " .. package:plat())
        local appledev = config.get("appledev")
        if appledev == "simulator" then
            platform = platform .. "-simulator"
        elseif appledev == "catalyst" then
            platform = "ios-maccatalyst"
        end
        local architectures = {
            x86 = "x86", x86_64 = "x64", x64 = "x64", arm64 = "arm64",
            ["arm64-v8a"] = "arm64", armv7 = "arm", ["armeabi-v7a"] = "arm",
            wasm32 = "wasm32"
        }
        local arch = architectures[package:arch()]
        assert(arch, "unsupported architecture: " .. package:arch())
        local backend = package:config("backend")
        if backend == "default" then
            if package:is_plat("macosx", "iphoneos", "appletvos", "applexros") then
                backend = "metal"
            elseif package:is_plat("windows") then
                backend = "wgl"
            elseif package:is_plat("wasm") then
                backend = "webgl"
            else
                backend = "egl"
            end
        end
        local preset = platform .. "-" .. arch .. "-" .. backend
        local selected
        for _, url in ipairs(package:urls()) do
            if package:url_alias(url) == preset then
                selected = url
                break
            end
        end
        assert(selected, "no release archive for " .. preset)
        -- xmake treats URLs as mirrors; only this target's archive may be tried.
        package:set("urls", selected, {alias = preset})
        package:add("links", "maplibre-native-c")
        package:add("rpathdirs", package:installdir("lib"))
    end)

    on_install(function (package)
        for _, directory in ipairs({"include", "lib", "bin", "share", "Frameworks"}) do
            if os.isdir(directory) then
                os.cp(directory, package:installdir())
            end
        end
    end)
