from __future__ import annotations

import http.server
import io
import json
import os
import pathlib
import sys
import tarfile
import tempfile
import threading
import unittest
from typing import ClassVar
from unittest import mock

from ohpm_publish import publish

# Asks for the passphrase on a terminal in raw mode until it gets the right
# one, as ohpm does: it drops a chunk of input that holds a control character,
# echoes an asterisk for each chunk it keeps, and ends the answer at a carriage
# return. It then records the arguments it published with.
FAKE_OHPM = """\
import os, sys, tty
if not sys.stdin.isatty():
    raise SystemExit("stdin is not a terminal")
tty.setraw(0)
while True:
    os.write(1, b"what is your passphrase of the private key: ")
    answer = b""
    while (chunk := os.read(0, 1024)) != b"\\r":
        if not any(byte < 32 for byte in chunk):
            answer += chunk
            os.write(1, b"*")
    if answer.decode() == os.environ["EXPECTED"]:
        break
with open(os.environ["RECORD"], "a") as record:
    record.write(" ".join(sys.argv[1:]) + "\\n")
"""


class Registry(http.server.BaseHTTPRequestHandler):
    packages: ClassVar[dict[str, list[str]]] = {}

    def do_GET(self) -> None:
        name = self.path.removeprefix("/ohpm/").replace("%2F", "/")
        if name not in self.packages:
            self.send_error(404)
            return
        body = json.dumps(
            {"versions": {version: {} for version in self.packages[name]}}
        ).encode()
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, *args: object) -> None:
        pass


class OhpmPublishTest(unittest.TestCase):
    def setUp(self) -> None:
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.directory = pathlib.Path(temporary.name)
        self.record = self.directory / "record.txt"
        ohpm = self.directory / "ohpm"
        ohpm.write_text(f"#!{sys.executable}\n{FAKE_OHPM}")
        ohpm.chmod(0o755)
        self.ohpm = str(ohpm)

        server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Registry)
        threading.Thread(target=server.serve_forever, daemon=True).start()
        self.addCleanup(server.server_close)
        self.addCleanup(server.shutdown)
        self.registry = f"http://127.0.0.1:{server.server_address[1]}/ohpm/"
        Registry.packages = {"@maplibre/native-ffi-opengl": ["0.202611.0"]}

        environment = mock.patch.dict(
            os.environ,
            {
                "OHPM_PUBLISH_ID": "publisher",
                "OHPM_PRIVATE_KEY": "-----BEGIN RSA PRIVATE KEY-----\n",
                "OHPM_KEY_PASSPHRASE": "secret",
                "EXPECTED": "secret",
                "RECORD": str(self.record),
            },
        )
        environment.start()
        self.addCleanup(environment.stop)

    def har(self, name: str, version: str) -> pathlib.Path:
        path = self.directory / f"{name.split('/')[1]}-{version}.har"
        manifest = json.dumps({"name": name, "version": version}).encode()
        with tarfile.open(path, "w:gz") as archive:
            info = tarfile.TarInfo("package/oh-package.json5")
            info.size = len(manifest)
            archive.addfile(info, io.BytesIO(manifest))
        return path

    def published(self) -> list[str]:
        return self.record.read_text().splitlines() if self.record.exists() else []

    def test_publishes_a_new_version_and_skips_a_listed_one(self) -> None:
        listed = self.har("@maplibre/native-ffi-opengl", "0.202611.0")
        new = self.har("@maplibre/native-ffi-vulkan", "0.202611.0")

        publish(self.ohpm, [listed, new], self.registry)

        (arguments,) = self.published()
        self.assertTrue(arguments.startswith("publish --publish_registry"))
        self.assertIn("--publish_id publisher", arguments)
        self.assertTrue(arguments.endswith(str(new)))

    def test_stops_ohpm_when_it_rejects_the_passphrase(self) -> None:
        os.environ["EXPECTED"] = "another"
        new = self.har("@maplibre/native-ffi-vulkan", "0.202611.0")

        with self.assertRaisesRegex(SystemExit, "rejected the passphrase"):
            publish(self.ohpm, [new], self.registry)
        self.assertEqual(self.published(), [])


if __name__ == "__main__":
    unittest.main()
