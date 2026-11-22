#!/usr/bin/env bash
set -u
cd "$(dirname "$0")/.." || exit 1
root="$(pwd)"
dir="$root/tester"

for bin in tester cgi_tester; do
    if [ ! -f "$dir/$bin" ]; then
        echo "missing tester/$bin: copy the 42 binaries into $dir/" >&2
        exit 1
    fi
    chmod +x "$dir/$bin"
done

mkdir -p "$dir/www" "$dir/uploads" "$dir/YoupiBanane/nop" "$dir/YoupiBanane/Yeah" "$dir/logs"
printf '<!DOCTYPE html>\n<html><body><h1>webserv: 42 tester root</h1></body></html>\n' > "$dir/www/index.html"
for f in youpi.bad_extension youpi.bla youpla.bla nop/youpi.bad_extension nop/other.pouic Yeah/not_happy.bad_extension; do
    printf '%s\n' "$f" > "$dir/YoupiBanane/$f"
done
rm -f "$dir"/uploads/* 2>/dev/null

[ -x ./webserv ] || { echo "build first: make" >&2; exit 1; }
if (exec 3<>/dev/tcp/127.0.0.1/8080) 2>/dev/null; then
    echo "port 8080 is already in use: stop the other server first" >&2
    exit 1
fi

PWD="$root" ./webserv config/tester.conf > "$dir/logs/server.log" 2>&1 &
server_pid=$!
for _ in $(seq 1 100); do
    (exec 3<>/dev/tcp/127.0.0.1/8080) 2>/dev/null && break
    sleep 0.1
done

yes '' | "$dir/tester" http://localhost:8080 | tee "$dir/logs/tester.raw"
tester_exit=${PIPESTATUS[1]}

server_alive=yes
kill -0 "$server_pid" 2>/dev/null || server_alive=no
kill -INT "$server_pid" 2>/dev/null
wait "$server_pid"
server_exit=$?

echo "------------------------------------------------------------"
echo "tester exit code : $tester_exit   (0 = every test passed)"
echo "server alive     : $server_alive"
echo "server exit code : $server_exit   (0 = clean shutdown on SIGINT)"
echo "server log       : tester/logs/server.log"
if [ "$tester_exit" -eq 0 ] && [ "$server_alive" = yes ] && [ "$server_exit" -eq 0 ]; then
    echo "RESULT: PASS"; exit 0
fi
echo "RESULT: FAIL"; exit 1
