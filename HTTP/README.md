# PacketPlayground HTTP

A small HTTP/1.1 server written from scratch in C++ using WinSock and blocking TCP sockets.

This is my first attempt at a hands-on exploration of how HTTP actually works on top of TCP, from accepting connections and receiving arbitrary byte streams to parsing request lines, headers, message bodies, chunked transfer encoding, and persistent connections.

No HTTP library is used for request parsing or server functionality.

## Features

* HTTP/1.0 and HTTP/1.1 request parsing
* Blocking TCP sockets using WinSock
* `WSAPoll()` for single-threaded connection multiplexing
* Incremental request parsing using a finite-state machine
* Both `Content-Length` and `chunked`request bodies
* Keep-alive connections
* Connection idle timeouts
* Maximum connection and buffer limits
* Basic method/path routing
* Simple response builder API
* Simple logging with configurable log levels
* A small Python-based protocol test harness

## Components
### `Server`

Responsible for:

* Initializing WinSock
* Creating and binding the listening socket
* Accepting connections
* Multiplexing sockets with `WSAPoll()`
* Receiving request bytes
* Managing connection timeouts
* Dispatching parsed requests to routes
* Sending HTTP responses

### `Parser`

The HTTP parser is implemented as a state machine.

The parser is designed to handle incomplete input because TCP provides a byte stream rather than discrete messages.

For example, the following request does not have to arrive in a single `recv()` call:

```text
GET / HTTP/1.1\r
\nHost: localhost\r\n\r\n
```

The parser retains its state and continues when more bytes become available.

### `ResponseBuilder`

Provides a small fluent API for constructing responses:

```cpp
PPG::ResponseBuilder Builder;

return Builder
    .OK()
    .HTML()
    .Body("<h1>Hello World</h1>")
    .Build();
```

Predefined responses currently include:

* `200 OK`
* `400 Bad Request`
* `404 Not Found`

### `Logger`

A small singleton logger supporting:

```text
TRACE
DEBUG
INFO
WARNING
ERROR
CRITICAL
```

The log level can be changed at runtime:

```cpp
Logger::Get().SetLevel(LogLevel::LogTrace);
```

## Example

The current server exposes two routes.

### `GET /`

Returns:

```html
<h1>Hello World</h1>
```

### `POST /echo`

Returns the request body as JSON-compatible response content.

For example:

```http
POST /echo HTTP/1.1
Host: localhost
Content-Type: application/json
Content-Length: 13

{"hello":"hi"}
```

## Building

### Requirements

* Windows
* C++17 compiler
* CMake 3.26 or newer
* WinSock 2
* MinGW/GCC or another compiler compatible with the current build configuration
* Python 3 for the test harness

### CMake

Configure the project:

```bash
cmake -S . -B build
```

Build it:

```bash
cmake --build build
```

The resulting executable is:

```text
PPG.exe
```

The repository also contains Windows batch scripts for building, cleaning, and running the project.

## Running

The current example server listens on:

```text
127.0.0.1:8080
```

Start the server and send a request with a browser, `curl`, or another HTTP client.

For example:

```bash
curl http://127.0.0.1:8080/
```

And:

```bash
curl -X POST http://127.0.0.1:8080/echo -d "Hello from curl"
```

## Testing

The project includes a small Python test harness.

The test cases are stored in:

```text
tests/tests.csv
```

and executed by:

```text
tests/tests.py
```

The tests currently focus primarily on HTTP parsing and malformed requests, including:

* Minimal valid requests
* HTTP/1.0 requests
* Missing `Host`
* Empty `Host`
* Duplicate `Host`
* Unknown methods
* Invalid HTTP versions
* Malformed request lines
* Missing request targets
* Extra request-line tokens
* Absolute URIs
* Query strings
* Percent encoding
* Header count limits
* Header size limits
* Invalid header syntax
* Header-name case handling
* Duplicate ordinary headers

More protocol and edge-case tests will be added as the implementation evolves.

## Design Goals

This project is primarily an educational exercise.

The main goals are to understand:

1. How TCP sockets actually behave
2. Why a TCP `recv()` does not necessarily correspond to one HTTP request
3. How incremental protocol parsers work
4. How HTTP determines message boundaries
5. How `Content-Length` and chunked transfer encoding work
6. How persistent HTTP connections are managed
7. How malformed input can affect protocol parsers
8. How a relatively small state machine can parse a streaming protocol
9. How socket-level networking maps onto an application-layer protocol

The implementation intentionally favors explicit code and understandable state transitions over abstraction-heavy designs.

## Current Limitations

This is **not intended to be a production HTTP server**.

The implementation currently has a deliberately limited HTTP feature set.

Notable limitations include:

* Windows/WinSock specific implementation
* Single-threaded server
* Blocking sockets
* Limited HTTP method support
* Limited URI validation
* Only basic routing
* Limited response functionality
* Only `Transfer-Encoding: chunked` is currently supported
* No TLS/HTTPS
* No HTTP/2 or HTTP/3
* No compression
* No static file server
* No sophisticated routing parameters
* No virtual-host handling
* No authentication
* No production-grade security hardening

HTTP parsing is also an ongoing part of the project, so strict RFC compliance is not currently the goal.

## Why Build This?

It is easy to write:

```cpp
recv(socket, buffer, ...);
```

and think you have received an HTTP request.

You haven't.

TCP gives the application an ordered byte stream. A single request may arrive across multiple `recv()` calls, multiple requests may arrive in one `recv()`, and a request can contain arbitrary body data.

That makes HTTP parsing an interesting exercise in state management and message framing.

This project exists to explore that process from the bottom up instead of hiding it behind a web framework or HTTP library.

## Roadmap

Possible future work:

* [ ] Expand parser test coverage
* [ ] Test fragmented TCP input explicitly
* [ ] Test multiple HTTP requests in a single receive
* [ ] Improve persistent-connection handling
* [ ] Improve URI/request-target validation
* [ ] Improve `Connection` header parsing
* [ ] Improve `Transfer-Encoding` parsing
* [ ] Unify fixed-length and chunked request-body handling
* [ ] Improve response handling for `HEAD`, `204`, and other special responses
* [ ] Add more HTTP status codes
* [ ] Add configurable server settings
* [ ] Improve CMake portability
* [ ] Add automated parser unit tests
* [ ] Add fuzz testing for malformed HTTP input
* [ ] Investigate HTTP pipelining
* [ ] Add static-file serving
* [ ] Explore TLS
* [ ] Explore HTTP/2

## References

The implementation is informed primarily by the HTTP specifications:

* RFC 9110 — HTTP Semantics
* RFC 9112 — HTTP/1.1

The RFCs are especially useful when working on message framing, header fields, connection persistence, and chunked transfer encoding.

## License

Add your preferred license here.
