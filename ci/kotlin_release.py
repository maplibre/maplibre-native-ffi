#!/usr/bin/env python3

import argparse
import base64
import hashlib
import http.client
import json
import os
import pathlib
import shutil
import subprocess
import tempfile
import urllib.parse

GROUP_PATH = pathlib.PurePosixPath("org/maplibre/nativeffi")
CHECKSUM_SUFFIXES = (".md5", ".sha1", ".sha256", ".sha512")
SNAPSHOT_USER_AGENT = (
    "maplibre-native-ffi-publisher/1 (+https://github.com/maplibre/maplibre-native-ffi)"
)


def repository_files(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(path for path in root.rglob("*") if path.is_file())


def secret_key_fingerprint(gnupg_home: pathlib.Path) -> str:
    result = subprocess.run(
        [
            "gpg",
            "--batch",
            "--homedir",
            os.fspath(gnupg_home),
            "--with-colons",
            "--list-secret-keys",
        ],
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    )
    fingerprints: list[str] = []
    expect_primary_fingerprint = False
    for line in result.stdout.splitlines():
        fields = line.split(":")
        record_type = fields[0]
        if record_type == "sec":
            expect_primary_fingerprint = True
        elif record_type == "fpr" and expect_primary_fingerprint:
            fingerprints.append(fields[9])
            expect_primary_fingerprint = False
        elif record_type not in {"fpr", "grp", "uid", "uat"}:
            expect_primary_fingerprint = False
    if len(fingerprints) != 1:
        raise SystemExit(
            f"the release signing key contains {len(fingerprints)} primary secret keys; "
            "expected one"
        )
    return fingerprints[0]


def import_signing_key(gnupg_home: pathlib.Path, private_key: str) -> str:
    subprocess.run(
        ["gpg", "--batch", "--homedir", os.fspath(gnupg_home), "--import"],
        input=private_key.encode(),
        check=True,
    )
    return secret_key_fingerprint(gnupg_home)


def sign_file(
    path: pathlib.Path,
    gnupg_home: pathlib.Path,
    fingerprint: str,
    passphrase: str,
) -> None:
    signature = path.with_name(path.name + ".asc")
    subprocess.run(
        [
            "gpg",
            "--batch",
            "--yes",
            "--homedir",
            os.fspath(gnupg_home),
            "--pinentry-mode",
            "loopback",
            "--passphrase-fd",
            "0",
            "--local-user",
            fingerprint,
            "--armor",
            "--detach-sign",
            "--output",
            os.fspath(signature),
            os.fspath(path),
        ],
        input=(passphrase + "\n").encode(),
        check=True,
    )
    verification = subprocess.run(
        [
            "gpg",
            "--batch",
            "--homedir",
            os.fspath(gnupg_home),
            "--verify",
            os.fspath(signature),
            os.fspath(path),
        ],
        capture_output=True,
        check=False,
    )
    if verification.returncode != 0:
        message = verification.stderr.decode(errors="replace")
        raise SystemExit(f"signature verification failed for {path}: {message}")


def write_checksum(path: pathlib.Path, algorithm: str) -> None:
    digest = hashlib.new(algorithm)
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
    path.with_name(path.name + f".{algorithm}").write_text(
        digest.hexdigest() + "\n", encoding="ascii"
    )


def module_names(repository: pathlib.Path) -> list[str]:
    group_root = repository / GROUP_PATH
    if not group_root.is_dir():
        raise SystemExit(f"the Maven repository is missing {group_root}")
    return sorted(path.name for path in group_root.iterdir() if path.is_dir())


def version_root(repository: pathlib.Path, module: str, version: str) -> pathlib.Path:
    root = repository / GROUP_PATH / module / version
    if not root.is_dir():
        raise SystemExit(f"the Maven repository is missing {root}")
    return root


def is_root_module(repository: pathlib.Path, module: str, version: str) -> bool:
    """A Kotlin Multiplatform root module points each target at its own module."""
    metadata = next(version_root(repository, module, version).glob("*.module"))
    variants = json.loads(metadata.read_text(encoding="utf-8"))["variants"]
    return any("available-at" in variant for variant in variants)


def sign_files(paths: list[pathlib.Path], work_directory: pathlib.Path) -> None:
    private_key = os.environ.get("GPG_PRIVATE_KEY")
    passphrase = os.environ.get("GPG_PASSPHRASE")
    if private_key is None or passphrase is None:
        raise SystemExit("GPG_PRIVATE_KEY and GPG_PASSPHRASE are required")
    with tempfile.TemporaryDirectory(dir=work_directory) as temporary_directory:
        gnupg_home = pathlib.Path(temporary_directory) / "gnupg"
        gnupg_home.mkdir(mode=0o700)
        fingerprint = import_signing_key(gnupg_home, private_key)
        for path in paths:
            sign_file(path, gnupg_home, fingerprint, passphrase)
            write_checksum(path, "md5")
            write_checksum(path, "sha1")


def is_signature_or_checksum(path: pathlib.Path) -> bool:
    return path.name.endswith((".asc", *CHECKSUM_SUFFIXES))


def create_bundle(repository: pathlib.Path, output: pathlib.Path, version: str) -> None:
    if output.suffix != ".zip":
        raise SystemExit(f"the bundle output must end in .zip: {output}")
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent) as temporary_directory:
        bundle_root = pathlib.Path(temporary_directory) / "repository"
        release_files: list[pathlib.Path] = []
        for module in module_names(repository):
            for source in repository_files(version_root(repository, module, version)):
                if source.name == "maven-metadata.xml" or is_signature_or_checksum(
                    source
                ):
                    continue
                destination = bundle_root / source.relative_to(repository)
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, destination)
                release_files.append(destination)
        sign_files(release_files, output.parent)
        shutil.make_archive(os.fspath(output.with_suffix("")), "zip", bundle_root)


def sign_snapshot(repository: pathlib.Path, version: str) -> None:
    release_files: list[pathlib.Path] = []
    for module in module_names(repository):
        for path in repository_files(version_root(repository, module, version)):
            if path.name.startswith("maven-metadata.xml"):
                continue
            if is_signature_or_checksum(path):
                path.unlink()
            else:
                release_files.append(path)
    sign_files(release_files, repository.parent)


def snapshot_module_files(
    repository: pathlib.Path, module: str, version: str
) -> list[pathlib.Path]:
    version_files = repository_files(version_root(repository, module, version))
    artifacts = [
        path for path in version_files if not path.name.startswith("maven-metadata.xml")
    ]
    version_metadata = [
        path for path in version_files if path.name.startswith("maven-metadata.xml")
    ]
    module_metadata = sorted(
        path
        for path in (repository / GROUP_PATH / module).iterdir()
        if path.is_file() and path.name.startswith("maven-metadata.xml")
    )
    return artifacts + version_metadata + module_metadata


def snapshot_upload_order(repository: pathlib.Path, version: str) -> list[pathlib.Path]:
    """Files to upload, every target module before the root modules that point
    at them, so a consumer never resolves a root whose targets are missing."""
    modules = module_names(repository)
    roots = [
        module for module in modules if is_root_module(repository, module, version)
    ]
    leaves = [module for module in modules if module not in roots]
    return [
        path
        for module in leaves + roots
        for path in snapshot_module_files(repository, module, version)
    ]


def upload_snapshot(repository: pathlib.Path, version: str) -> None:
    username = os.environ.get("MAVEN_CENTRAL_USERNAME")
    password = os.environ.get("MAVEN_CENTRAL_PASSWORD")
    if username is None or password is None:
        raise SystemExit(
            "MAVEN_CENTRAL_USERNAME and MAVEN_CENTRAL_PASSWORD are required"
        )
    credentials = base64.b64encode(f"{username}:{password}".encode()).decode("ascii")
    connection = http.client.HTTPSConnection("central.sonatype.com", timeout=120)
    try:
        for path in snapshot_upload_order(repository, version):
            relative = path.relative_to(repository).as_posix()
            print(f"uploading {relative}", flush=True)
            with path.open("rb") as body:
                connection.request(
                    "PUT",
                    "/repository/maven-snapshots/" + urllib.parse.quote(relative),
                    body=body,
                    headers={
                        "Authorization": f"Basic {credentials}",
                        "User-Agent": SNAPSHOT_USER_AGENT,
                        "Content-Type": "application/octet-stream",
                        "Content-Length": str(path.stat().st_size),
                    },
                )
                response = connection.getresponse()
                response_body = response.read()
            if not 200 <= response.status < 300:
                message = response_body.decode(errors="replace")[:1000]
                raise SystemExit(
                    f"snapshot upload of {relative} returned HTTP "
                    f"{response.status}: {message}"
                )
    finally:
        connection.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="operation", required=True)

    bundle_parser = subparsers.add_parser("bundle")
    bundle_parser.add_argument("repository", type=pathlib.Path)
    bundle_parser.add_argument("output", type=pathlib.Path)
    bundle_parser.add_argument("version")

    sign_snapshot_parser = subparsers.add_parser("sign-snapshot")
    sign_snapshot_parser.add_argument("repository", type=pathlib.Path)
    sign_snapshot_parser.add_argument("version")

    upload_snapshot_parser = subparsers.add_parser("upload-snapshot")
    upload_snapshot_parser.add_argument("repository", type=pathlib.Path)
    upload_snapshot_parser.add_argument("version")

    args = parser.parse_args()
    if args.operation == "bundle":
        create_bundle(args.repository, args.output, args.version)
    elif args.operation == "sign-snapshot":
        sign_snapshot(args.repository, args.version)
    elif args.operation == "upload-snapshot":
        upload_snapshot(args.repository, args.version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
