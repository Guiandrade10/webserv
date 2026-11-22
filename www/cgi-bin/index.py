#!/usr/bin/env python3
print("Content-Type: text/html")
print()
print("<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>CGI scripts</title></head><body>")
print("<h1>CGI index (index.py)</h1><ul>")
for name in ["hello.py", "form.py?name=Ana&age=20", "env.py", "relative.py", "status.py",
             "redirect.py", "slow.py", "big.py", "crash.py", "bad_output.py", "infinite.py", "drip.py"]:
    print("<li><a href=\"/cgi-bin/%s\">%s</a></li>" % (name, name))
print("</ul></body></html>")
