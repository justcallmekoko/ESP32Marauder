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
#include "Arduino.h"
#include "AsyncEventSource.h"
#ifndef ESP8266
  #include <rom/ets_sys.h>
#endif

static String generateEventMessage(const char *message, const char *event, uint32_t id, uint32_t reconnect){
  String ev;

  if(reconnect){
    ev += F("retry: ");
    ev += reconnect;
    ev += F("\r\n");
  }

  if(id){
    ev += F("id: ");
    ev += String(id);
    ev += F("\r\n");
  }

  if(event != NULL){
    ev += F("event: ");
    ev += String(event);
    ev += F("\r\n");
  }

  if(message != NULL){
    size_t messageLen = strlen(message);
    char * lineStart = (char *)message;
    char * lineEnd;
    do {
      char * nextN = strchr(lineStart, '\n');
      char * nextR = strchr(lineStart, '\r');
      if(nextN == NULL && nextR == NULL){
        size_t llen = ((char *)message + messageLen) - lineStart;
        char * ldata = (char *)malloc(llen+1);
        if(ldata != NULL){
          memcpy(ldata, lineStart, llen);
          ldata[llen] = 0;
          ev += F("data: ");
          ev += ldata;
          ev += F("\r\n\r\n");
          free(ldata);
        }
        lineStart = (char *)message + messageLen;
      } else {
        char * nextLine = NULL;
        if(nextN != NULL && nextR != NULL){
          if(nextR < nextN){
            lineEnd = nextR;
            if(nextN == (nextR + 1))
              nextLine = nextN + 1;
            else
              nextLine = nextR + 1;
          } else {
            lineEnd = nextN;
            if(nextR == (nextN + 1))
              nextLine = nextR + 1;
            else
              nextLine = nextN + 1;
          }
        } else if(nextN != NULL){
          lineEnd = nextN;
          nextLine = nextN + 1;
        } else {
          lineEnd = nextR;
          nextLine = nextR + 1;
        }

        size_t llen = lineEnd - lineStart;
        char * ldata = (char *)malloc(llen+1);
        if(ldata != NULL){
          memcpy(ldata, lineStart, llen);
          ldata[llen] = 0;
          ev += F("data: ");
          ev += ldata;
          ev += F("\r\n");
          free(ldata);
        }
        lineStart = nextLine;
        if(lineStart == ((char *)message + messageLen))
          ev += F("\r\n");
      }
    } while(lineStart < ((char *)message + messageLen));
  }

  return ev;
}

// Message

AsyncEventSourceMessage::AsyncEventSourceMessage(const char * data, size_t len)
: _data(nullptr), _len(len), _sent(0), _acked(0)
{
  _data = (uint8_t*)malloc(_len+1);
  if(_data == nullptr){
    _len = 0;
  } else {
    memcpy(_data, data, len);
    _data[_len] = 0;
  }
}

AsyncEventSourceMessage::~AsyncEventSourceMessage() {
     if(_data != NULL)
        free(_data);
}

size_t AsyncEventSourceMessage::ack(size_t len, uint32_t time) {
  (void)time;
  // If the whole message is now acked...
  if(_acked + len > _len){
     // Return the number of extra bytes acked (they will be carried on to the next message)
     const size_t extra = _acked + len - _len;
     _acked = _len;
     return extra;
  }
  // Return that no extra bytes left.
  _acked += len;
  return 0;
}

// This could also return void as the return value is not used.
// Leaving as-is for compatibility...
size_t AsyncEventSourceMessage::send(AsyncClient *client) {
    if (_sent >= _len) {
      return 0;
    }
    const size_t len_to_send = _len - _sent;
    auto position = reinterpret_cast<const char*>(_data + _sent);
    const size_t sent_now = client->write(position, len_to_send);
    _sent += sent_now;
    return sent_now;
}

// Client

AsyncEventSourceClient::AsyncEventSourceClient(AsyncWebServerRequest *request, AsyncEventSource *server)
: _messageQueue(AlternativeLinkedList<AsyncEventSourceMessage *>([](AsyncEventSourceMessage *m){ delete  m; }))
{
  _client = request->client();
  _server = server;
  _lastId = 0;
  if(request->hasHeader(F("Last-Event-ID")))
    _lastId = atoi(request->getHeader(F("Last-Event-ID"))->value().c_str());

  _client->setRxTimeout(0);
  _client->onError(NULL, NULL);
  _client->onAck([](void *r, AsyncClient* c, size_t len, uint32_t time){ (void)c; ((AsyncEventSourceClient*)(r))->_onAck(len, time); }, this);
  _client->onPoll([](void *r, AsyncClient* c){ (void)c; ((AsyncEventSourceClient*)(r))->_onPoll(); }, this);
  _client->onData(NULL, NULL);
  _client->onTimeout([this](void *r, AsyncClient* c __attribute__((unused)), uint32_t time){ ((AsyncEventSourceClient*)(r))->_onTimeout(time); }, this);
  _client->onDisconnect([this](void *r, AsyncClient* c){ ((AsyncEventSourceClient*)(r))->_onDisconnect(); delete c; }, this);

  _server->_addClient(this);
  delete request;
}

AsyncEventSourceClient::~AsyncEventSourceClient(){
  _lockmq.lock();
  _messageQueue.free();
  _lockmq.unlock();
  close();
}

void AsyncEventSourceClient::_queueMessage(AsyncEventSourceMessage *dataMessage){
  if(dataMessage == NULL)
    return;
  if(!connected()){
    delete dataMessage;
    return;
  }
  //length() is not thread-safe, thus acquiring the lock before this call..
  _lockmq.lock();
  if(_messageQueue.length() >= SSE_MAX_QUEUED_MESSAGES){
#ifdef ESP8266
    ets_printf(String(F("ERROR: Too many messages queued\n")).c_str());
#else
    log_e("Too many messages queued: deleting message");
#endif
      delete dataMessage;
  } else {
    _messageQueue.add(dataMessage);
    // runqueue trigger when new messages added
    if(_client->canSend()) {
      _runQueue();
    }
  }
  _lockmq.unlock();
}

void AsyncEventSourceClient::_onAck(size_t len, uint32_t time){
  // Same here, acquiring the lock early
  _lockmq.lock();
  while(len && !_messageQueue.isEmpty()){
    len = _messageQueue.front()->ack(len, time);
    if(_messageQueue.front()->finished())
      _messageQueue.remove(_messageQueue.front());
  }
  _runQueue();
  _lockmq.unlock();
}

void AsyncEventSourceClient::_onPoll(){
  _lockmq.lock();
  if(!_messageQueue.isEmpty()){
    _runQueue();
  }
  _lockmq.unlock();
}

void AsyncEventSourceClient::_onTimeout(uint32_t time __attribute__((unused))){
  _client->close(true);
}

void AsyncEventSourceClient::_onDisconnect(){
  _client = NULL;
  _server->_handleDisconnect(this);
}

void AsyncEventSourceClient::close(){
  if(_client != NULL)
    _client->close();
}

void AsyncEventSourceClient::write(const char * message, size_t len){
  _queueMessage(new AsyncEventSourceMessage(message, len));
}

void AsyncEventSourceClient::send(const char *message, const char *event, uint32_t id, uint32_t reconnect){
  String ev = generateEventMessage(message, event, id, reconnect);
  _queueMessage(new AsyncEventSourceMessage(ev.c_str(), ev.length()));
}

size_t AsyncEventSourceClient::packetsWaiting() const {
    size_t len;
    _lockmq.lock();
    len = _messageQueue.length();
    _lockmq.unlock();
    return len;
}

void AsyncEventSourceClient::_runQueue() {
  // Calls to this private method now already protected by _lockmq acquisition
  // so no extra call of _lockmq.lock() here..
  for (auto i = _messageQueue.begin(); i != _messageQueue.end(); ++i) {
    // If it crashes here, iterator (i) has been invalidated as _messageQueue
    // has been changed... (UL 2020-11-15: Not supposed to happen any more ;-) )
    if (!(*i)->sent()) {
      (*i)->send(_client);
    }
  }
}


// Handler

AsyncEventSource::AsyncEventSource(const String& url)
  : _url(url)
  , _clients(AlternativeLinkedList<AsyncEventSourceClient *>([](AsyncEventSourceClient *c){ delete c; }))
  , _connectcb(NULL)
{}

AsyncEventSource::~AsyncEventSource(){
  close();
}

void AsyncEventSource::onConnect(ArEventHandlerFunction cb){
  _connectcb = cb;
}

void AsyncEventSource::authorizeConnect(ArAuthorizeConnectHandler cb){
  _authorizeConnectHandler = cb;
}

void AsyncEventSource::_addClient(AsyncEventSourceClient * client){
  /*char * temp = (char *)malloc(2054);
  if(temp != NULL){
    memset(temp+1,' ',2048);
    temp[0] = ':';
    temp[2049] = '\r';
    temp[2050] = '\n';
    temp[2051] = '\r';
    temp[2052] = '\n';
    temp[2053] = 0;
    client->write((const char *)temp, 2053);
    free(temp);
  }*/
  AsyncWebLockGuard l(_client_queue_lock);
  _clients.add(client);
  if(_connectcb)
    _connectcb(client);
}

void AsyncEventSource::_handleDisconnect(AsyncEventSourceClient * client){
  AsyncWebLockGuard l(_client_queue_lock);
  _clients.remove(client);
}

void AsyncEventSource::close(){
  // While the whole loop is not done, the linked list is locked and so the
  // iterator should remain valid even when AsyncEventSource::_handleDisconnect()
  // is called very early
  AsyncWebLockGuard l(_client_queue_lock);
  for(const auto &c: _clients){
    if(c->connected())
      c->close();
  }
}

// pmb fix
size_t AsyncEventSource::avgPacketsWaiting() const {
  size_t aql = 0;
  uint32_t nConnectedClients = 0;
  AsyncWebLockGuard l(_client_queue_lock);
  if (_clients.isEmpty()) {
    return 0;
  }
  for(const auto &c: _clients){
    if(c->connected()) {
      aql += c->packetsWaiting();
      ++nConnectedClients;
    }
  }
  return ((aql) + (nConnectedClients/2)) / (nConnectedClients); // round up
}

void AsyncEventSource::send(
    const char *message, const char *event, uint32_t id, uint32_t reconnect){
  String ev = generateEventMessage(message, event, id, reconnect);
  AsyncWebLockGuard l(_client_queue_lock);
  for(const auto &c: _clients){
    if(c->connected()) {
      c->write(ev.c_str(), ev.length());
    }
  }
}

size_t AsyncEventSource::count() const {
  size_t n_clients;
  AsyncWebLockGuard l(_client_queue_lock);
  n_clients = _clients.count_if([](AsyncEventSourceClient *c){
                                    return c->connected();
                                });
  return n_clients;
}

bool AsyncEventSource::canHandle(AsyncWebServerRequest *request){
  if(request->method() != HTTP_GET || !request->url().equals(_url)) {
    return false;
  }
  request->addInterestingHeader(F("Last-Event-ID"));
  request->addInterestingHeader("Cookie");
  return true;
}

void AsyncEventSource::handleRequest(AsyncWebServerRequest *request){
  if((_username.length() && _password.length()) && !request->authenticate(_username.c_str(), _password.c_str())) {
    return request->requestAuthentication();
  }
  if(_authorizeConnectHandler != NULL){
    if(!_authorizeConnectHandler(request)){
      return request->send(401);
    }
  }
  request->send(new AsyncEventSourceResponse(this));
}

// Response

AsyncEventSourceResponse::AsyncEventSourceResponse(AsyncEventSource *server){
  _server = server;
  _code = 200;
  _contentType = F("text/event-stream");
  _sendContentLength = false;
  addHeader(F("Cache-Control"), F("no-cache"));
  addHeader(F("Connection"), F("keep-alive"));
}

void AsyncEventSourceResponse::_respond(AsyncWebServerRequest *request){
  String out = _assembleHead(request->version());
  request->client()->write(out.c_str(), _headLength);
  _state = RESPONSE_WAIT_ACK;
}

size_t AsyncEventSourceResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time __attribute__((unused))){
  if(len){
    new AsyncEventSourceClient(request, _server);
  }
  return 0;
}

