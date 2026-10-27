from __future__ import annotations

import json
import pathlib
import tomllib

# Targets whose suite runs on an emulator instead of through ctest, so CMake
# registers no test preset for them.
EMULATOR_TESTED = {
    "android-x64-egl",
    "android-x64-vulkan",
    "ohos-x64-egl",
}


def runtime_tested(preset: str, tested: set[str]) -> bool:
    """Whether CI executes this target's native suite rather than only building it."""
    return preset in tested or preset in EMULATOR_TESTED


def load_configuration(
    root: pathlib.Path,
) -> tuple[dict[str, object], dict[str, object]]:
    with (root / "ci" / "workflow.toml").open("rb") as file:
        source = tomllib.load(file)
    presets = json.loads((root / "CMakePresets.json").read_text())
    return source, presets


def platform(preset: str) -> str:
    if preset.startswith("linux-gnu-"):
        return "linux-gnu"
    if preset.startswith("linux-musl-"):
        return "linux-musl"
    if preset.startswith("ios-simulator-"):
        return "ios-simulator"
    if preset.startswith("ios-maccatalyst-"):
        return "ios-maccatalyst"
    if preset.startswith("visionos-simulator-"):
        return "visionos-simulator"
    if preset.startswith("tvos-simulator-"):
        return "tvos-simulator"
    return preset.split("-", 1)[0]


def architecture(preset: str) -> str:
    parts = preset.split("-")
    for candidate in ("x64", "arm64", "arm", "wasm32"):
        if candidate in parts:
            return candidate
    raise SystemExit(f"error: cannot determine architecture from preset {preset!r}")


def backend(preset: str) -> str:
    value = preset.rsplit("-", 1)[-1]
    if value not in {"egl", "metal", "vulkan", "webgl", "webgpu", "wgl"}:
        raise SystemExit(f"error: cannot determine backend from preset {preset!r}")
    return value


# Blacksmith runs CI through its open source program. Windows ARM64 uses
# GitHub-hosted runners. Windows x64 uses Mesa software drivers for the native
# suites and the headless smoke app.
LINUX_RUNNER = "blacksmith-4vcpu-ubuntu-2404"
LINUX_ARM_RUNNER = "blacksmith-4vcpu-ubuntu-2404-arm"
MACOS_RUNNER = "blacksmith-6vcpu-macos-26"
WINDOWS_RUNNER = "blacksmith-4vcpu-windows-2025"
WINDOWS_ARM_RUNNER = "windows-11-arm"
SMALL_RUNNER = "blacksmith-2vcpu-ubuntu-2404"


def runner(preset: str) -> str:
    target_platform = platform(preset)
    target_architecture = architecture(preset)
    if target_platform in {"linux-gnu", "linux-musl"}:
        # The Zig toolchain selects the libc. These images stay pinned so the
        # graphics headers and loaders used at build time stay reproducible.
        return LINUX_ARM_RUNNER if target_architecture == "arm64" else LINUX_RUNNER
    if target_platform in {
        "macos",
        "ios",
        "ios-simulator",
        "ios-maccatalyst",
        "tvos",
        "tvos-simulator",
        "visionos",
        "visionos-simulator",
    }:
        return MACOS_RUNNER
    if target_platform == "windows":
        return WINDOWS_ARM_RUNNER if target_architecture == "arm64" else WINDOWS_RUNNER
    # FreeBSD cross-compiles on Linux, and its suites run in a FreeBSD virtual
    # machine there.
    if target_platform in {"android", "emscripten", "freebsd", "ohos"}:
        return LINUX_RUNNER
    raise SystemExit(f"error: cannot determine runner from preset {preset!r}")


def device_boot(preset: str) -> tuple[str, str] | None:
    """The named step that boots the device a target's suites run on, if any.

    The suites boot the device themselves when it is not ready, and every boot
    command reuses a ready device. Booting in a step of its own lets CI tell a
    device that never came up from a suite that failed on it.
    """
    target_platform = platform(preset)
    if target_platform == "android" and preset in EMULATOR_TESTED:
        # The EGL target's suites run on API 26, as the root `test` task and
        # the device test scripts ask for it.
        api = " --api 26" if backend(preset) == "egl" else ""
        return (
            "Boot Android emulator",
            f"mise exec -- tools/devices/android-serial x86_64{api}",
        )
    if target_platform == "ohos" and preset in EMULATOR_TESTED:
        return "Boot OpenHarmony emulator", "mise exec -- tools/devices/ohos-key"
    if target_platform == "freebsd":
        return (
            "Boot FreeBSD virtual machine",
            "mise exec -- tools/devices/freebsd-host",
        )
    if target_platform == "ios-simulator":
        return "Boot iOS simulator", "mise exec -- devices-cli get ios"
    if target_platform == "visionos-simulator":
        return "Boot visionOS simulator", "mise exec -- devices-cli get visionos"
    if target_platform == "tvos-simulator":
        return "Boot tvOS simulator", "mise exec -- devices-cli get tvos"
    return None


def ohos_commands(preset: str) -> list[str]:
    # The emulator executes x64 EGL. Other OpenHarmony targets still prove that
    # each binding links against its backend-specific native artifact.
    action = "test" if preset in EMULATOR_TESTED else "build"
    return [
        f"mise run //bindings/rust:{action} {preset}",
        f"mise run //bindings/go:{action} {preset}",
        f"mise run //bindings/typescript:{action} {preset}",
    ]


def preset_sets(
    presets: dict[str, object],
) -> tuple[list[str], set[str], set[str], set[str]]:
    def names(kind: str) -> list[str]:
        # Hidden presets carry settings for others to inherit and name no
        # target, so they take part in no preset pairing.
        return [
            preset["name"]
            for preset in presets.get(kind, [])
            if not preset.get("hidden", False)
        ]

    configured = names("configurePresets")
    built = set(names("buildPresets"))
    tested = set(names("testPresets"))
    packaged = set(names("packagePresets"))
    if set(configured) != built:
        raise SystemExit(
            "error: configure and build presets differ: "
            f"missing={sorted(set(configured) - built)}, extra={sorted(built - set(configured))}"
        )
    if not tested <= set(configured) or not packaged <= set(configured):
        raise SystemExit(
            "error: test and package presets must reference configure presets"
        )
    return configured, built, tested, packaged
