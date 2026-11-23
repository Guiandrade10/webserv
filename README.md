*This project has been created as part of the 42 curriculum by gude-and, jde-carv, jose-vda*

# webserv

## Description

**webserv** is an HTTP/1.1 server written in C++98, without any external library. It is the
42 project where you build your own web server, in the spirit of NGINX: it reads a
configuration file, listens on several ports, and answers browsers, `curl` and other HTTP
clients.

An HTTP server waits for TCP connections, reads requests (`GET /index.html HTTP/1.1` plus
headers and an optional body), decides what to do with them and sends back a response (a
status line, headers and a body). webserv does it for many clients at the same time with a
single thread and **one `poll()` call**: every socket and pipe is non-blocking, and the server
only reads or writes a file descriptor when `poll()` says it is ready.

Feature overview:

- static website (HTML, CSS, JS, images), index files, directory listing (autoindex);
- file upload (`multipart/form-data` and raw bodies), download, and `DELETE`;
- CGI (Python and the 42 `cgi_tester`) with `GET` and `POST`, chunked bodies, timeouts;
- NGINX-like configuration: several ports, virtual hosts, routes, redirects, body limits,
  allowed methods, custom error pages;
- keep-alive, pipelining, chunked transfer encoding, timeouts on every state;
- never blocks, never crashes, no memory or file descriptor leak (checked with valgrind).

## Instructions

### Requirements

- Linux or macOS
- a C++ compiler (`c++`) supporting `-std=c++98`, and `make`
- `python3` (`/usr/bin/python3`) for the CGI scripts
- optional: `siege` for a stress test, `valgrind` for a leak check

### Build and run

```bash
make                              # builds ./webserv (make again: "Nothing to be done")
./webserv                         # uses config/default.conf
./webserv path/to/file.conf       # any other configuration
```

Relative paths in a configuration file are relative to the directory where `./webserv` is
started, so start it from the repository root. Stop it with `Ctrl+C` (clean shutdown, exit
code 0).

With `config/default.conf`:

| URL | What it shows |
|-----|---------------|
| http://localhost:8080/ | the demo panel: static site, listing, upload, delete, CGI, redirects, errors |
| http://localhost:8081/ | another site on another port, with the default error pages |
| `curl --resolve alpha.test:8082:127.0.0.1 http://alpha.test:8082/` | virtual host `alpha.test` (and `beta.test`) |

### 42 tester

Copy the two binaries from the intra into the `tester/` folder, then run `make test`:

```bash
cp /path/to/tester /path/to/cgi_tester tester/
make test        # builds webserv, creates the tester files, runs every test (2-6 minutes)
```

`make test` uses `config/tester.conf` on port 8080 (stop any other webserv first), answers
the tester's "press enter" prompts automatically and prints `RESULT: PASS` at the end.

## Resources

- [RFC 9110 — HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110) (methods, status codes, headers)
- [RFC 9112 — HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112) (message format, chunked, keep-alive)
- [RFC 3875 — The Common Gateway Interface (CGI) 1.1](https://www.rfc-editor.org/rfc/rfc3875)
- [NGINX documentation](https://nginx.org/en/docs/http/ngx_http_core_module.html): `server`,
  `location`, `root`, `error_page`, `client_max_body_size`
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) (sockets, `poll`)
- [MDN — HTTP response status codes](https://developer.mozilla.org/en-US/docs/Web/HTTP/Status)
- Manual pages: `man 2 poll`, `man 2 socket`, `man 2 accept`, `man 2 fork`, `man 2 execve`,
  `man 2 waitpid`, `man 7 tcp`

### Use of AI


During the development of **Webserv**, we used Claude as an assistance tool throughout different stages of the project.

AI was mainly used to support the learning process, clarify technical concepts, investigate problems, and improve development efficiency. Some of the topics where AI assistance was useful included:

- **Understanding concepts:** Explaining HTTP/1.1, TCP sockets, request parsing, response generation, CGI execution, non-blocking I/O, and other concepts related to web server architecture.
- **Problem-solving and debugging:** Helping investigate unexpected behaviour, interpret error messages, identify potential bugs, and explore possible solutions.
- **Code review:** Reviewing specific parts of the implementation, discussing edge cases, identifying potential memory management issues, and suggesting improvements in code readability and structure.
- **Testing:** Helping identify relevant test scenarios, including malformed requests, HTTP methods, status codes, connection handling, configuration parsing, and CGI behaviour.
- **Documentation:** Assisting with the organisation and improvement of technical documentation and README content.


## Features

- **HTTP/1.1**: `GET`, `POST`, `DELETE`; `HEAD` and other known methods answer `405` (with
  `Allow`) when not allowed; unknown methods answer `501`.
- **Incremental parser**: requests can arrive byte by byte or several at once (pipelining);
  bodies by `Content-Length` or `Transfer-Encoding: chunked`; strict limits and the right
  status for each error (`400`, `413`, `414`, `431`, `501`, `505`).
- **Static files** streamed from disk in 64 KiB blocks, correct `Content-Type`, index files,
  directory listing with absolute links.
- **Uploads**: `multipart/form-data` (browser forms, `curl -F`) and raw bodies; file names are
  sanitized (`../../x` becomes `x`); `201 Created` with `Location`.
- **DELETE**: `204 No Content`; directories are never removed.
- **CGI**: chosen by file extension; full environment (`REQUEST_METHOD`, `QUERY_STRING`,
  `CONTENT_LENGTH`, `PATH_INFO`, every header as `HTTP_*`...); the body is given de-chunked on
  stdin; the output is read through `poll()`; `502` for invalid output, `504` after 10 s without
  output; processes are always reaped (no zombies).
- **Configuration**: several servers and ports, virtual hosts (`server_name`), routes with
  their own root, redirects, body limits, allowed methods, custom error pages; every error in a
  configuration file is reported with its file and line.
- **Robustness**: timeouts for every state, lingering close (the client always receives an
  error such as `413`), at most 256 clients at a time, every exception caught, clean shutdown
  on `SIGINT`/`SIGTERM`.

## Configuration reference

```nginx
server {
    listen 127.0.0.1:8080;            # or just: listen 8080;
    server_name localhost site1.test;
    root ./www/site1;
    error_page 404 /errors/404.html;
    location /uploads {
        allowed_methods GET POST DELETE;
        upload_store ./www/site1/uploads;
        client_max_body_size 10M;
    }
    location /cgi-bin {
        root ./www/cgi-bin;           # the prefix is replaced: /cgi-bin/x.py -> ./www/cgi-bin/x.py
        cgi .py /usr/bin/python3;
    }
}
```

| Directive | Context | Arguments | Default |
|-----------|---------|-----------|---------|
| `listen` | server | `PORT` or `HOST:PORT` (repeatable) | required |
| `server_name` | server | one or more names | none |
| `root` | server, location | directory | required in `server` |
| `index` | server, location | one or more file names | `index.html` |
| `autoindex` | server, location | `on` \| `off` | `off` |
| `client_max_body_size` | server, location | size with optional `K`/`M`/`G` (`0` = unlimited) | `1M` |
| `error_page` | server | status codes... path (relative to the server root) | none |
| `allowed_methods` | location | one or more of `GET POST DELETE` | `GET` |
| `return` | location | `301`/`302`/`303`/`307`/`308` and a URL | none |
| `upload_store` | location | writable directory (enables uploads) | off |
| `cgi` | location | `.extension` and interpreter (repeatable) | none |
| `location` | server | URL prefix and a block | an implicit `/` |

A location inherits `index`, `autoindex` and `client_max_body_size` from its server. A
location with its own `root` replaces its prefix (subject example: `/kapouet` with root
`/tmp/www` maps `/kapouet/pouic/toto/pouet` to `/tmp/www/pouic/toto/pouet`); without `root`,
it uses `<server root>/<prefix>`. Two servers on the same `host:port` must have different
`server_name`s; the first one is the default for that address.

## Architecture

```
main ── ConfigParser ──► servers ──► EventLoop::run()   (the ONLY poll())
EventLoop
 ├─ Listener   one per host:port; knows the servers configured on it
 ├─ Client     one per connection; state machine
 │    ├─ RequestParser ──► HttpRequest (body in a temporary file)
 │    ├─ Router: server by Host header, location by longest prefix
 │    ├─ RequestHandler: redirect → 501 → 405 → CGI → GET / POST / DELETE
 │    └─ CgiProcess: fork/execve, stdout pipe watched by poll(), output in a temporary file
 └─ retired CGI processes, reaped with waitpid(WNOHANG)
```

Source folders: `src/config`, `src/core`, `src/http`, `src/handlers`, `src/cgi`, `src/utils`.

**Event loop** (one iteration): rebuild the `pollfd` vector from the state of every listener,
client socket and CGI pipe → `poll()` → one `accept`/`recv`/`send`/`read` per ready fd → start
the CGIs requested in this round → timeouts → kill and reap finished CGIs → close finished
clients. No file descriptor is closed while events are being dispatched.

**Client state machine**:

```
READING_REQUEST ──► SENDING_RESPONSE ──(keep-alive)──► READING_REQUEST
       │                  ▲          ──(close)──────► LINGERING ──► TO_CLOSE
       └► WAITING_FOR_CGI ┘
any state ──(error, timeout, disconnection)──► TO_CLOSE
```

**CGI lifecycle**: `pipe` + `fork`; the child redirects stdin to the body file and stdout to
the pipe, closes every other fd, `chdir`s to the script directory and `execve`s the
interpreter; the parent watches the pipe with `poll()` and copies the output to a temporary
file; at EOF the output headers become the response. A CGI that is no longer needed is killed
and reaped.

## Technical choices

- **`poll()`** (not `epoll`/`kqueue`): portable to Linux and macOS and simple to explain. The
  `pollfd` vector is rebuilt on every iteration, so it always matches the current state.
- **Only ask for what is needed**: `POLLOUT` only while there is something to send (otherwise
  `poll()` would return immediately forever). While a CGI runs, the client socket asks for
  `POLLIN` (up to 16 KiB buffered) so that a client that leaves is noticed and its CGI killed.
- **Deferred close**: file descriptors are closed at the end of the iteration, so a reused fd
  number can never receive events meant for the old one.
- **Bodies and CGI output on disk**: the 42 tester sends 20 bodies of 100 MB at the same time;
  kept in memory, this kills a server on an 8 GB machine. On disk, the server peaks at ~7 MiB.
- **No `errno` after I/O**: a call is made only after `poll()` reported the fd ready, so a
  negative return is an error and the client is dropped.
- **Lingering close**: after an error such as `413`, we `shutdown()` the writing side and drain
  the input for 2 s; closing a socket with unread data sends a reset and the client could lose
  the response.
- **Directory without index → 404, and `/dir` served without a redirect to `/dir/`**: required
  by the 42 tester (NGINX would answer 403 and 301). Autoindex links are therefore absolute.
- **`$PWD` for absolute paths**: CGI scripts run after `chdir()`, so configuration paths are
  made absolute at load time; `getcwd`/`realpath` are not allowed, `std::getenv("PWD")` is.
- **Strict configuration**: unknown directives, wrong values, missing directories or
  conflicting servers stop the server at startup with `file:line: reason`.
