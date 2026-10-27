"""The suites that each native target's CI job runs after its build."""

from __future__ import annotations

import dataclasses
import re
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from tools.ci.plan import Target

DESKTOP = {"linux-gnu", "linux-musl", "macos", "windows"}

# Desktop suites run in the matching native target job and reuse its build tree.
#
# Every example's `smoke` task renders one frame and exits. A command marked
# `display` opens a window, so on Linux, whose runners have no display server,
# it runs under Xvfb and draws with Mesa's software drivers. Windows CI runs
# headless examples and covers window presentation in the native suite. A suite's
# `environments` set variables on its commands in the listed targets.
SUITES: list[dict] = [
    {
        "platforms": ["linux-gnu", "macos", "windows"],
        "commands": [
            {"task": "//bindings/rust:test", "exclude": ["linux-gnu-x64-vulkan"]},
            {
                "task": "//bindings/rust:test",
                "args": ["release"],
                "include": ["linux-gnu-x64-vulkan"],
            },
            {"task": "//examples/rust-map:check", "exclude": ["macos-arm64-egl"]},
            {
                "task": "//examples/rust-map:smoke",
                "exclude": ["macos-arm64-egl"],
                "display": True,
            },
        ],
    },
    # A glibc runner builds the consumers, which execute against their packaged
    # musl libraries in Alpine.
    {
        "platforms": ["linux-musl"],
        "commands": [
            {"task": "//bindings/rust:test"},
            {"task": "//bindings/dotnet:test"},
            {"task": "//bindings/go:test"},
            {"task": "//bindings/zig:test"},
            {"task": "//bindings/python:test"},
        ],
    },
    # Every other binding suite is handed a local install prefix. These commands
    # serve the package that this job built through a published crate's download
    # path. The path differs only in which preset it resolves, so one platform is
    # enough.
    {
        "platforms": ["linux-gnu"],
        "commands": [
            {
                "task": "//bindings/rust:test:package",
                "include": ["linux-gnu-x64-vulkan"],
            },
        ],
    },
    # Zig is unavailable on Windows ARM64; see the `core:zig` note in bindings/zig/mise.toml.
    {
        "platforms": ["linux-gnu", "macos", "windows"],
        "exclude": ["windows-arm64-wgl", "windows-arm64-vulkan"],
        "commands": [
            {"task": "//bindings/zig:test"},
            {"task": "//examples/zig-map:smoke", "display": True},
            {"task": "//examples/zig-readback:smoke"},
        ],
    },
    {
        "platforms": ["linux-gnu", "macos", "windows"],
        "commands": [{"task": "//bindings/kotlin:test"}],
    },
    {
        "platforms": ["linux-gnu", "macos"],
        "commands": [
            {"task": "//bindings/swift:test", "exclude": ["linux-gnu-x64-vulkan"]},
            {
                "task": "//bindings/swift:test",
                "args": ["release"],
                "include": ["linux-gnu-x64-vulkan"],
            },
            {"task": "//examples/swift-map:smoke", "include": ["macos-arm64-metal"]},
        ],
    },
    {
        "platforms": ["linux-gnu", "macos", "windows"],
        "commands": [
            {"task": "//bindings/dotnet:test"},
            {"task": "//examples/dotnet-map:smoke", "display": True},
        ],
    },
    {
        "platforms": ["linux-gnu", "macos"],
        "commands": [{"task": "//bindings/go:test"}],
    },
    # go-map drives OpenGL through EGL, on Linux only.
    {
        "platforms": ["linux-gnu"],
        "commands": [
            {
                "task": "//examples/go-map:check",
                "include": ["linux-gnu-x64-egl", "linux-gnu-arm64-egl"],
            },
            {
                "task": "//examples/go-map:smoke",
                "include": ["linux-gnu-x64-egl", "linux-gnu-arm64-egl"],
                "display": True,
            },
        ],
    },
    {
        "platforms": ["linux-gnu", "macos"],
        "commands": [{"task": "//examples/c-map:smoke", "display": True}],
    },
    # One target runs the suite again on the oldest supported Python.
    {
        "platforms": ["linux-gnu", "macos", "windows"],
        "commands": [
            {"task": "//bindings/python:test"},
            {
                "task": "//bindings/python:test:floor",
                "include": ["linux-gnu-x64-vulkan"],
            },
        ],
    },
    {
        "platforms": ["linux-gnu", "macos", "windows"],
        "commands": [{"task": "//bindings/dart:test"}],
    },
    # The TypeScript suite runs under Node, Bun, and Deno in turn, and loads the
    # packages that it packs and installs. The binding does not support Windows yet:
    # on Windows x64, the addon's first double in a process reached JavaScript as a
    # heap address, for a reason that is unknown.
    {
        "platforms": ["linux-gnu", "macos"],
        "commands": [{"task": "//bindings/typescript:test"}],
    },
    # The browser suites run against the Emscripten build in headless Chromium. The
    # Kotlin/Wasm suite runs on the page and links its own module from the package.
    # The canary passes only when a test that fails on purpose fails the Kotlin/Wasm
    # run, and one backend is enough to show that. The TypeScript suite runs on the
    # page and links its own test module.
    {
        "platforms": ["emscripten"],
        "commands": [
            {"task": "//bindings/rust:test"},
            {"task": "//bindings/typescript:test"},
            {"task": "//bindings/kotlin:test"},
            {
                "task": "//bindings/kotlin:test:wasm-canary",
                "include": ["emscripten-wasm32-webgl"],
            },
        ],
    },
]

# The CI steps that run before the suites. A suite after the archive and
# install also needs those to have succeeded.
BUILD = "build"
BOOT = "boot"


@dataclasses.dataclass(frozen=True)
class Suite:
    """One command that a target's job runs after its build, and how to run it."""

    id: str
    command: tuple[str, ...]
    env: dict[str, str]
    # `xvfb` runs the command on a virtual display, and `windows-console` runs
    # it in the Windows desktop session.
    wrapper: str | None
    shell: str | None
    timeout_minutes: int | None
    # The steps that must have succeeded for this one to run.
    prerequisites: tuple[str, ...]

    @property
    def task(self) -> str:
        return self.command[2]


def suite_id(task: str) -> str:
    return re.sub(r"[^A-Za-z0-9]+", "-", task).strip("-")


def desktop_commands(target: Target) -> list[tuple[list[str], dict[str, str], bool]]:
    """Each desktop suite command, its environment, and whether it opens a window."""
    preset = target.preset
    commands = []
    for suite in SUITES:
        if target.platform not in suite["platforms"]:
            continue
        if preset in suite.get("exclude", []):
            continue
        env = {}
        for environment in suite.get("environments", []):
            if preset in environment["include"]:
                env.update(environment["env"])
        for command in suite["commands"]:
            if command.get("include") and preset not in command["include"]:
                continue
            if preset in command.get("exclude", []):
                continue
            # Windows surface coverage lives in the native suite. Windowed
            # examples run on Linux and macOS; headless examples still run here.
            if command.get("display") and target.platform == "windows":
                continue
            arguments = [
                "mise",
                "run",
                command["task"],
                preset,
                *command.get("args", []),
            ]
            commands.append((arguments, dict(env), bool(command.get("display"))))
    return commands


def android_commands(target: Target, abi: str) -> list[str]:
    preset = target.preset
    render_backend = "opengl" if target.backend == "egl" else target.backend
    arguments = f"{render_backend} {abi}"
    if target.device:
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


def ohos_commands(target: Target) -> list[str]:
    # The emulator executes x64 EGL. Other OpenHarmony targets still prove that
    # each binding links against its backend-specific native artifact.
    action = "test" if target.device else "build"
    return [
        f"mise run //bindings/rust:{action} {target.preset}",
        f"mise run //bindings/go:{action} {target.preset}",
        f"mise run //bindings/typescript:{action} {target.preset}",
    ]


def apple_commands(target: Target) -> list[str]:
    preset = target.preset
    target_platform = target.platform
    if target_platform == "ios":
        return [
            f"mise run //bindings/kotlin:ios-build {preset}",
            f"mise run //bindings/swift:build {preset}",
            "mise run //examples/swift-map:build:ios",
            f"mise run //bindings/dart:build:mobile {preset}",
            f"mise run //bindings/rust:build {preset}",
        ]
    if target_platform == "ios-simulator":
        return [
            f"mise run //bindings/kotlin:test {preset}",
            f"mise run //bindings/swift:test {preset}",
            "mise run //examples/swift-map:build:ios-simulator",
            f"mise run //bindings/dart:test:ios {preset}",
            f"mise run //bindings/rust:test {preset}",
        ]
    if target_platform == "ios-maccatalyst":
        # Kotlin/Native has no Mac Catalyst target.
        return [
            f"mise run //bindings/swift:test {preset}",
            f"mise run //bindings/rust:test {preset}",
            f"mise run //bindings/zig:test {preset}",
        ]
    if target_platform == "tvos":
        return [
            f"mise run //bindings/kotlin:ios-build {preset}",
            f"mise run //bindings/swift:build {preset}",
        ]
    if target_platform == "tvos-simulator":
        return [
            f"mise run //bindings/kotlin:test {preset}",
            f"mise run //bindings/swift:test {preset}",
        ]
    if target_platform in {"visionos", "visionos-simulator"}:
        action = "test" if target_platform == "visionos-simulator" else "build"
        return [
            f"mise run //bindings/swift:{action} {preset}",
            f"mise run //bindings/rust:{action} {preset}",
        ]
    return []


def consumer_commands(target: Target) -> list[tuple[list[str], dict[str, str], bool]]:
    """The binding and example commands, as `desktop_commands` returns them."""
    target_platform = target.platform
    if target_platform in DESKTOP or target_platform == "emscripten":
        return desktop_commands(target)
    if target_platform == "android":
        abi = {
            "arm": "armeabi-v7a",
            "arm64": "arm64-v8a",
            "x64": "x86_64",
        }[target.architecture]
        lines = android_commands(target, abi)
    elif target_platform == "ohos":
        lines = ohos_commands(target)
    else:
        lines = apple_commands(target)
    return [(line.split(" "), {}, False) for line in lines]


def runtime_tested(target: Target) -> bool:
    """Whether CI executes this target's native suite rather than only building it.

    A target marked `ctest: false` runs its suite only on the device it names.
    """
    return target.tested or target.device is not None


def native_checks(target: Target) -> list[list[str]]:
    """The checks against the target's own build tree."""
    preset = target.preset
    target_platform = target.platform
    commands = []
    if runtime_tested(target):
        commands.append(["mise", "run", "test", preset])
    if target_platform == "linux-gnu":
        commands.append(["mise", "run", "check-glibc-floor", preset])
    elif target_platform == "linux-musl":
        commands.append(["mise", "run", "check-musl-abi", preset])
    return commands


def target_suites(target: Target) -> list[Suite]:
    """What `ci:target` runs for a target, in order, after its build and boot."""
    preset = target.preset
    # The job builds the target and boots its device in steps of their own
    # before this runs. Each prerequisite runs only after the ones before it
    # succeeded.
    native = (BUILD, BOOT) if target.device else (BUILD,)
    suites: list[Suite] = []

    def add(
        command: list[str],
        prerequisites: tuple[str, ...],
        env: dict[str, str],
        display: bool = False,
        step_id: str | None = None,
    ) -> None:
        wrapper, shell, timeout = None, None, None
        if display and target.platform == "linux-gnu":
            wrapper = "xvfb"
        # Lavapipe presents Win32 surfaces through GDI, which needs the VM's
        # interactive desktop. WGL and the binding suites run in service session 0.
        if preset == "windows-x64-vulkan" and command[2:] == ["test", preset]:
            wrapper, shell, timeout = "windows-console", "pwsh", 15
        suite = Suite(
            id=step_id or suite_id(command[2]),
            command=tuple(command),
            env=env,
            wrapper=wrapper,
            shell=shell,
            timeout_minutes=timeout,
            prerequisites=prerequisites,
        )
        # A failure names its suite's task, so a target runs each task once.
        if any(other.id == suite.id for other in suites):
            raise SystemExit(f"error: {preset} runs two suites with id {suite.id!r}")
        suites.append(suite)

    for command in native_checks(target):
        add(command, native, {})
    add(["mise", "run", "archive-native", preset], native, {}, step_id="archive")
    add(
        [
            "mise",
            "run",
            "install-native-package",
            preset,
            f"build/packages/native/dist/maplibre-native-c-{preset}.tar.gz",
        ],
        (*native, "archive"),
        {},
        step_id="install",
    )
    for command, env, display in consumer_commands(target):
        # The consumers run against the packaged install, which replaced the
        # build tree, so they must not rebuild it.
        add(
            command,
            (*native, "archive", "install"),
            {"MISE_TASK_SKIP": "//:build", **env},
            display,
        )
    return suites
