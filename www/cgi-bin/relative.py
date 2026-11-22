#!/usr/bin/env python3
with open("data.txt") as data:
    content = data.read()
print("Content-Type: text/plain")
print()
print(content, end="")
