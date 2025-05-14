/*
  Asynchronous WebServer library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#ifndef ASYNCEVENTSOURCE_H_
#define ASYNCEVENTSOURCE_H_

#include <Arduino.h>
#ifdef ESP32
#include <AsyncTCP.h>
#ifndef SSE_MAX_QUEUED_MESSAGES
#define SSE_MAX_QUEUED_MESSAGES 32
#endif
#else
#include <ESPAsyncTCP.h>
#ifndef SSE_MAX_QUEUED_MESSAGES
#define SSE_MAX_QUEUED_MESSAGES 8
#endif
#endif

#include <ESPAsyncWebServer.h>

#include "AsyncWebSynchronization.h"

#ifdef ESP8266
#include <Hash.h>
#ifdef CRYPTO_HASH_h // include Hash.h from espressif framework if the first include was from the crypto library
#include <../src/Hash.h>
#endif
#endif

#ifdef ESP32
#define DEFAULT_MAX_SSE_CLIENTS 8
#else
#define DEFAULT_MAX_SSE_CLIENTS 4
#endif

class AsyncEventSource;
class AsyncEventSourceResponse;
class AsyncEventSourceClient;
typedef std::function<void(AsyncEventSourceClient *client)> ArEventHandlerFunction;
typedef std::function<bool(AsyncWebServerRequest *request)> ArAuthorizeConnectHandler;

class AsyncEventSourceMessage {
  private:
    uint8_t * _data;
    size_t _len;
    size_t _sent;
    //size_t _ack;
    size_t _acked;
  public:
    AsyncEventSourceMessage(const char * data, size_t len);
    ~AsyncEventSourceMessage();
    size_t ack(size_t len, uint32_t time __attribute__((unused)));
    size_t send(AsyncClient *client);
    bool finished(){ return _acked == _len; }
    bool sent() { return _sent == _len; }
};

class AsyncEventSourceClient {
  private:
    AsyncClient *_client;
    AsyncEventSource *_server;
    uint32_t _lastId;
    AlternativeLinkedList<AsyncEventSourceMessage *> _messageQueue;
    // ArFi 2020-08-27 for protecting/serializing _messageQueue
    AsyncPlainLock _lockmq;
    void _queueMessage(AsyncEventSourceMessage *dataMessage);
    void _runQueue();

  public:

    AsyncEventSourceClient(AsyncWebServerRequest *request, AsyncEventSource *server);
    ~AsyncEventSourceClient();

    AsyncClient* client(){ return _client; }
    void close();
    void write(const char * message, size_t len);
    void send(const char *message, const char *event=NULL, uint32_t id=0, uint32_t reconnect=0);
    bool connected() const { return (_client != NULL) && _client->connected(); }
    uint32_t lastId() const { return _lastId; }
    size_t  packetsWaiting() const;

    //system callbacks (do not call)
    void _onAck(size_t len, uint32_t time);
    void _onPoll();
    void _onTimeout(uint32_t time);
    void _onDisconnect();
};

class AsyncEventSource: public AsyncWebHandler {
  private:
    String _url;
    AlternativeLinkedList<AsyncEventSourceClient *> _clients;
    // Same as for individual messages, protect mutations of _clients list
    // since simultaneous access from different tasks is possible
    AsyncWebLock _client_queue_lock;
    ArEventHandlerFunction _connectcb;
	  ArAuthorizeConnectHandler _authorizeConnectHandler;
  public:
    AsyncEventSource(const String& url);
    ~AsyncEventSource();

    const char * url() const { return _url.c_str(); }
    void close();
    void onConnect(ArEventHandlerFunction cb);
    void authorizeConnect(ArAuthorizeConnectHandler cb);
    void send(const char *message, const char *event=NULL, uint32_t id=0, uint32_t reconnect=0);
    // number of clients connected
    size_t count() const;
    size_t  avgPacketsWaiting() const;

    //system callbacks (do not call)
    void _addClient(AsyncEventSourceClient * client);
    void _handleDisconnect(AsyncEventSourceClient * client);
    virtual bool canHandle(AsyncWebServerRequest *request) override final;
    virtual void handleRequest(AsyncWebServerRequest *request) override final;
};

class AsyncEventSourceResponse: public AsyncWebServerResponse {
  private:
    String _content;
    AsyncEventSource *_server;
  public:
    AsyncEventSourceResponse(AsyncEventSource *server);
    void _respond(AsyncWebServerRequest *request);
    size_t _ack(AsyncWebServerRequest *request, size_t len, uint32_t time);
    bool _sourceValid() const { return true; }
};


#endif /* ASYNCEVENTSOURCE_H_ */
