from __future__ import annotations

import json
import pathlib
import tomllib


DESKTOP = {"linux", "macos", "windows"}


def load_configuration(
    root: pathlib.Path,
) -> tuple[dict[str, object], list[dict[str, str]]]:
    with (root / "ci" / "workflow.toml").open("rb") as file:
        source = tomllib.load(file)
    targets = json.loads((root / "native-targets.json").read_text())
    return source, targets


def platform(target: str) -> str:
    if target.startswith("ios-simulator-"):
        return "ios-simulator"
    return target.split("-", 1)[0]


def architecture(target: str) -> str:
    parts = target.split("-")
    for candidate in ("x64", "arm64"):
        if candidate in parts:
            return candidate
    raise SystemExit(f"error: cannot determine architecture from target {target!r}")


def backend(target: str) -> str:
    value = target.rsplit("-", 1)[-1]
    if value not in {"egl", "metal", "vulkan", "wgl"}:
        raise SystemExit(f"error: cannot determine backend from target {target!r}")
    return value


def runner(target: str) -> str:
    target_platform = platform(target)
    target_architecture = architecture(target)
    if target_platform == "linux":
        # Linux runners are pinned because the runner image's glibc sets the
        # minimum glibc our published natives require. See the glibc_floor
        # variable in mise.linux.toml.
        return "ubuntu-24.04-arm" if target_architecture == "arm64" else "ubuntu-24.04"
    if target_platform in {"macos", "ios", "ios-simulator"}:
        return "macos-26"
    if target_platform == "windows":
        # Zig 0.16 cannot run reliably on Windows ARM64, but its x64 build can
        # cross-compile the complete ARM64 dependency graph on an x64 runner.
        return "windows-2022"
    if target_platform in {"android", "ohos"}:
        return "ubuntu-latest"
    raise SystemExit(f"error: cannot determine runner from target {target!r}")


def suite_commands(source: dict[str, object], target: str) -> list[str]:
    commands = []
    for suite in source["suites"]:
        if platform(target) not in suite["platforms"]:
            continue
        if target in suite.get("exclude", []):
            continue
        for command in suite["commands"]:
            if (
                command.get("platforms")
                and platform(target) not in command["platforms"]
            ):
                continue
            if command.get("include") and target not in command["include"]:
                continue
            if target in command.get("exclude", []):
                continue
            commands.append(f"mise run {command['task']} {target}")
    return commands


def android_commands(target: str, abi: str, build_map: bool) -> list[str]:
    render_backend = "opengl" if backend(target) == "egl" else backend(target)
    arguments = f"{render_backend} {abi}"
    commands = [f"mise run //bindings/kotlin:androidBuild {arguments} --prebuilt"]
    if build_map:
        commands.append(f"mise run //examples/android-map:build {arguments} --prebuilt")
    return commands


def native_commands(target: str, tested: set[str]) -> list[str]:
    target_platform = platform(target)
    if target_platform == "ohos":
        return [f"mise run //bindings/rust:build:ohos {target}"]
    commands = [f"mise run {'test' if target in tested else 'build'} {target}"]
    if target_platform == "linux":
        commands.append(f"mise run check-glibc-floor {target}")
    return commands


def consumer_commands(source: dict[str, object], target: str) -> list[str]:
    target_platform = platform(target)
    commands = []
    if target_platform == "android":
        abi = "arm64-v8a" if architecture(target) == "arm64" else "x86_64"
        commands.extend(android_commands(target, abi, backend(target) == "egl"))
    elif target_platform == "ios":
        commands.extend(
            [
                f"mise run //bindings/kotlin:iosBuild {target}",
                "mise run //bindings/swift:build:ios",
                "mise run //examples/swift-map:build:ios",
            ]
        )
    elif target_platform == "ios-simulator":
        commands.extend(
            [
                f"mise run //bindings/kotlin:iosBuild {target}",
                "mise run //bindings/swift:build:ios-simulator",
                "mise run //bindings/zig:test:ios-simulator",
                "bash scripts/run-ios-simulator-test.sh bindings/swift/.build/ios-simulator/arm64-apple-ios-simulator/debug/MaplibreNativeIOSSimulatorTests 120",
                "mise run //examples/swift-map:build:ios-simulator",
            ]
        )
    elif target_platform in DESKTOP:
        commands.extend(suite_commands(source, target))
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


GRADLE_PROJECTS = (
    "mise run //bindings/kotlin:",
    "mise run //examples/android-map:",
    "mise run //examples/compose-map:",
    "mise run //examples/lwjgl-map:",
    "mise run //:kotlin:",
)


def uses_gradle(commands: list[str]) -> bool:
    """Whether a row runs any Gradle build.

    `setup-gradle` restores a Gradle user home, and stops the daemons in its
    post-action so Windows file locks do not outlive the job. A row that runs no
    Gradle build has neither to manage, and pays the action's setup and teardown
    for an empty cache entry.
    """
    return any(
        command.startswith(project)
        for command in commands
        for project in GRADLE_PROJECTS
    )


def target_sets(
    targets: list[dict[str, str]],
) -> tuple[list[str], set[str], set[str]]:
    configured = [target["name"] for target in targets]
    if len(configured) != len(set(configured)):
        raise SystemExit("error: native target names must be unique")
    tested = {
        target
        for target in configured
        if platform(target) in DESKTOP | {"ios-simulator"}
        and not (platform(target) == "windows" and architecture(target) == "arm64")
    }
    packaged = {target for target in configured if platform(target) != "ohos"}
    return configured, tested, packaged


def target_rows(
    source: dict[str, object], targets: list[dict[str, str]]
) -> list[dict[str, object]]:
    configured, tested, packaged = target_sets(targets)
    rows = []
    # The toolchain cache key covers the runner OS, architecture and image, so
    # one row per runner label is enough to write every entry the others read.
    claimed_runners: set[str] = set()
    for target in configured:
        native = native_commands(target, tested)
        consumers = consumer_commands(source, target)
        row_runner = runner(target)
        row = {
            "target": target,
            "runner": row_runner,
            "package": target in packaged,
            "zig": uses_zig(native + consumers),
            "gradle": uses_gradle(native + consumers),
            "save_toolchains": row_runner not in claimed_runners,
            "native_commands": native if target in packaged else native + consumers,
        }
        claimed_runners.add(row_runner)
        if target in packaged:
            row["consumer_commands"] = consumers
        rows.append(row)
    return rows
