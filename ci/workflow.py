from __future__ import annotations

import json
import pathlib
import tomllib


DESKTOP = {"linux", "macos", "windows"}


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
    return preset.split("-", 1)[0]


def architecture(preset: str) -> str:
    parts = preset.split("-")
    for candidate in ("x64", "arm64"):
        if candidate in parts:
            return candidate
    raise SystemExit(f"error: cannot determine architecture from preset {preset!r}")


def backend(preset: str) -> str:
    value = preset.rsplit("-", 1)[-1]
    if value not in {"egl", "metal", "vulkan", "wgl"}:
        raise SystemExit(f"error: cannot determine backend from preset {preset!r}")
    return value


def runner(preset: str) -> str:
    target_platform = platform(preset)
    target_architecture = architecture(preset)
    if target_platform == "linux":
        # Linux runners are pinned because the runner image's glibc sets the
        # minimum glibc our published natives require. See the glibc_floor
        # variable in mise.linux.toml.
        return "ubuntu-24.04-arm" if target_architecture == "arm64" else "ubuntu-24.04"
    if target_platform in {"macos", "ios", "ios-simulator"}:
        return "macos-26"
    if target_platform == "windows":
        return "windows-11-arm" if target_architecture == "arm64" else "windows-2022"
    if target_platform in {"android", "ohos"}:
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
            commands.append(f"mise run {command['task']} {preset}")
    return commands


def android_commands(preset: str, abi: str, build_map: bool) -> list[str]:
    render_backend = "opengl" if backend(preset) == "egl" else backend(preset)
    arguments = f"{render_backend} {abi}"
    commands = [f"mise run //bindings/kotlin:androidBuild {arguments} --prebuilt"]
    if build_map:
        commands.append(f"mise run //examples/android-map:build {arguments} --prebuilt")
    return commands


def native_commands(preset: str, tested: set[str]) -> list[str]:
    target_platform = platform(preset)
    if target_platform == "ohos":
        return [f"mise run //bindings/rust:build:ohos {preset}"]
    commands = [f"mise run {'test' if preset in tested else 'build'} {preset}"]
    if target_platform == "linux":
        commands.append(f"mise run check-glibc-floor {preset}")
    return commands


def consumer_commands(source: dict[str, object], preset: str) -> list[str]:
    target_platform = platform(preset)
    commands = []
    if target_platform == "android":
        abi = "arm64-v8a" if architecture(preset) == "arm64" else "x86_64"
        commands.extend(android_commands(preset, abi, backend(preset) == "egl"))
    elif target_platform == "ios":
        commands.extend(
            [
                f"mise run //bindings/kotlin:iosBuild {preset}",
                "mise run //bindings/swift:build:ios",
                "mise run //examples/swift-map:build:ios",
            ]
        )
    elif target_platform == "ios-simulator":
        commands.extend(
            [
                f"mise run //bindings/kotlin:iosBuild {preset}",
                "mise run //bindings/swift:build:ios-simulator",
                "mise run //bindings/zig:test:ios-simulator",
                "bash scripts/run-ios-simulator-test.sh bindings/swift/.build/ios-simulator/arm64-apple-ios-simulator/debug/MaplibreNativeIOSSimulatorTests 120",
                "mise run //examples/swift-map:build:ios-simulator",
            ]
        )
    elif target_platform in DESKTOP:
        commands.extend(suite_commands(source, preset))
    return commands


ZIG_PROJECTS = (
    "mise run //bindings/zig:",
    "mise run //examples/zig-map:",
    "mise run //examples/zig-readback:",
)


def uses_zig(commands: list[str]) -> bool:
    """Whether a row fetches every Zig project the package cache key covers.

    The key hashes every `build.zig.zon` in the repository and is shared by all
    rows on the same runner OS and architecture. Cache keys are immutable, so a
    row that runs only some of those projects would win the key with a cache
    that is missing the rest, and Zig's fetcher has no retry: the rows that do
    run the examples would then fail whenever an upstream host is unavailable.
    """
    return all(
        any(command.startswith(project) for command in commands)
        for project in ZIG_PROJECTS
    )


def preset_sets(
    presets: dict[str, object],
) -> tuple[list[str], set[str], set[str], set[str]]:
    configured = [
        preset["name"]
        for preset in presets["configurePresets"]
        if not preset.get("hidden", False)
    ]
    built = {preset["name"] for preset in presets.get("buildPresets", [])}
    tested = {preset["name"] for preset in presets.get("testPresets", [])}
    packaged = {preset["name"] for preset in presets.get("packagePresets", [])}
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
    for preset in configured:
        native = native_commands(preset, tested)
        consumers = consumer_commands(source, preset)
        row = {
            "preset": preset,
            "runner": runner(preset),
            "package": preset in packaged,
            "zig": uses_zig(native + consumers),
            "native_commands": native if preset in packaged else native + consumers,
        }
        if preset in packaged:
            row["consumer_commands"] = consumers
        rows.append(row)
    return rows
