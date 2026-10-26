from __future__ import annotations

import json
import pathlib
import shlex
import tomllib

DESKTOP = {"linux-gnu", "linux-musl", "macos", "windows"}

# Targets whose suite runs on an emulator instead of through ctest, so CMake
# registers no test preset for them.
EMULATOR_TESTED = {
    "android-x64-egl",
    "android-x64-vulkan",
    "ohos-x64-egl",
}


# Linux runners have no display server. A command that opens a window gets a
# virtual one, and draws on the X11 platform rather than the surfaceless one
# that the job exports for the headless suites.
XVFB_RUN = "env -u EGL_PLATFORM xvfb-run --auto-servernum"


def mise_command(command: str) -> str:
    """The `mise run` command a step runs, without the display wrapper."""
    return command.removeprefix(f"{XVFB_RUN} ")


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


def suite_commands(source: dict[str, object], preset: str) -> list[str]:
    return [line for line, _ in suite_steps(source, preset)]


def suite_environment(source: dict[str, object], preset: str) -> dict[str, dict]:
    """The step environment of each suite command that has one, by command."""
    return {line: env for line, env in suite_steps(source, preset) if env}


def suite_steps(
    source: dict[str, object], preset: str
) -> list[tuple[str, dict[str, str]]]:
    commands = []
    for suite in source["suites"]:
        if platform(preset) not in suite["platforms"]:
            continue
        if preset in suite.get("exclude", []):
            continue
        env = {}
        for environment in suite.get("environments", []):
            if preset in environment["include"]:
                env.update(environment["env"])
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
            # Windows surface coverage lives in the native suite. Windowed
            # examples run on Linux and macOS; headless examples still run here.
            if command.get("display") and platform(preset) == "windows":
                continue
            arguments = [str(command["task"])]
            # A command with `preset = false` runs once for the job it lands in
            # rather than against the job's build tree.
            if command.get("preset", True):
                arguments.append(preset)
            arguments.extend(str(argument) for argument in command.get("args", []))
            line = f"mise run {shlex.join(arguments)}"
            if command.get("display") and platform(preset) == "linux-gnu":
                line = f"{XVFB_RUN} {line}"
            commands.append((line, dict(env)))
    return commands


def android_commands(preset: str, abi: str) -> list[str]:
    render_backend = "opengl" if backend(preset) == "egl" else backend(preset)
    arguments = f"{render_backend} {abi}"
    if preset in EMULATOR_TESTED:
        # Each device test task cross-compiles the artifact the build task
        # would, so it stands in for that command.
        commands = [
            f"mise run //bindings/kotlin:build {preset}",
            f"mise run //bindings/kotlin:test {preset}",
            f"mise run //bindings/kotlin:test:native {preset}",
            f"mise run //bindings/go:test {preset}",
            f"mise run //bindings/rust:test {preset}",
            f"mise run //bindings/zig:test {preset}",
            f"mise run //bindings/dart:test:android {preset}",
        ]
        # Python replaces the EGL suite's API 26 emulator with a newer API,
        # so run it after every suite that reuses the API 26 device.
        commands.append(f"mise run //bindings/python:test {preset}")
    else:
        commands = [
            f"mise run //bindings/kotlin:build {preset}",
            f"mise run //bindings/kotlin:android-build {arguments}",
            f"mise run //bindings/go:build {preset}",
            f"mise run //bindings/rust:build {preset}",
            f"mise run //bindings/zig:build {preset}",
            f"mise run //bindings/dart:build:mobile {preset}",
        ]
    return commands


def device_boot(preset: str) -> tuple[str, str] | None:
    """The named step that boots the device a target's suites run on, if any.

    The suites boot the device themselves when it is not ready, and every boot
    task reuses a ready device. Booting in a step of its own lets CI tell a
    device that never came up from a suite that failed on it.
    """
    target_platform = platform(preset)
    if target_platform == "android" and preset in EMULATOR_TESTED:
        # The EGL target's suites run on API 26, as the root `test` task and
        # the device test scripts boot it.
        api = " --api 26" if backend(preset) == "egl" else ""
        return "Boot Android emulator", f"mise run //:android-emulator:boot x86_64{api}"
    if target_platform == "ohos" and preset in EMULATOR_TESTED:
        return "Boot OpenHarmony emulator", "mise run //:ohos-emulator:boot"
    if target_platform == "freebsd":
        return "Boot FreeBSD virtual machine", "mise run //:freebsd-vm:boot"
    if target_platform == "ios-simulator":
        return "Boot iOS simulator", "mise run //:ios-simulator:boot"
    if target_platform == "visionos-simulator":
        return "Boot visionOS simulator", "mise run //:visionos-simulator:boot"
    if target_platform == "tvos-simulator":
        return "Boot tvOS simulator", "mise run //:tvos-simulator:boot"
    return None


SWIFT_PROJECTS = ("mise run //bindings/swift:", "mise run //examples/swift-map:")


def uses_swift(commands: list[str]) -> bool:
    """Whether a row builds a Swift package, which resolves its dependencies."""
    return any(mise_command(command).startswith(SWIFT_PROJECTS) for command in commands)


def ohos_commands(preset: str) -> list[str]:
    # The emulator executes x64 EGL. Other OpenHarmony targets still prove that
    # each binding links against its backend-specific native artifact.
    action = "test" if preset in EMULATOR_TESTED else "build"
    return [
        f"mise run //bindings/rust:{action} {preset}",
        f"mise run //bindings/go:{action} {preset}",
        f"mise run //bindings/typescript:{action} {preset}",
    ]


def native_commands(preset: str, tested: set[str]) -> list[str]:
    """The build, then the checks against its tree. The build always comes first."""
    target_platform = platform(preset)
    # The build is its own step so that a failing C suite leaves the binding
    # suites a tree to run against.
    commands = [f"mise run build {preset}"]
    if runtime_tested(preset, tested):
        commands.append(f"mise run test {preset}")
    if target_platform == "linux-gnu":
        commands.append(f"mise run check-glibc-floor {preset}")
    elif target_platform == "linux-musl":
        commands.append(f"mise run check-musl-abi {preset}")
    return commands


def consumer_commands(source: dict[str, object], preset: str) -> list[str]:
    target_platform = platform(preset)
    commands = []
    if target_platform == "android":
        abi = {
            "arm": "armeabi-v7a",
            "arm64": "arm64-v8a",
            "x64": "x86_64",
        }[architecture(preset)]
        commands.extend(android_commands(preset, abi))
    elif target_platform == "ohos":
        commands.extend(ohos_commands(preset))
    elif target_platform == "ios":
        commands.extend(
            [
                f"mise run //bindings/kotlin:ios-build {preset}",
                f"mise run //bindings/swift:build {preset}",
                "mise run //examples/swift-map:build:ios",
                f"mise run //bindings/dart:build:mobile {preset}",
                f"mise run //bindings/rust:build {preset}",
            ]
        )
    elif target_platform == "ios-simulator":
        commands.extend(
            [
                f"mise run //bindings/kotlin:test {preset}",
                f"mise run //bindings/swift:test {preset}",
                "mise run //examples/swift-map:build:ios-simulator",
                f"mise run //bindings/dart:test:ios {preset}",
                f"mise run //bindings/rust:test {preset}",
            ]
        )
    elif target_platform == "ios-maccatalyst":
        # Kotlin/Native has no Mac Catalyst target.
        commands.extend(
            [
                f"mise run //bindings/swift:test {preset}",
                f"mise run //bindings/rust:test {preset}",
                f"mise run //bindings/zig:test {preset}",
            ]
        )
    elif target_platform == "tvos":
        commands.extend(
            [
                f"mise run //bindings/kotlin:ios-build {preset}",
                f"mise run //bindings/swift:build {preset}",
            ]
        )
    elif target_platform == "tvos-simulator":
        commands.extend(
            [
                f"mise run //bindings/kotlin:test {preset}",
                f"mise run //bindings/swift:test {preset}",
            ]
        )
    elif target_platform in {"visionos", "visionos-simulator"}:
        action = "test" if target_platform == "visionos-simulator" else "build"
        commands.extend(
            [
                f"mise run //bindings/swift:{action} {preset}",
                f"mise run //bindings/rust:{action} {preset}",
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
        any(mise_command(command).startswith(project) for command in commands)
        for project in ZIG_PROJECTS
    )


GRADLE_PROJECTS = (
    "mise run //bindings/kotlin:",
    "mise run //:kotlin:",
)


def uses_gradle(commands: list[str]) -> bool:
    """Whether a row runs any Gradle build.

    `setup-gradle` restores a Gradle user home and stops the daemons in its
    post-action, so Windows file locks do not outlive the job. A row with no
    Gradle build pays that setup and teardown for an empty cache entry.
    """
    return any(
        mise_command(command).startswith(project)
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
            "swift": uses_swift(native + consumers),
            "boot": device_boot(preset),
            "save_toolchains": row_runner not in claimed_runners,
            "native_commands": native if preset in packaged else native + consumers,
            "environment": suite_environment(source, preset),
        }
        claimed_runners.add(row_runner)
        if preset in packaged:
            row["consumer_commands"] = consumers
        rows.append(row)
    return rows
