#!/usr/bin/env python3
"""Print the test executables from `cargo test --no-run --message-format=json`.

Reads cargo's JSON messages on standard input and prints the executable path of
each test artifact, one per line, for the emulator test runners to push.
"""

import json
import sys

for line in sys.stdin:
    message = json.loads(line)
    if (
        message.get("reason") == "compiler-artifact"
        and message.get("profile", {}).get("test")
        and message.get("executable") is not None
    ):
        print(message["executable"])
