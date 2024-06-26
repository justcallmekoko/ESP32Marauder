# ESP Async WebServer

[![License: LGPL 3.0](https://img.shields.io/badge/License-LGPL%203.0-yellow.svg)](https://opensource.org/license/lgpl-3-0/)
[![Continuous Integration](https://github.com/mathieucarbou/ESPAsyncWebServer/actions/workflows/ci.yml/badge.svg)](https://github.com/mathieucarbou/ESPAsyncWebServer/actions/workflows/ci.yml)
[![PlatformIO Registry](https://badges.registry.platformio.org/packages/mathieucarbou/library/ESP%20Async%20WebServer.svg)](https://registry.platformio.org/libraries/mathieucarbou/ESP%20Async%20WebServer)

Asynchronous HTTP and WebSocket Server Library for ESP32.
Supports: WebSocket, SSE, Authentication, Arduino Json 7, File Upload, Static File serving, URL Rewrite, URL Redirect, etc.

This fork is based on [yubox-node-org/ESPAsyncWebServer](https://github.com/yubox-node-org/ESPAsyncWebServer) and includes all the concurrency fixes.

## Changes in this fork

- Removed SPIFFSEditor
- Deployed in PlatformIO registry and Arduino IDE library manager
- CI
- Some code cleanup
- `SSE_MAX_QUEUED_MESSAGES` to control the maximum number of messages that can be queued for a SSE client
- `write()` function public in `AsyncEventSource.h`
- Arduino Json 7 compatibility and backward compatible with 6 and 6 (changes in `AsyncJson.h`). The API to use Json has not changed. These are only internal changes.
- `WS_MAX_QUEUED_MESSAGES`: control the maximum number of messages that can be queued for a Websocket client
- Resurrected `AsyncWebSocketMessageBuffer` and `makeBuffer()` in order to make the fork API-compatible with the original library from me-no-dev regarding WebSocket.
- [#5](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/5) ([@vortigont](https://github.com/vortigont)): set real "Last-Modified" header based on file's LastWrite time
- [#13](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/13) ([@tueddy](https://github.com/tueddy)): Compile with Arduino 3 (ESP-IDF 5.1)
- [#14](https://github.com/mathieucarbou/ESPAsyncWebServer/pull/14) ([@nilo85](https://github.com/nilo85)): Add support for Auth & GET requests in AsyncCallbackJsonWebHandler
- Added `setAuthentication(const String& username, const String& password)`
- Added `StreamConcat` example to shoiw how to stream multiple files in one response
- Remove filename after inline in Content-Disposition header according to RFC2183
- Depends on `mathieucarbou/Async TCP @ ^3.1.4`
- Arduino 3 / ESP-IDF 5.1 compatibility
- Added all flavors of `binary()`, `text()`, `binaryAll()` and `textAll()` in `AsyncWebSocket`
- Added `setCloseClientOnQueueFull(bool)` which can be set on a client to either close the connection or discard messages but not close the connection when the queue is full

## Documentation

Usage and API stays the same as the original library.
Please look at the original libraries for more examples and documentation.

[https://github.com/yubox-node-org/ESPAsyncWebServer](https://github.com/yubox-node-org/ESPAsyncWebServer)

## `AsyncWebSocketMessageBuffer` and `makeBuffer()`

The fork from `yubox-node-org` introduces some breaking API changes compared to the original library, especially regarding the use of `std::shared_ptr<std::vector<uint8_t>>` for WebSocket.

This fork is compatible with the original library from `me-no-dev` regarding WebSocket, and wraps the optimizations done by `yubox-node-org` in the `AsyncWebSocketMessageBuffer` class.
So you have the choice of which API to use.
I strongly suggest to use the optimized API from `yubox-node-org` as it is much more efficient.

Here is an example for serializing a Json document in a websocket message buffer. This code is compatible with any forks, but not optimized:

```cpp
void send(JsonDocument& doc) {
  const size_t len = measureJson(doc);

  // original API from me-no-dev
  AsyncWebSocketMessageBuffer* buffer = _ws->makeBuffer(len);
  assert(buffer); // up to you to keep or remove this
  serializeJson(doc, buffer->get(), len);
  _ws->textAll(buffer);
}
```

Here is an example for serializing a Json document in a more optimized way, and compatible with both forks:

```cpp
void send(JsonDocument& doc) {
  const size_t len = measureJson(doc);

#if defined(ASYNCWEBSERVER_FORK_mathieucarbou)

  // this fork (originally from yubox-node-org), uses another API with shared pointer that better support concurrent use cases then the original project
  auto buffer = std::make_shared<std::vector<uint8_t>>(len);
  assert(buffer); // up to you to keep or remove this
  serializeJson(doc, buffer->data(), len);
  _ws->textAll(std::move(buffer));

#else

  // original API from me-no-dev
  AsyncWebSocketMessageBuffer* buffer = _ws->makeBuffer(len);
  assert(buffer); // up to you to keep or remove this
  serializeJson(doc, buffer->get(), len);
  _ws->textAll(buffer);

#endif
}
```
