# Web Template (Crow)

Minimal C++ web server using the Crow framework (header-only, located in `../third/`).

The application code in `app/main.cpp` uses only C++11 language features. The build
uses C++17 because the bundled `third/crow_all.h` (modern Crow) relies on
`std::string_view` and C++17 transparent comparators internally.

## Features

- HTTP Digest authentication on all `/api/*` routes (Crow middleware)
  - Realm: `SignalController`
  - Default credentials: `admin` / `admin123`
  - qop: `auth`, MD5
- `GET /api/Auth/Check` (protected) to verify credentials
- `GET/POST /api/Config/DeviceName` (protected) to read/update the device name
- Static files served from `../web/dist/` (the frontend build output) at `/static/<path>`,
  with the Vue app's `index.html` served at `/`
- Device name persisted in `data/device_name.txt` (auto-created on first write)
- Default device name: `Traffic PC`

## Build

```bash
cd app
mkdir -p build && cd build
cmake ..
make -j
```

## Run

```bash
cd app
./build/server
```

Then open <http://localhost:8080> in your browser (the page is served from
`../web/dist/index.html`).

## API

### `GET /api/Config/DeviceName`

```json
{ "deviceName": "Traffic PC" }
```

### `POST /api/Config/DeviceName`

Request body:
```json
{ "deviceName": "New Name" }
```

Response (200):
```json
{ "deviceName": "New Name" }
```

Error response (400) on malformed JSON or missing `deviceName`:
```json
{ "error": "..." }
```

## Layout

```
app/
  md5.hpp           # standalone MD5 (used by DigestAuth)
  app.hpp           # RestApp + DigestAuth + DigestAuthMiddleware
  main.cpp          # constructs RestApp and calls run()
  CMakeLists.txt    # build script
  data/             # created at runtime
    device_name.txt # persistent device name
  build/            # cmake build directory
```

## Auth

All `/api/*` routes are protected by `DigestAuthMiddleware` (a Crow middleware). On
unauthenticated access the server returns a **401 with the challenge in the JSON body**
(not in the `WWW-Authenticate` header):

```
HTTP/1.1 401 Unauthorized
Content-Type: application/json

{
  "error": "unauthorized",
  "challenge": "Digest realm=\"SignalController\", nonce=\"...\", qop=\"auth\""
}
```

The client must read `response.data.challenge`, compute the digest response, and retry
with an `Authorization: Digest ...` header. See `web/src/api/auth.js` for the working
JS implementation.

**Why not `WWW-Authenticate` header?** Browsers show a native auth dialog and cache
credentials by realm when they see that header. For a SPA we want full control of the
login UX, so the challenge is delivered in the body instead. The server still validates
the full digest math (HA1/HA2/response) — only the transport for the challenge changes.

The bundled Crow does not ship built-in digest auth; the implementation here is adapted
from the `web-conf-scaffold/references/digest_auth.h` reference.

**Changing the default credentials**: edit `app/app.hpp`:
```cpp
DigestAuthMiddleware()
    : auth_("SignalController", "admin", "admin123") {}
```
