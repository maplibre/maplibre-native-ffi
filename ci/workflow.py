from __future__ import annotations

import json
import pathlib
import tomllib

DESKTOP = {"linux", "macos", "windows"}

# Targets whose suite runs on an emulator instead of through ctest, so CMake
# registers no test preset for them.
EMULATOR_TESTED = {
    "android-x64-egl",
    "android-x64-vulkan",
    "ohos-x64-egl",
}


def runtime_tested(preset: str, tested: set[str]) -> bool:
    """Whether CI executes this target's C API suite rather than only building it."""
    return preset in tested or preset in EMULATOR_TESTED


def load_configuration(
    root: pathlib.Path,
) -> tuple[dict[str, object], dict[str, object]]:
    with (root / "ci" / "workflow.toml").open("rb") as file:
        source = tomllib.load(file)
    presets = json.loads((root / "CMakePresets.json").read_text())
    return source, presets


def platform(preset: str) -> str:
    if preset.startswith("ios-simulator-"):
        return "ios-simulator"
    if preset.startswith("tvos-simulator-"):
        return "tvos-simulator"
    return preset.split("-", 1)[0]


def architecture(preset: str) -> str:
    parts = preset.split("-")
    for candidate in ("x64", "arm64", "wasm32"):
        if candidate in parts:
            return candidate
    raise SystemExit(f"error: cannot determine architecture from preset {preset!r}")


def backend(preset: str) -> str:
    value = preset.rsplit("-", 1)[-1]
    if value not in {"egl", "metal", "vulkan", "webgl", "webgpu", "wgl"}:
        raise SystemExit(f"error: cannot determine backend from preset {preset!r}")
    return value


def runner(preset: str) -> str:
    target_platform = platform(preset)
    target_architecture = architecture(preset)
    if target_platform == "linux":
        # The zig toolchain sets the glibc floor. These images stay pinned so
        # the graphics loaders and drivers the tests use stay reproducible.
        return "ubuntu-24.04-arm" if target_architecture == "arm64" else "ubuntu-24.04"
    if target_platform in {"macos", "ios", "ios-simulator", "tvos", "tvos-simulator"}:
        return "macos-26"
    if target_platform == "windows":
        return "windows-11-arm" if target_architecture == "arm64" else "windows-2022"
    if target_platform in {"android", "emscripten", "ohos"}:
        return "ubuntu-latest"
    raise SystemExit(f"error: cannot determine runner from preset {preset!r}")


def suite_commands(source: dict[str, object], preset: str) -> list[str]:
    commands = []
    for suite in source["suites"]:
        if platform(preset) not in suite["platforms"]:
            continue
        if preset in suite.get("exclude", []):
            continue
        for command in suite["commands"]:
            if (
                command.get("platforms")
                and platform(preset) not in command["platforms"]
            ):
                continue
            if command.get("include") and preset not in command["include"]:
                continue
            if preset in command.get("exclude", []):
                continue
            # A command with `preset = false` runs once for the job it lands in
            # rather than against the job's build tree.
            if command.get("preset", True):
                commands.append(f"mise run {command['task']} {preset}")
            else:
                commands.append(f"mise run {command['task']}")
    return commands


def android_commands(preset: str, abi: str, build_map: bool) -> list[str]:
    render_backend = "opengl" if backend(preset) == "egl" else backend(preset)
    arguments = f"{render_backend} {abi}"
    if preset in EMULATOR_TESTED:
        # Each device test task cross-compiles the artifact the build task
        # would, so it stands in for that command.
        commands = [
            f"mise run //bindings/kotlin:build {preset}",
            f"mise run //bindings/kotlin:test {preset}",
            f"mise run //bindings/go:test {preset}",
            f"mise run //bindings/rust:test {preset}",
            f"mise run //bindings/zig:test {preset}",
        ]
        commands.append(f"mise run //bindings/python:test {preset}")
    else:
        commands = [
            f"mise run //bindings/kotlin:build {preset}",
            f"mise run //bindings/kotlin:android-build {arguments} --prebuilt",
            f"mise run //bindings/go:build {preset}",
            f"mise run //bindings/rust:build {preset}",
            f"mise run //bindings/zig:build {preset}",
        ]
    if build_map:
        commands.append(f"mise run //examples/android-map:build {arguments} --prebuilt")
    commands.append(f"mise run //bindings/dart:build:mobile {preset}")
    return commands


def ohos_commands(preset: str) -> list[str]:
    # The emulator executes x64 EGL. Other OpenHarmony targets still prove that
    # each binding links against its backend-specific native artifact.
    action = "test" if preset in EMULATOR_TESTED else "build"
    return [
        f"mise run //bindings/rust:{action} {preset}",
        f"mise run //bindings/go:{action} {preset}",
    ]


def native_commands(preset: str, tested: set[str]) -> list[str]:
    target_platform = platform(preset)
    commands = [
        f"mise run {'test' if runtime_tested(preset, tested) else 'build'} {preset}"
    ]
    if target_platform == "linux":
        commands.append(f"mise run check-glibc-floor {preset}")
    return commands


def consumer_commands(source: dict[str, object], preset: str) -> list[str]:
    target_platform = platform(preset)
    commands = []
    if target_platform == "android":
        abi = "arm64-v8a" if architecture(preset) == "arm64" else "x86_64"
        commands.extend(android_commands(preset, abi, backend(preset) == "egl"))
    elif target_platform == "ohos":
        commands.extend(ohos_commands(preset))
    elif target_platform == "ios":
        commands.extend(
            [
                f"mise run //bindings/kotlin:ios-build {preset}",
                f"mise run //bindings/swift:build {preset}",
                "mise run //examples/swift-map:build:ios",
                f"mise run //bindings/dart:build:mobile {preset}",
            ]
        )
    elif target_platform == "ios-simulator":
        commands.extend(
            [
                f"mise run //bindings/kotlin:test {preset}",
                f"mise run //bindings/swift:test {preset}",
                f"mise run //bindings/zig:test {preset}",
                "mise run //examples/swift-map:build:ios-simulator",
                f"mise run //bindings/dart:build:mobile {preset}",
            ]
        )
    elif target_platform == "tvos-simulator":
        commands.extend(
            [
                f"mise run //bindings/zig:test {preset}",
            ]
        )
    elif target_platform in DESKTOP or target_platform == "emscripten":
        commands.extend(suite_commands(source, preset))
    return commands


ZIG_PROJECTS = (
    "mise run //bindings/zig:",
    "mise run //examples/zig-map:",
    "mise run //examples/zig-readback:",
)


def uses_zig(commands: list[str]) -> bool:
    """Whether a row fetches every Zig project the package cache key covers.

    The key hashes every `build.zig.zon` and is shared by all rows on the same
    runner. Cache keys are immutable, so a row that fetches only some projects
    would claim the key with an incomplete cache.
    """
    return all(
        any(command.startswith(project) for command in commands)
        for project in ZIG_PROJECTS
    )


GRADLE_PROJECTS = (
    "mise run //bindings/kotlin:",
    "mise run //examples/android-map:",
    "mise run //examples/compose-map:",
    "mise run //examples/lwjgl-map:",
    "mise run //:kotlin:",
)


def uses_gradle(commands: list[str]) -> bool:
    """Whether a row runs any Gradle build.

    `setup-gradle` restores a Gradle user home and stops the daemons in its
    post-action, so Windows file locks do not outlive the job. A row with no
    Gradle build pays that setup and teardown for an empty cache entry.
    """
    return any(
        command.startswith(project)
        for command in commands
        for project in GRADLE_PROJECTS
    )


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


def target_rows(
    source: dict[str, object], presets: dict[str, object]
) -> list[dict[str, object]]:
    configured, _, tested, packaged = preset_sets(presets)
    rows = []
    # The toolchain cache key covers the runner OS, architecture and image, so
    # one row per runner label is enough to write every entry the others read.
    claimed_runners: set[str] = set()
    for preset in configured:
        native = native_commands(preset, tested)
        consumers = consumer_commands(source, preset)
        row_runner = runner(preset)
        row = {
            "preset": preset,
            "runner": row_runner,
            "package": preset in packaged,
            "zig": uses_zig(native + consumers),
            "gradle": uses_gradle(native + consumers),
            "save_toolchains": row_runner not in claimed_runners,
            "native_commands": native if preset in packaged else native + consumers,
        }
        claimed_runners.add(row_runner)
        if preset in packaged:
            row["consumer_commands"] = consumers
        rows.append(row)
    return rows
