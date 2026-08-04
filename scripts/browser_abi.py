"""Shared pieces of the browser module's generated ABI description.

The headers digest lives here rather than in each generator because both of them
produce it and the loader compares one against the other: two copies could only
ever disagree. Path separators are normalised for the same reason -- a module
built on Windows and a binding generated on Linux must digest identical headers
identically.
"""

from __future__ import annotations

import hashlib
import pathlib


def headers_digest(include: pathlib.Path) -> str:
    """Digests every public header under [include], path and contents."""
    lines = []
    for header in sorted(include.rglob("*.h")):
        digest = hashlib.sha256(header.read_bytes()).hexdigest().upper()
        # POSIX separators always: the digest describes the header set, not the
        # filesystem that happened to hold it while it was read.
        lines.append(f"{header.relative_to(include).as_posix()} {digest}\n")
    return hashlib.sha256("".join(lines).encode()).hexdigest().upper()
