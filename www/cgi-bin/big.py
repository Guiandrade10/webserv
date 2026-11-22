#!/usr/bin/env python3
import sys

out = sys.stdout.buffer
out.write(b"Content-Type: application/octet-stream\r\n\r\n")
block = b"0123456789abcdef" * 4096
for _ in range(20 * 16):
    out.write(block)
out.flush()
