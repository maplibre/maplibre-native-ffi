"""Run the release app on CI's device and require the suite and a clean reload."""

import argparse
import os
import queue
import re
import subprocess
import threading
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
WORK = ROOT / "build/react-native-spike"


def command(*args):
    return subprocess.check_output(args, text=True).strip()


def run(preset):
    if preset.startswith("android-"):
        abi = "x86_64" if "-x64-" in preset else "arm64-v8a"
        serial = command(
            str(ROOT / "tools/devices/android-serial"),
            abi,
            "--api",
            "26"
            if preset == "android-x64-egl"
            else os.environ["MLN_FFI_ANDROID_EMULATOR_API"],
        )
        adb = str(Path(os.environ["ANDROID_HOME"]) / "platform-tools/adb")
        device = [adb, "-s", serial]
        package = "com.helloworld"
        subprocess.run(
            [
                *device,
                "install",
                "-r",
                str(WORK / "app/android/app/build/outputs/apk/release/app-release.apk"),
            ],
            check=True,
        )
        subprocess.run([*device, "shell", "am", "force-stop", package], check=True)
        log = [
            *device,
            "logcat",
            "-T",
            "1",
            "-s",
            "MlnLifecycle:V",
            "ReactNativeJS:V",
            "AndroidRuntime:E",
        ]
        launch = [*device, "shell", "am", "start", "-n", f"{package}/.MainActivity"]
    else:
        udid = command("devices-cli", "get", "ios")
        package = "org.reactjs.native.example.HelloWorld"
        subprocess.run(
            [
                "xcrun",
                "simctl",
                "install",
                udid,
                str(
                    WORK
                    / "derived/Build/Products/Release-iphonesimulator/HelloWorld.app"
                ),
            ],
            check=True,
        )
        subprocess.run(["xcrun", "simctl", "terminate", udid, package], check=False)
        log = [
            "xcrun",
            "simctl",
            "spawn",
            udid,
            "log",
            "stream",
            "--level",
            "debug",
            "--style",
            "compact",
            "--predicate",
            'process == "HelloWorld"',
        ]
        launch = ["xcrun", "simctl", "launch", udid, package]
    lines = queue.Queue()
    process = subprocess.Popen(
        log, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True
    )

    def read():
        for line in process.stdout:
            lines.put(line)
        lines.put(None)

    threading.Thread(target=read, daemon=True).start()
    passed = set()
    released = set()
    deadline = time.monotonic() + 180
    try:
        subprocess.run(launch, check=True)
        with (WORK / f"{preset}-runtime.log").open("w") as output:
            while passed != {1, 2} or released != {1}:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise RuntimeError(
                        f"Timed out: suite runtimes {passed}, released runtimes {released}"
                    )
                try:
                    line = lines.get(timeout=remaining)
                except queue.Empty as error:
                    raise RuntimeError("React Native suite timed out") from error
                if line is None:
                    raise RuntimeError(
                        "Device log stream ended before the suite passed"
                    )
                output.write(line)
                output.flush()
                if "Mln" in line or "FAIL " in line:
                    print(line, end="", flush=True)
                if (
                    "FAIL " in line
                    or "FATAL EXCEPTION" in line
                    or "Unhandled JS Exception" in line
                ):
                    raise RuntimeError("React Native suite failed; see runtime log")
                match = re.search(r"MlnBinding (\d+) passed", line)
                if match:
                    passed.add(int(match[1]))
                match = re.search(r"MlnLifecycle:? (\d+) realm-released", line)
                if match:
                    released.add(int(match[1]))
        print("React Native suite, readback and runtime reload passed")
    finally:
        process.terminate()
        process.wait(timeout=10)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "preset",
        choices=[
            "android-x64-egl",
            "android-x64-vulkan",
            "android-arm64-egl",
            "android-arm64-vulkan",
            "ios-simulator-arm64-metal",
        ],
    )
    run(parser.parse_args().preset)
