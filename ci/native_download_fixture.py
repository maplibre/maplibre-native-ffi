"""Serve one native package as a temporary snapshot release."""

from __future__ import annotations

import argparse
import functools
import hashlib
import http.server
import os
import pathlib
import shutil
import subprocess
import tempfile
import threading

SNAPSHOT_TAG = "unstable-native-snapshot"
RELEASE_BASE_URL_ENV = "MAPLIBRE_NATIVE_C_TEST_SNAPSHOT_BASE_URL"


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    """Serve fixture files without writing an access log."""

    def log_message(self, format: str, *args: object) -> None:
        pass


def serve(archive: pathlib.Path, command: list[str]) -> int:
    """Run a command against a snapshot release containing one archive."""
    archive = archive.resolve(strict=True)
    with tempfile.TemporaryDirectory() as temporary:
        release = pathlib.Path(temporary, SNAPSHOT_TAG)
        release.mkdir()
        fixture_archive = release / archive.name
        try:
            os.link(archive, fixture_archive)
        except OSError:
            shutil.copy2(archive, fixture_archive)

        with archive.open("rb") as archive_file:
            digest = hashlib.file_digest(archive_file, "sha256").hexdigest()
        (release / "SHA256SUMS").write_text(
            f"{digest}  {archive.name}\n", encoding="utf-8"
        )

        handler = functools.partial(QuietHandler, directory=temporary)
        with http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            base_url = f"http://127.0.0.1:{server.server_port}"
            environment = os.environ.copy()
            environment[RELEASE_BASE_URL_ENV] = base_url
            try:
                return subprocess.run(command, env=environment, check=False).returncode
            finally:
                server.shutdown()
                thread.join()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=pathlib.Path)
    parser.add_argument("command", nargs=argparse.REMAINDER)
    arguments = parser.parse_args()
    command = arguments.command
    if command[:1] == ["--"]:
        command = command[1:]
    if not command:
        parser.error("a command is required after --")
    return serve(arguments.archive, command)


if __name__ == "__main__":
    raise SystemExit(main())
