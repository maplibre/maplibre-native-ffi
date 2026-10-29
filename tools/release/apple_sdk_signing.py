"""Sign the final Apple SDK with an identity in an ephemeral keychain."""

from __future__ import annotations

import argparse
import base64
import os
import pathlib
import re
import secrets
import subprocess
import tempfile


def run(*command: str) -> str:
    # CalledProcessError includes argv, which can contain keychain passwords.
    result = subprocess.run(command, capture_output=True, text=True, check=False)
    if result.returncode:
        raise SystemExit(f"{command[0]} {command[1]} failed:\n{result.stderr}")
    return result.stdout


def verify(bundle: pathlib.Path, team: str) -> None:
    run(
        "codesign",
        "--verify",
        "--strict",
        "--verbose=2",
        "-R",
        f'anchor apple generic and certificate leaf[subject.OU] = "{team}"',
        str(bundle),
    )


def sign(bundle: pathlib.Path) -> None:
    configuration = {}
    for name in (
        "APPLE_SDK_CERTIFICATE_BASE64",
        "APPLE_SDK_CERTIFICATE_PASSWORD",
        "APPLE_SDK_TEAM_ID",
    ):
        value = os.environ.get(name)
        if not value:
            raise SystemExit(f"{name} is required for Apple SDK signing")
        configuration[name] = value
    team = configuration["APPLE_SDK_TEAM_ID"]
    if not re.fullmatch(r"[A-Z0-9]{10}", team):
        raise SystemExit("APPLE_SDK_TEAM_ID must be a ten-character Apple Team ID")

    with tempfile.TemporaryDirectory() as directory:
        work = pathlib.Path(directory)
        certificate = work / "identity.p12"
        certificate.write_bytes(
            base64.b64decode(
                configuration["APPLE_SDK_CERTIFICATE_BASE64"], validate=True
            )
        )
        certificate.chmod(0o600)
        keychain = str(work / "signing.keychain-db")
        password = secrets.token_urlsafe(32)
        run("security", "create-keychain", "-p", password, keychain)
        try:
            run("security", "set-keychain-settings", "-lut", "3600", keychain)
            run("security", "unlock-keychain", "-p", password, keychain)
            run(
                "security",
                "import",
                str(certificate),
                "-k",
                keychain,
                "-P",
                configuration["APPLE_SDK_CERTIFICATE_PASSWORD"],
                "-T",
                "/usr/bin/codesign",
            )
            run(
                "security",
                "set-key-partition-list",
                "-S",
                "apple-tool:,apple:,codesign:",
                "-s",
                "-k",
                password,
                keychain,
            )
            identities = run(
                "security", "find-identity", "-v", "-p", "codesigning", keychain
            )
            matches = re.findall(
                rf'^\s*\d+\) ([A-Fa-f0-9]{{40}}) "Apple Distribution: .+ \({team}\)"$',
                identities,
                re.MULTILINE,
            )
            if len(matches) != 1:
                raise SystemExit(
                    "Expected one valid Apple Distribution identity for APPLE_SDK_TEAM_ID"
                )
            run(
                "codesign",
                "--force",
                "--timestamp",
                "--sign",
                matches[0],
                "--keychain",
                keychain,
                str(bundle),
            )
            verify(bundle, team)
        finally:
            run("security", "delete-keychain", keychain)


def sign_archive(archive: pathlib.Path) -> None:
    with tempfile.TemporaryDirectory() as directory:
        work = pathlib.Path(directory)
        run("ditto", "-x", "-k", str(archive), str(work))
        bundle = work / "MaplibreNativeC.xcframework"
        sign(bundle)
        signed = work / archive.name
        run(
            "ditto",
            "-c",
            "-k",
            "--norsrc",
            "--noextattr",
            "--keepParent",
            str(bundle),
            str(signed),
        )
        extracted = work / "verify"
        run("ditto", "-x", "-k", str(signed), str(extracted))
        verify(extracted / bundle.name, os.environ["APPLE_SDK_TEAM_ID"])
        archive.write_bytes(signed.read_bytes())


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=pathlib.Path)
    sign(parser.parse_args().bundle)
