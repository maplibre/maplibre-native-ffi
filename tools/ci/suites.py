"""The suites that each native target's CI job runs after its build.

A job builds its platform once. The native suites and each binding's tests run
once for every backend of the platform that support-gaps.toml lets them test
on the runner, and name that backend as their last argument. A binding that
can test on no backend builds instead, once.
"""

from __future__ import annotations

import dataclasses
import re
from typing import TYPE_CHECKING

from tools.ci import gaps

if TYPE_CHECKING:
    from tools.ci.plan import Target

# Kotlin/Native can't run on Linux ARM64, so the Linux x64 job compiles the
# Linux ARM64 KLIB, and the Linux ARM64 job leaves Kotlin/Native out.
LINUX_ARM64_KLIB = "//bindings/kotlin:linux-arm64-klib"

GOBJECT_ARCHIVE_PREFIX = "maplibre-native-ffi-gobject-"

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
    # `windows-console` runs the command in the Windows desktop session.
    wrapper: str | None
    shell: str | None
    timeout_minutes: int | None
    # The steps that must have succeeded for this one to run.
    prerequisites: tuple[str, ...]
    # The render backend whose suites it runs, if any.
    backend: str | None = None

    @property
    def task(self) -> str:
        return self.command[2]


def gobject_archive(target: Target) -> str | None:
    """The GObject archive that a target's job publishes, if any."""
    if not gaps.supported("gobject", target, "package"):
        return None
    return f"{GOBJECT_ARCHIVE_PREFIX}{target.preset}.tar.gz"


def suite_id(task: str) -> str:
    return re.sub(r"[^A-Za-z0-9]+", "-", task).strip("-")


def binding_command(binding: str, target: Target, backend: str | None) -> list[str]:
    """Select how a supported binding builds, or runs its tests for a backend,
    on the target's runner."""
    platform = target.platform
    preset = target.preset
    mobile = platform in {"android", "ios", "tvos", "visionos"}
    runnable = backend is not None
    action = "test" if runnable else "build"
    args = [preset]
    language = binding.split("-", 1)[0]
    if binding == "kotlin-jvm":
        if platform == "android":
            if runnable:
                action = "test:android"
            else:
                action = "android-build"
                abi = {
                    "arm": "armeabi-v7a",
                    "arm64": "arm64-v8a",
                    "x86": "x86",
                    "x64": "x86_64",
                }[target.architecture]
                args = [abi]
        else:
            action = "test:jvm" if runnable else "build:jvm"
    elif binding == "kotlin-native":
        if platform == "web":
            action = "test:wasm" if runnable else "build"
        elif runnable:
            action = "test:native"
        elif platform in {"ios", "tvos"}:
            action = "ios-build"
    elif binding == "dart":
        if platform == "android":
            action = "test:android" if runnable else "build:mobile"
        elif platform in {"ios", "ios-simulator"}:
            action = "test:ios" if runnable else "build:mobile"
    elif binding == "python" and platform == "android" and not runnable:
        action = "build-runtime"
    elif binding == "typescript" and (
        mobile or platform.endswith("-simulator") or platform == "ios-maccatalyst"
    ):
        action += ":hermes"
    if runnable:
        args.append(backend)
    if binding == "swift" and preset == "linux-gnu-x64":
        args.append("release")
    return ["mise", "run", f"//bindings/{language}:{action}", *args]


def consumer_commands(target: Target) -> list[tuple[list[str], str | None]]:
    """Every binding runs unless support-gaps.toml explicitly excludes it: its
    tests once for each backend that it can test, or else its build. Each
    command comes with the backend whose tests it runs, if any."""
    commands: list[tuple[list[str], str | None]] = []
    for binding in gaps.BINDING_LABELS:
        if binding == "c" or not gaps.supported(binding, target, "build"):
            continue
        if (
            binding == "kotlin-native"
            and target.platform == "linux-gnu"
            and target.architecture == "arm64"
        ):
            continue
        # Android also publishes the native KLIB, which the test executable
        # alone does not prove builds.
        if (
            binding == "kotlin-native"
            and target.platform == "android"
            and target.device
        ):
            commands.append(
                (["mise", "run", "//bindings/kotlin:build", target.preset], None)
            )
        tested = gaps.tested_backends(binding, target)
        for backend in tested or (None,):
            commands.append((binding_command(binding, target, backend), backend))
        if binding == "gobject" and gobject_archive(target):
            commands.append(
                (["mise", "run", "//bindings/gobject:archive", target.preset], None)
            )
        # These exercise additional contracts on one representative target,
        # with its default backend.
        extra = None
        if target.preset == "linux-gnu-x64":
            extra = {
                "go": "//bindings/go:test:package",
                "python": "//bindings/python:test:floor",
            }.get(binding)
        elif target.preset == "macos-arm64" and binding == "go":
            extra = "//bindings/go:test:package"
        elif target.preset == "web-wasm32-webgl" and binding == "kotlin-native":
            extra = "//bindings/kotlin:test:wasm-canary"
        backend = target.default_backend
        if extra and backend in tested:
            commands.append((["mise", "run", extra, target.preset, backend], backend))
        if target.preset == "linux-gnu-x64" and binding == "kotlin-native":
            commands.append((["mise", "run", LINUX_ARM64_KLIB], None))
    return commands


def command_support(command: str) -> list[tuple[str, str]]:
    """The support matrix cells established by a binding command."""
    match = re.search(r"mise run //bindings/([^: ]+):([^ ]+)", command)
    if match is None:
        return []
    binding, action = match.groups()
    if binding == "kotlin":
        if action == "android-build" or action in {
            "test:android",
            "test:jvm",
            "build:jvm",
        }:
            binding = "kotlin-jvm"
        else:
            binding = "kotlin-native"
    status = (
        "tested" if action == "test" or action.startswith("test:") else "build-only"
    )
    return [(binding, status)]


def native_checks(target: Target) -> list[tuple[list[str], str | None]]:
    """The checks against the target's own build tree: the native suites of
    each backend that the runner can test, and the platform's ABI checks."""
    preset = target.preset
    target_platform = target.platform
    commands: list[tuple[list[str], str | None]] = [
        (["mise", "run", "test", preset, backend], backend)
        for backend in gaps.tested_backends("c", target)
    ]
    if target_platform == "linux-gnu":
        commands.append((["mise", "run", "check-glibc-floor", preset], None))
    elif target_platform == "linux-musl":
        commands.append((["mise", "run", "check-musl-abi", preset], None))
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
        backend: str | None = None,
        step_id: str | None = None,
    ) -> None:
        wrapper, shell, timeout = None, None, None
        # Lavapipe presents Win32 surfaces through GDI, which needs the VM's
        # interactive desktop. WGL and the binding suites run in service session 0.
        if preset in {"windows-x64", "mingw-x64"} and command[2:] == [
            "test",
            preset,
            "vulkan",
        ]:
            wrapper, shell, timeout = "windows-console", "pwsh", 15
        suite = Suite(
            id=step_id or suite_id(command[2] + (f"-{backend}" if backend else "")),
            command=tuple(command),
            env=env,
            wrapper=wrapper,
            shell=shell,
            timeout_minutes=timeout,
            prerequisites=prerequisites,
            backend=backend,
        )
        # A failure names its suite's task, so a target runs each task once for
        # each backend.
        if any(other.id == suite.id for other in suites):
            raise SystemExit(f"error: {preset} runs two suites with id {suite.id!r}")
        suites.append(suite)

    for command, backend in native_checks(target):
        add(command, native, {}, backend)
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
    for command, backend in consumer_commands(target):
        # The consumers run against the packaged install, which replaced the
        # build tree, so they must not rebuild it.
        add(
            command,
            (*native, "archive", "install"),
            {"MISE_TASK_SKIP": "//:build"},
            backend,
        )
    return suites
