"""The suites that each native target's CI job runs after its build."""

from __future__ import annotations

import dataclasses
import re
from typing import TYPE_CHECKING

from tools.ci import gaps

if TYPE_CHECKING:
    from tools.ci.plan import Target

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

    @property
    def task(self) -> str:
        return self.command[2]


def suite_id(task: str) -> str:
    return re.sub(r"[^A-Za-z0-9]+", "-", task).strip("-")


def binding_command(binding: str, target: Target) -> list[str]:
    """Select how a supported binding builds or runs on the target's runner."""
    platform = target.platform
    preset = target.preset
    mobile = platform in {"android", "ios", "tvos", "visionos"}
    runnable = gaps.supported(binding, target, "test")
    action = "test" if runnable else "build"
    args = [preset]
    language = binding.split("-", 1)[0]
    if binding == "kotlin-jvm":
        if platform == "android":
            if not runnable:
                action = "android-build"
                backend = "opengl" if target.backend == "egl" else target.backend
                abi = {
                    "arm": "armeabi-v7a",
                    "arm64": "arm64-v8a",
                    "x86": "x86",
                    "x64": "x86_64",
                }[target.architecture]
                args = [backend, abi]
        else:
            action = "test:jvm" if runnable else "build:jvm"
    elif binding == "kotlin-native":
        if platform == "emscripten":
            action = "test:wasm" if runnable else "wasm-module"
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
    if binding in {"rust", "swift"} and preset == "linux-gnu-x64-vulkan":
        args.append("release")
    return ["mise", "run", f"//bindings/{language}:{action}", *args]


def consumer_commands(target: Target) -> list[list[str]]:
    """Every binding runs unless support-gaps.toml explicitly excludes it."""
    commands = []
    for binding in gaps.BINDING_LABELS:
        if binding == "c" or not gaps.supported(binding, target, "build"):
            continue
        # Android also publishes the native KLIB, which the test executable
        # alone does not prove builds.
        if (
            binding == "kotlin-native"
            and target.platform == "android"
            and target.device
        ):
            commands.append(["mise", "run", "//bindings/kotlin:build", target.preset])
        commands.append(binding_command(binding, target))
        # These exercise additional contracts on one representative target.
        extra = None
        if target.preset == "linux-gnu-x64-vulkan":
            extra = {
                "rust": "//bindings/rust:test:package",
                "python": "//bindings/python:test:floor",
            }.get(binding)
        elif target.preset == "emscripten-wasm32-webgl" and binding == "kotlin-native":
            extra = "//bindings/kotlin:test:wasm-canary"
        if extra and gaps.supported(binding, target, "test"):
            commands.append(["mise", "run", extra, target.preset])
    return commands


def command_support(command: str) -> list[tuple[str, str]]:
    """The support matrix cells established by a binding command."""
    match = re.search(r"mise run //bindings/([^: ]+):([^ ]+)", command)
    if match is None:
        return []
    binding, action = match.groups()
    if binding == "kotlin":
        if (
            action == "android-build"
            or action == "test"
            or action in {"test:jvm", "build:jvm"}
        ):
            binding = "kotlin-jvm"
        else:
            binding = "kotlin-native"
    status = (
        "tested" if action == "test" or action.startswith("test:") else "build-only"
    )
    return [(binding, status)]


def native_checks(target: Target) -> list[list[str]]:
    """The checks against the target's own build tree."""
    preset = target.preset
    target_platform = target.platform
    commands = []
    if gaps.supported("c", target, "test"):
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
        step_id: str | None = None,
    ) -> None:
        wrapper, shell, timeout = None, None, None
        # Lavapipe presents Win32 surfaces through GDI, which needs the VM's
        # interactive desktop. WGL and the binding suites run in service session 0.
        if preset in {"windows-x64-vulkan", "mingw-x64-vulkan"} and command[2:] == [
            "test",
            preset,
        ]:
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
    for command in consumer_commands(target):
        # The consumers run against the packaged install, which replaced the
        # build tree, so they must not rebuild it.
        add(
            command,
            (*native, "archive", "install"),
            {"MISE_TASK_SKIP": "//:build"},
        )
    return suites
