#!/usr/bin/env python3
import sys
import time

sys.stdout.write("Content-Type: text/plain\r\n\r\n")
sys.stdout.flush()
while True:
    sys.stdout.write("still running\n")
    sys.stdout.flush()
    time.sleep(5)
