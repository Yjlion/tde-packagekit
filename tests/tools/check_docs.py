#!/usr/bin/env python3
"""Checks that every relative link and image in the Markdown docs resolves."""
import glob
import os
import re
import sys

LINK = re.compile(r"!?\[[^\]]*\]\(([^)\s]+)\)")
failures = 0
checked = 0
for path in ["README.md"] + sorted(glob.glob("doc/*.md")):
    with open(path, encoding="utf-8") as handle:
        for number, line in enumerate(handle, 1):
            for target in LINK.findall(line):
                if re.match(r"[a-z]+:", target) or target.startswith("#"):
                    continue
                checked += 1
                full = os.path.normpath(os.path.join(os.path.dirname(path), target.split("#")[0]))
                if not os.path.exists(full):
                    print(f"{path}:{number}: missing {target}")
                    failures += 1
print(f"{checked} links checked, {failures} broken")
sys.exit(1 if failures else 0)
