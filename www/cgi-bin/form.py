#!/usr/bin/env python3
import html
import os
import sys
from urllib.parse import parse_qsl

method = os.environ.get("REQUEST_METHOD", "GET")
if method == "POST":
    length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
    data = sys.stdin.buffer.read(length).decode("utf-8", "replace")
else:
    data = os.environ.get("QUERY_STRING", "")
fields = parse_qsl(data, keep_blank_values=True)

print("Content-Type: text/html; charset=utf-8")
print()
print("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>Form</title></head><body>")
print("<h1>%s form.py</h1><ul>" % html.escape(method))
for name, value in fields:
    print("<li>%s = %s</li>" % (html.escape(name), html.escape(value)))
print("</ul>")
print("<form method=\"post\" action=\"/cgi-bin/form.py\">"
      " <button type=\"submit\">POST</button></form>")
print("</body></html>")
