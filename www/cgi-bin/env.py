#!/usr/bin/env python3
import os

print("Content-Type: text/plain")
print()
for name in sorted(os.environ):
    print("%s=%s" % (name, os.environ[name]))
