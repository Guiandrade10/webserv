#!/usr/bin/env python3
import os
import time

print("Content-Type: text/html")
print()
print("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>CGI</title></head><body>")
print("<h1>Hello from CGI</h1>")
print("<p>Method: %s</p>" % os.environ.get("REQUEST_METHOD", "?"))
print("<p>Time: %s</p>" % time.strftime("%Y-%m-%d %H:%M:%S"))
print("</body></html>")
