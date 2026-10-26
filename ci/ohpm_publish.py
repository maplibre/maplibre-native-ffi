#!/usr/bin/env python3
"""Publishes HARs to OHPM with the official ohpm, skipping versions it has.

    ohpm_publish.py <ohpm> <har>...

OHPM_PUBLISH_ID, OHPM_PRIVATE_KEY, and OHPM_KEY_PASSPHRASE name the publisher.
ohpm signs its login with the private key, which it accepts only encrypted, and
reads the passphrase from a terminal in raw mode; a wrong passphrase makes it
ask again forever. So ohpm runs on a pseudo-terminal here, which answers the
prompt once and stops ohpm if it asks again. ohpm drops any input that holds a
control character, so the passphrase and the carriage return that ends it
arrive apart: the return follows the asterisk that ohpm echoes for the
passphrase.

A published version waits for OHPM's review before the registry lists it, so
a rerun after a publish that succeeded fails in ohpm rather than skipping it.
"""

from __future__ import annotations

import json
import os
import pathlib
import pty
import select
import subprocess
import sys
import tarfile
import tempfile
import termios
import urllib.error
import urllib.parse
import urllib.request

REGISTRY = "https://ohpm.openharmony.cn/ohpm/"
PROMPT = b"passphrase"
ECHO = b"*"


def manifest(har: pathlib.Path) -> dict:
    with tarfile.open(har) as archive:
        member = archive.extractfile("package/oh-package.json5")
        if member is None:
            raise SystemExit(f"{har} has no package/oh-package.json5")
        # pack-ohpm.sh writes the manifest as JSON.
        return json.load(member)


def published(registry: str, name: str, version: str) -> bool:
    url = registry + urllib.parse.quote(name, safe="@")
    try:
        with urllib.request.urlopen(url, timeout=60) as response:
            return version in json.load(response).get("versions", {})
    except urllib.error.HTTPError as error:
        with error:
            if error.code == 404:
                return False
            raise


def answer_once(command: list[str], passphrase: str) -> int:
    """Runs a command on a pseudo-terminal and answers its passphrase prompt."""
    controller, terminal = pty.openpty()
    # The terminal never echoes the answer into the log, whatever mode the
    # command reads it in.
    attributes = termios.tcgetattr(terminal)
    attributes[3] &= ~termios.ECHO
    termios.tcsetattr(terminal, termios.TCSANOW, attributes)
    process = subprocess.Popen(
        command, stdin=terminal, stdout=terminal, stderr=terminal, close_fds=True
    )
    os.close(terminal)
    seen = b""
    typed = entered = False
    try:
        while True:
            ready, _, _ = select.select([controller], [], [], 1)
            if not ready:
                if process.poll() is not None:
                    break
                continue
            try:
                chunk = os.read(controller, 4096)
            except OSError:
                # The terminal closes once the command exits.
                break
            if not chunk:
                break
            sys.stdout.buffer.write(chunk)
            sys.stdout.buffer.flush()
            seen = (seen + chunk)[-256:]
            if typed and not entered and ECHO in chunk:
                entered = True
                seen = b""
                os.write(controller, b"\r")
            elif PROMPT in seen.lower():
                if typed:
                    process.kill()
                    process.wait()
                    raise SystemExit("ohpm rejected the passphrase of the private key")
                typed = True
                seen = b""
                os.write(controller, passphrase.encode())
    finally:
        os.close(controller)
    return process.wait()


def publish(ohpm: str, hars: list[pathlib.Path], registry: str = REGISTRY) -> None:
    publish_id = os.environ["OHPM_PUBLISH_ID"]
    passphrase = os.environ["OHPM_KEY_PASSPHRASE"]
    with tempfile.TemporaryDirectory() as directory:
        key = pathlib.Path(directory, "key.pem")
        key.touch(mode=0o600)
        key.write_text(os.environ["OHPM_PRIVATE_KEY"])
        for har in hars:
            package = manifest(har)
            name, version = package["name"], package["version"]
            if published(registry, name, version):
                print(f"{name} {version} is already on OHPM")
                continue
            command = [
                ohpm,
                "publish",
                "--publish_registry",
                registry,
                "--publish_id",
                publish_id,
                "--key_path",
                str(key),
                str(har),
            ]
            if answer_once(command, passphrase) != 0:
                raise SystemExit(f"ohpm failed to publish {name} {version}")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        raise SystemExit("usage: ohpm_publish.py <ohpm> <har>...")
    publish(sys.argv[1], [pathlib.Path(har) for har in sys.argv[2:]])
