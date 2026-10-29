from __future__ import annotations

import base64
import os
import pathlib
import plistlib
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

import apple_sdk_signing as signing

TEAM = "ABCDEFGHIJ"
IDENTITY = "A" * 40
CONFIGURATION = {
    "APPLE_SDK_CERTIFICATE_BASE64": base64.b64encode(b"certificate").decode(),
    "APPLE_SDK_CERTIFICATE_PASSWORD": "export-password",
    "APPLE_SDK_TEAM_ID": TEAM,
}


class SigningIdentityTest(unittest.TestCase):
    def command(self, *args: str) -> str:
        if args[:2] == ("security", "find-identity"):
            return f'  1) {IDENTITY} "Apple Distribution: MapLibre ({TEAM})"\n'
        return ""

    def test_selects_the_publisher_and_deletes_its_keychain(self) -> None:
        with (
            mock.patch.dict(os.environ, CONFIGURATION),
            mock.patch.object(signing, "run", side_effect=self.command) as run,
        ):
            signing.sign(pathlib.Path("MaplibreNativeC.xcframework"))
        commands = [call.args for call in run.call_args_list]
        signature = next(
            args for args in commands if args[:2] == ("codesign", "--force")
        )
        self.assertIn("--timestamp", signature)
        self.assertEqual(signature[signature.index("--sign") + 1], IDENTITY)
        verification = next(
            args for args in commands if args[:2] == ("codesign", "--verify")
        )
        self.assertIn(
            f'anchor apple generic and certificate leaf[subject.OU] = "{TEAM}"',
            verification,
        )
        self.assertEqual(commands[-1][:2], ("security", "delete-keychain"))
        self.assertFalse(pathlib.Path(commands[-1][-1]).parent.exists())

    def test_rejects_a_different_team_and_still_deletes_the_keychain(self) -> None:
        with (
            mock.patch.dict(
                os.environ, {**CONFIGURATION, "APPLE_SDK_TEAM_ID": "OTHERTEAM1"}
            ),
            mock.patch.object(signing, "run", side_effect=self.command) as run,
            self.assertRaisesRegex(SystemExit, "Expected one valid Apple Distribution"),
        ):
            signing.sign(pathlib.Path("MaplibreNativeC.xcframework"))
        self.assertEqual(run.call_args.args[:2], ("security", "delete-keychain"))

    def test_command_failure_does_not_print_password_arguments(self) -> None:
        result = subprocess.CompletedProcess([], 1, stdout="", stderr="import failed")
        with (
            mock.patch.object(subprocess, "run", return_value=result),
            self.assertRaises(SystemExit) as error,
        ):
            signing.run("security", "import", "-P", "secret-password")
        self.assertNotIn("secret-password", str(error.exception))


@unittest.skipUnless(sys.platform == "darwin", "Requires Apple's codesign and ditto")
class SigningArchiveTest(unittest.TestCase):
    def test_packaging_preserves_the_signature_and_tampering_invalidates_it(
        self,
    ) -> None:
        # Ad-hoc signing exercises the real seal without a publisher's private key.
        def sign(bundle: pathlib.Path) -> None:
            signing.run("codesign", "--force", "--sign", "-", str(bundle))

        def verify(bundle: pathlib.Path, team: str) -> None:
            signing.run("codesign", "--verify", "--strict", str(bundle))

        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            bundle = root / "MaplibreNativeC.xcframework"
            bundle.mkdir()
            (bundle / "Info.plist").write_bytes(
                plistlib.dumps({"CFBundlePackageType": "XFWK"})
            )
            (bundle / "resource").write_text("sealed content")
            (bundle / "link").symlink_to("resource")
            archive = root / "MaplibreNativeC.xcframework.zip"
            signing.run("ditto", "-c", "-k", "--keepParent", str(bundle), str(archive))
            with (
                mock.patch.dict(os.environ, CONFIGURATION),
                mock.patch.object(signing, "sign", side_effect=sign),
                mock.patch.object(signing, "verify", side_effect=verify),
            ):
                signing.sign_archive(archive)
            extracted = root / "extracted"
            signing.run("ditto", "-x", "-k", str(archive), str(extracted))
            signed = extracted / bundle.name
            verify(signed, TEAM)
            self.assertTrue((signed / "link").is_symlink())
            flattened = root / "npm" / bundle.name
            shutil.copytree(signed, flattened, symlinks=False)
            with self.assertRaises(SystemExit):
                verify(flattened, TEAM)
            sign(flattened)
            verify(flattened, TEAM)
            (signed / "resource").write_text("tampered content")
            with self.assertRaises(SystemExit):
                verify(signed, TEAM)
