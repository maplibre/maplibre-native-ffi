"""Serve one native package as a temporary release."""

from __future__ import annotations

import argparse
import functools
import http.server
import os
import pathlib
import shutil
import subprocess
import tempfile
import threading

RELEASE_URL_ENV = "MAPLIBRE_NATIVE_FFI_RELEASE_URL"


class QuietHandler(http.server.SimpleHTTPRequestHandler):
    """Serve fixture files without writing an access log."""

    def log_message(self, format: str, *args: object) -> None:
        pass


def serve(archive: pathlib.Path, tag: str, command: list[str]) -> int:
    """Run a command against a release `tag` containing one archive."""
    archive = archive.resolve(strict=True)
    with tempfile.TemporaryDirectory() as temporary:
        release = pathlib.Path(temporary, tag)
        release.mkdir()
        fixture_archive = release / archive.name
        try:
            os.link(archive, fixture_archive)
        except OSError:
            shutil.copy2(archive, fixture_archive)

        handler = functools.partial(QuietHandler, directory=temporary)
        with http.server.ThreadingHTTPServer(("127.0.0.1", 0), handler) as server:
            thread = threading.Thread(target=server.serve_forever, daemon=True)
            thread.start()
            environment = os.environ.copy()
            environment[RELEASE_URL_ENV] = f"http://127.0.0.1:{server.server_port}"
            try:
                return subprocess.run(command, env=environment, check=False).returncode
            finally:
                server.shutdown()
                thread.join()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=pathlib.Path)
    parser.add_argument("tag")
    parser.add_argument("command", nargs=argparse.REMAINDER)
    arguments = parser.parse_args()
    command = arguments.command
    if command[:1] == ["--"]:
        command = command[1:]
    if not command:
        parser.error("a command is required after --")
    return serve(arguments.archive, arguments.tag, command)


if __name__ == "__main__":
    raise SystemExit(main())
