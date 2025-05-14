/*
  Asynchronous WebServer library for Espressif MCUs

  Copyright (c) 2016 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

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
#include "AsyncWebSocket.h"

#include <cstring>

#include <libb64/cencode.h>

#ifndef ESP8266
#include "mbedtls/sha1.h"
#include <rom/ets_sys.h>
#else
#include <Hash.h>
#endif

#define MAX_PRINTF_LEN 64

size_t webSocketSendFrameWindow(AsyncClient *client){
  if(!client->canSend())
    return 0;
  size_t space = client->space();
  if(space < 9)
    return 0;
  return space - 8;
}

size_t webSocketSendFrame(AsyncClient *client, bool final, uint8_t opcode, bool mask, uint8_t *data, size_t len){
  if(!client->canSend()) {
    // Serial.println("SF 1");
    return 0;
  }
  size_t space = client->space();
  if(space < 2) {
    // Serial.println("SF 2");
    return 0;
  }
  uint8_t mbuf[4] = {0,0,0,0};
  uint8_t headLen = 2;
  if(len && mask){
    headLen += 4;
    mbuf[0] = rand() % 0xFF;
    mbuf[1] = rand() % 0xFF;
    mbuf[2] = rand() % 0xFF;
    mbuf[3] = rand() % 0xFF;
  }
  if(len > 125)
    headLen += 2;
  if(space < headLen) {
    // Serial.println("SF 2");
    return 0;
  }
  space -= headLen;

  if(len > space) len = space;

  uint8_t *buf = (uint8_t*)malloc(headLen);
  if(buf == NULL){
    //os_printf("could not malloc %u bytes for frame header\n", headLen);
    // Serial.println("SF 3");
    return 0;
  }

  buf[0] = opcode & 0x0F;
  if(final)
    buf[0] |= 0x80;
  if(len < 126)
    buf[1] = len & 0x7F;
  else {
    buf[1] = 126;
    buf[2] = (uint8_t)((len >> 8) & 0xFF);
    buf[3] = (uint8_t)(len & 0xFF);
  }
  if(len && mask){
    buf[1] |= 0x80;
    memcpy(buf + (headLen - 4), mbuf, 4);
  }
  if(client->add((const char *)buf, headLen) != headLen){
    //os_printf("error adding %lu header bytes\n", headLen);
    free(buf);
    // Serial.println("SF 4");
    return 0;
  }
  free(buf);

  if(len){
    if(len && mask){
      size_t i;
      for(i=0;i<len;i++)
        data[i] = data[i] ^ mbuf[i%4];
    }
    if(client->add((const char *)data, len) != len){
      //os_printf("error adding %lu data bytes\n", len);
      // Serial.println("SF 5");
      return 0;
    }
  }
  if(!client->send()){
    //os_printf("error sending frame: %lu\n", headLen+len);
    // Serial.println("SF 6");
    return 0;
  }
  // Serial.println("SF");
  return len;
}



/*
 *    AsyncWebSocketMessageBuffer
 */

AsyncWebSocketMessageBuffer::AsyncWebSocketMessageBuffer()
    : _buffer(std::make_shared<std::vector<uint8_t>>(0)) 
{
}

AsyncWebSocketMessageBuffer::AsyncWebSocketMessageBuffer(uint8_t* data, size_t size)
    : _buffer(std::make_shared<std::vector<uint8_t>>(size)) 
{
    if (_buffer->capacity() < size) {
        _buffer.reset();
        _buffer = std::make_shared<std::vector<uint8_t>>(0);
    } else {
        std::memcpy(_buffer->data(), data, size);
    }
}

AsyncWebSocketMessageBuffer::AsyncWebSocketMessageBuffer(size_t size)
    : _buffer(std::make_shared<std::vector<uint8_t>>(size)) 
{
    if (_buffer->capacity() < size) {
        _buffer.reset();
        _buffer = std::make_shared<std::vector<uint8_t>>(0);
    }
}

AsyncWebSocketMessageBuffer::~AsyncWebSocketMessageBuffer() 
{
    _buffer.reset();
}

bool AsyncWebSocketMessageBuffer::reserve(size_t size) 
{
    if (_buffer->capacity() >= size)
        return true;
    _buffer->reserve(size);
    return _buffer->capacity() >= size;
}

/*
 * Control Frame
 */

class AsyncWebSocketControl {
private:
    uint8_t _opcode;
    uint8_t *_data;
    size_t _len;
    bool _mask;
    bool _finished;

public:
    AsyncWebSocketControl(uint8_t opcode, const uint8_t *data=NULL, size_t len=0, bool mask=false)
      :_opcode(opcode)
      ,_len(len)
      ,_mask(len && mask)
      ,_finished(false)
    {
        if (data == NULL)
            _len = 0;
        if (_len)
        {
            if (_len > 125)
                _len = 125;

            _data = (uint8_t*)malloc(_len);

            if(_data == NULL)
                _len = 0;
            else
                memcpy(_data, data, len);
        }
        else
            _data = NULL;
    }

    virtual ~AsyncWebSocketControl()
    {
        if (_data != NULL)
            free(_data);
    }

    virtual bool finished() const { return _finished; }
    uint8_t opcode(){ return _opcode; }
    uint8_t len(){ return _len + 2; }
    size_t send(AsyncClient *client){
        _finished = true;
        return webSocketSendFrame(client, true, _opcode & 0x0F, _mask, _data, _len);
    }
};


/*
 * AsyncWebSocketMessage Message
 */


AsyncWebSocketMessage::AsyncWebSocketMessage(std::shared_ptr<std::vector<uint8_t>> buffer, uint8_t opcode, bool mask) :
    _WSbuffer{buffer},
    _opcode(opcode & 0x07),
    _mask{mask},
    _status{_WSbuffer?WS_MSG_SENDING:WS_MSG_ERROR}
{
} 

void AsyncWebSocketMessage::ack(size_t len, uint32_t time)
{
    (void)time;
    _acked += len;
    if (_sent >= _WSbuffer->size() && _acked >= _ack)
    {
        _status = WS_MSG_SENT;
    }
    //ets_printf("A: %u\n", len);
}

size_t AsyncWebSocketMessage::send(AsyncClient *client)
{
    if (_status != WS_MSG_SENDING)
        return 0;
    if (_acked < _ack){
        return 0;
    }
    if (_sent == _WSbuffer->size())
    {
        if(_acked == _ack)
            _status = WS_MSG_SENT;
        return 0;
    }
    if (_sent > _WSbuffer->size())
    {
        _status = WS_MSG_ERROR;
        //ets_printf("E: %u > %u\n", _sent, _WSbuffer->length());
        return 0;
    }

    size_t toSend = _WSbuffer->size() - _sent;
    size_t window = webSocketSendFrameWindow(client);

    if (window < toSend) {
        toSend = window;
    }

    _sent += toSend;
    _ack += toSend + ((toSend < 126)?2:4) + (_mask * 4);

    //ets_printf("W: %u %u\n", _sent - toSend, toSend);

    bool final = (_sent == _WSbuffer->size());
    uint8_t* dPtr = (uint8_t*)(_WSbuffer->data() + (_sent - toSend));
    uint8_t opCode = (toSend && _sent == toSend)?_opcode:(uint8_t)WS_CONTINUATION;

    size_t sent = webSocketSendFrame(client, final, opCode, _mask, dPtr, toSend);
    _status = WS_MSG_SENDING;
    if (toSend && sent != toSend){
        //ets_printf("E: %u != %u\n", toSend, sent);
        _sent -= (toSend - sent);
        _ack -= (toSend - sent);
    }
    //ets_printf("S: %u %u\n", _sent, sent);
    return sent;
}


/*
 * Async WebSocket Client
 */
 const char * AWSC_PING_PAYLOAD = "ESPAsyncWebServer-PING";
 const size_t AWSC_PING_PAYLOAD_LEN = 22;

AsyncWebSocketClient::AsyncWebSocketClient(AsyncWebServerRequest *request, AsyncWebSocket *server)
  : _tempObject(NULL)
{
    _client = request->client();
    _server = server;
    _clientId = _server->_getNextId();
    _status = WS_CONNECTED;
    _pstate = 0;
    _lastMessageTime = millis();
    _keepAlivePeriod = 0;
    _client->setRxTimeout(0);
    _client->onError([](void *r, AsyncClient* c, int8_t error){ (void)c; ((AsyncWebSocketClient*)(r))->_onError(error); }, this);
    _client->onAck([](void *r, AsyncClient* c, size_t len, uint32_t time){ (void)c; ((AsyncWebSocketClient*)(r))->_onAck(len, time); }, this);
    _client->onDisconnect([](void *r, AsyncClient* c){ ((AsyncWebSocketClient*)(r))->_onDisconnect(); delete c; }, this);
    _client->onTimeout([](void *r, AsyncClient* c, uint32_t time){ (void)c; ((AsyncWebSocketClient*)(r))->_onTimeout(time); }, this);
    _client->onData([](void *r, AsyncClient* c, void *buf, size_t len){ (void)c; ((AsyncWebSocketClient*)(r))->_onData(buf, len); }, this);
    _client->onPoll([](void *r, AsyncClient* c){ (void)c; ((AsyncWebSocketClient*)(r))->_onPoll(); }, this);
    _server->_handleEvent(this, WS_EVT_CONNECT, request, NULL, 0);
    delete request;
    memset(&_pinfo,0,sizeof(_pinfo));
}

AsyncWebSocketClient::~AsyncWebSocketClient()
{
    {
        AsyncWebLockGuard l(_lock);

        _messageQueue.clear();
        _controlQueue.clear();
    }
    _server->_handleEvent(this, WS_EVT_DISCONNECT, NULL, NULL, 0);
}

void AsyncWebSocketClient::_clearQueue()
{
    while (!_messageQueue.empty() && _messageQueue.front().finished())
      _messageQueue.pop_front();
}

void AsyncWebSocketClient::_onAck(size_t len, uint32_t time){
    _lastMessageTime = millis();

    AsyncWebLockGuard l(_lock);

    if (!_controlQueue.empty()) {
        auto &head = _controlQueue.front();
        if (head.finished()){
            len -= head.len();
            if (_status == WS_DISCONNECTING && head.opcode() == WS_DISCONNECT){
                _controlQueue.pop_front();
                _status = WS_DISCONNECTED;
                l.unlock();
                if (_client) _client->close(true);
                return;
            }
            _controlQueue.pop_front();
        }
    }

    if(len && !_messageQueue.empty()){
        _messageQueue.front().ack(len, time);
    }

    _clearQueue();

    _runQueue();
}

void AsyncWebSocketClient::_onPoll()
{
    if (!_client)
        return;

    AsyncWebLockGuard l(_lock);
    if (_client->canSend() && (!_controlQueue.empty() || !_messageQueue.empty()))
    {
        l.unlock();
        _runQueue();
    }
    else if (_keepAlivePeriod > 0 && (millis() - _lastMessageTime) >= _keepAlivePeriod && (_controlQueue.empty() && _messageQueue.empty()))
    {
        l.unlock();
        ping((uint8_t *)AWSC_PING_PAYLOAD, AWSC_PING_PAYLOAD_LEN);
    }
}

void AsyncWebSocketClient::_runQueue()
{
    if (!_client)
        return;

    AsyncWebLockGuard l(_lock);

    _clearQueue();

    if (!_controlQueue.empty() && (_messageQueue.empty() || _messageQueue.front().betweenFrames()) && webSocketSendFrameWindow(_client) > (size_t)(_controlQueue.front().len() - 1))
    {
        //l.unlock();
        _controlQueue.front().send(_client);
    }
    else if (!_messageQueue.empty() && _messageQueue.front().betweenFrames() && webSocketSendFrameWindow(_client))
    {
        //l.unlock();
        _messageQueue.front().send(_client);
    }
}

bool AsyncWebSocketClient::queueIsFull() const
{
    size_t size;
    {
        AsyncWebLockGuard l(_lock);
        size = _messageQueue.size();
    }
    return (size >= WS_MAX_QUEUED_MESSAGES) || (_status != WS_CONNECTED);
}

size_t AsyncWebSocketClient::queueLen() const
{
    AsyncWebLockGuard l(_lock);

    return _messageQueue.size() + _controlQueue.size();
}

bool AsyncWebSocketClient::canSend() const
{
    size_t size;
    {
        AsyncWebLockGuard l(_lock);
        size = _messageQueue.size();
    }
    return size < WS_MAX_QUEUED_MESSAGES;
}

void AsyncWebSocketClient::_queueControl(uint8_t opcode, const uint8_t *data, size_t len, bool mask)
{
    if (!_client)
        return;

    {
        AsyncWebLockGuard l(_lock);
        _controlQueue.emplace_back(opcode, data, len, mask);
    }

    if (_client && _client->canSend())
        _runQueue();
}

void AsyncWebSocketClient::_queueMessage(std::shared_ptr<std::vector<uint8_t>> buffer, uint8_t opcode, bool mask)
{
    if(_status != WS_CONNECTED)
        return;

    if (!_client)
        return;

    if (buffer->size() == 0)
        return;

    {
        AsyncWebLockGuard l(_lock);
        if (_messageQueue.size() >= WS_MAX_QUEUED_MESSAGES)
        {
            l.unlock();
            if(closeWhenFull)
            {
#ifdef ESP8266
                ets_printf("AsyncWebSocketClient::_queueMessage: Too many messages queued: closing connection\n");
#else
                log_e("Too many messages queued: closing connection");
#endif
                _status = WS_DISCONNECTED;
                if (_client) _client->close(true);
            } else {
#ifdef ESP8266
                ets_printf("AsyncWebSocketClient::_queueMessage: Too many messages queued: discarding new message\n");
#else
                log_e("Too many messages queued: discarding new message");
#endif
            }
            return;
        }
        else
        {
            _messageQueue.emplace_back(buffer, opcode, mask);
        }
    }

    if (_client && _client->canSend())
        _runQueue();
}

void AsyncWebSocketClient::close(uint16_t code, const char * message)
{
    if(_status != WS_CONNECTED)
        return;

    if(code)
    {
        uint8_t packetLen = 2;
        if (message != NULL)
        {
            size_t mlen = strlen(message);
            if(mlen > 123) mlen = 123;
            packetLen += mlen;
        }
        char * buf = (char*)malloc(packetLen);
        if (buf != NULL)
        {
            buf[0] = (uint8_t)(code >> 8);
            buf[1] = (uint8_t)(code & 0xFF);
            if(message != NULL){
                memcpy(buf+2, message, packetLen -2);
            }
            _queueControl(WS_DISCONNECT, (uint8_t*)buf, packetLen);
            free(buf);
            return;
        }
    }
    _queueControl(WS_DISCONNECT);
}

void AsyncWebSocketClient::ping(const uint8_t *data, size_t len)
{
    if (_status == WS_CONNECTED)
        _queueControl(WS_PING, data, len);
}

void AsyncWebSocketClient::_onError(int8_t)
{
    //Serial.println("onErr");
}

void AsyncWebSocketClient::_onTimeout(uint32_t time)
{
    // Serial.println("onTime");
    (void)time;
    _client->close(true);
}

void AsyncWebSocketClient::_onDisconnect()
{
    // Serial.println("onDis");
    _client = NULL;
}

void AsyncWebSocketClient::_onData(void *pbuf, size_t plen)
{
  // Serial.println("onData");
  _lastMessageTime = millis();
  uint8_t *data = (uint8_t*)pbuf;
  while(plen > 0){
    if(!_pstate){
      const uint8_t *fdata = data;
      _pinfo.index = 0;
      _pinfo.final = (fdata[0] & 0x80) != 0;
      _pinfo.opcode = fdata[0] & 0x0F;
      _pinfo.masked = (fdata[1] & 0x80) != 0;
      _pinfo.len = fdata[1] & 0x7F;
      data += 2;
      plen -= 2;
      if(_pinfo.len == 126){
        _pinfo.len = fdata[3] | (uint16_t)(fdata[2]) << 8;
        data += 2;
        plen -= 2;
      } else if(_pinfo.len == 127){
        _pinfo.len = fdata[9] | (uint16_t)(fdata[8]) << 8 | (uint32_t)(fdata[7]) << 16 | (uint32_t)(fdata[6]) << 24 | (uint64_t)(fdata[5]) << 32 | (uint64_t)(fdata[4]) << 40 | (uint64_t)(fdata[3]) << 48 | (uint64_t)(fdata[2]) << 56;
        data += 8;
        plen -= 8;
      }

      if(_pinfo.masked){
        memcpy(_pinfo.mask, data, 4);
        data += 4;
        plen -= 4;
      }
    }

    const size_t datalen = std::min((size_t)(_pinfo.len - _pinfo.index), plen);
    const auto datalast = data[datalen];

    if(_pinfo.masked){
      for(size_t i=0;i<datalen;i++)
        data[i] ^= _pinfo.mask[(_pinfo.index+i)%4];
    }

    if((datalen + _pinfo.index) < _pinfo.len){
      _pstate = 1;

      if(_pinfo.index == 0){
        if(_pinfo.opcode){
          _pinfo.message_opcode = _pinfo.opcode;
          _pinfo.num = 0;
        }
      }
      if (datalen > 0) _server->_handleEvent(this, WS_EVT_DATA, (void *)&_pinfo, (uint8_t*)data, datalen);

      _pinfo.index += datalen;
    } else if((datalen + _pinfo.index) == _pinfo.len){
      _pstate = 0;
      if(_pinfo.opcode == WS_DISCONNECT){
        if(datalen){
          uint16_t reasonCode = (uint16_t)(data[0] << 8) + data[1];
          char * reasonString = (char*)(data+2);
          if(reasonCode > 1001){
            _server->_handleEvent(this, WS_EVT_ERROR, (void *)&reasonCode, (uint8_t*)reasonString, strlen(reasonString));
          }
        }
        if(_status == WS_DISCONNECTING){
          _status = WS_DISCONNECTED;
          _client->close(true);
        } else {
          _status = WS_DISCONNECTING;
          _client->ackLater();
          _queueControl(WS_DISCONNECT, data, datalen);
        }
      } else if(_pinfo.opcode == WS_PING){
        _queueControl(WS_PONG, data, datalen);
      } else if(_pinfo.opcode == WS_PONG){
        if(datalen != AWSC_PING_PAYLOAD_LEN || memcmp(AWSC_PING_PAYLOAD, data, AWSC_PING_PAYLOAD_LEN) != 0)
          _server->_handleEvent(this, WS_EVT_PONG, NULL, data, datalen);
      } else if(_pinfo.opcode < 8){//continuation or text/binary frame
        _server->_handleEvent(this, WS_EVT_DATA, (void *)&_pinfo, data, datalen);
        if (_pinfo.final) _pinfo.num = 0;
        else _pinfo.num += 1;   
      }
    } else {
      //os_printf("frame error: len: %u, index: %llu, total: %llu\n", datalen, _pinfo.index, _pinfo.len);
      //what should we do?
      break;
    }

    // restore byte as _handleEvent may have added a null terminator i.e., data[len] = 0;
    if (datalen > 0)
      data[datalen] = datalast;

    data += datalen;
    plen -= datalen;
  }
}

size_t AsyncWebSocketClient::printf(const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  char* temp = new char[MAX_PRINTF_LEN];
  if(!temp){
    va_end(arg);
    return 0;
  }
  char* buffer = temp;
  size_t len = vsnprintf(temp, MAX_PRINTF_LEN, format, arg);
  va_end(arg);

  if (len > (MAX_PRINTF_LEN - 1)) {
    buffer = new char[len + 1];
    if (!buffer) {
   	  delete[] temp;
      return 0;
    }
    va_start(arg, format);
    vsnprintf(buffer, len + 1, format, arg);
    va_end(arg);
  }
  text(buffer, len);
  if (buffer != temp) {
    delete[] buffer;
  }
  delete[] temp;
  return len;
}

#ifndef ESP32
size_t AsyncWebSocketClient::printf_P(PGM_P formatP, ...)
{
  va_list arg;
  va_start(arg, formatP);
  char* temp = new char[MAX_PRINTF_LEN];
  if(!temp){
    va_end(arg);
    return 0;
  }
  char* buffer = temp;
  size_t len = vsnprintf_P(temp, MAX_PRINTF_LEN, formatP, arg);
  va_end(arg);

  if (len > (MAX_PRINTF_LEN - 1)) {
    buffer = new char[len + 1];
    if (!buffer) {
   	  delete[] temp;
      return 0;
    }
    va_start(arg, formatP);
    vsnprintf_P(buffer, len + 1, formatP, arg);
    va_end(arg);
  }
  text(buffer, len);
  if (buffer != temp) {
    delete[] buffer;
  }
  delete[] temp;
  return len;
}
#endif

namespace {
std::shared_ptr<std::vector<uint8_t>> makeSharedBuffer(const uint8_t *message, size_t len)
{
    auto buffer = std::make_shared<std::vector<uint8_t>>(len);
    std::memcpy(buffer->data(), message, len);
    return buffer;
}
}

void AsyncWebSocketClient::text(AsyncWebSocketMessageBuffer * buffer)
{
    if (buffer) {
        text(std::move(buffer->_buffer));
        delete buffer;
    }
}

void AsyncWebSocketClient::text(std::shared_ptr<std::vector<uint8_t>> buffer)
{
    _queueMessage(buffer);
}

void AsyncWebSocketClient::text(const uint8_t *message, size_t len)
{
    text(makeSharedBuffer(message, len));
}

void AsyncWebSocketClient::text(const char *message, size_t len)
{
    text((const uint8_t *)message, len);
}

void AsyncWebSocketClient::text(const char *message)
{
    text(message, strlen(message));
}

void AsyncWebSocketClient::text(const String &message)
{
    text(message.c_str(), message.length());
}

void AsyncWebSocketClient::text(const __FlashStringHelper *data)
{
    PGM_P p = reinterpret_cast<PGM_P>(data);

    size_t n = 0;
    while (1)
    {
        if (pgm_read_byte(p+n) == 0) break;
            n += 1;
    }

    char * message = (char*) malloc(n+1);
    if(message)
    {
        memcpy_P(message, p, n);
        message[n] = 0;
        text(message, n);
        free(message);
    }
}

void AsyncWebSocketClient::binary(AsyncWebSocketMessageBuffer * buffer)
{
    if (buffer) {
        binary(std::move(buffer->_buffer));
        delete buffer;
    }
}

void AsyncWebSocketClient::binary(std::shared_ptr<std::vector<uint8_t>> buffer)
{
    _queueMessage(buffer, WS_BINARY);
}

void AsyncWebSocketClient::binary(const uint8_t *message, size_t len)
{
    binary(makeSharedBuffer(message, len));
}

void AsyncWebSocketClient::binary(const char *message, size_t len)
{
    binary((const uint8_t *)message, len);
}

void AsyncWebSocketClient::binary(const char *message)
{
    binary(message, strlen(message));
}

void AsyncWebSocketClient::binary(const String &message)
{
    binary(message.c_str(), message.length());
}

void AsyncWebSocketClient::binary(const __FlashStringHelper *data, size_t len)
{
    PGM_P p = reinterpret_cast<PGM_P>(data);
    char *message = (char*) malloc(len);
    if (message) {
        memcpy_P(message, p, len);
        binary(message, len);
        free(message);
    }
}

IPAddress AsyncWebSocketClient::remoteIP() const
{
    if (!_client)
        return IPAddress((uint32_t)0U);

    return _client->remoteIP();
}

uint16_t AsyncWebSocketClient::remotePort() const
{
    if(!_client)
        return 0;

    return _client->remotePort();
}



/*
 * Async Web Socket - Each separate socket location
 */

AsyncWebSocket::AsyncWebSocket(const String& url)
  :_url(url)
  ,_cNextId(1)
  ,_enabled(true)
{
  _eventHandler = NULL;
}

AsyncWebSocket::~AsyncWebSocket(){}

void AsyncWebSocket::_handleEvent(AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len){
  if(_eventHandler != NULL){
    _eventHandler(this, client, type, arg, data, len);
  }
}

AsyncWebSocketClient *AsyncWebSocket::_newClient(AsyncWebServerRequest *request)
{
    _clients.emplace_back(request, this);
    return &_clients.back();
}

bool AsyncWebSocket::availableForWriteAll()
{
    return std::none_of(std::begin(_clients), std::end(_clients),
                      [](const AsyncWebSocketClient &c){ return c.queueIsFull(); });
}

bool AsyncWebSocket::availableForWrite(uint32_t id)
{
    const auto iter = std::find_if(std::begin(_clients), std::end(_clients),
                                    [id](const AsyncWebSocketClient &c){ return c.id() == id; });
    if (iter == std::end(_clients))
        return true;
    return !iter->queueIsFull();
}

size_t AsyncWebSocket::count() const
{
    return std::count_if(std::begin(_clients), std::end(_clients),
                          [](const AsyncWebSocketClient &c){ return c.status() == WS_CONNECTED; });
}

AsyncWebSocketClient * AsyncWebSocket::client(uint32_t id)
{
    const auto iter = std::find_if(std::begin(_clients), std::end(_clients),
                                    [id](const AsyncWebSocketClient &c){ return c.id() == id && c.status() == WS_CONNECTED; });
    if (iter == std::end(_clients))
        return nullptr;

    return &(*iter);
}


void AsyncWebSocket::close(uint32_t id, uint16_t code, const char * message)
{
    if (AsyncWebSocketClient *c = client(id))
        c->close(code, message);
}

void AsyncWebSocket::closeAll(uint16_t code, const char * message)
{
    for (auto &c : _clients)
        if (c.status() == WS_CONNECTED)
            c.close(code, message);
}

void AsyncWebSocket::cleanupClients(uint16_t maxClients)
{
    if (count() > maxClients)
        _clients.front().close();

    for (auto iter = std::begin(_clients); iter != std::end(_clients);)
    {
        if (iter->shouldBeDeleted())
            iter = _clients.erase(iter);
        else
            iter++;
    }
}

void AsyncWebSocket::ping(uint32_t id, const uint8_t *data, size_t len)
{
    if (AsyncWebSocketClient * c = client(id))
        c->ping(data, len);
}

void AsyncWebSocket::pingAll(const uint8_t *data, size_t len)
{
    for (auto &c : _clients)
        if (c.status() == WS_CONNECTED)
            c.ping(data, len);
}

void AsyncWebSocket::text(uint32_t id, const uint8_t *message, size_t len)
{
    if (AsyncWebSocketClient * c = client(id))
        c->text(makeSharedBuffer(message, len));
}
void AsyncWebSocket::text(uint32_t id, const char *message, size_t len)
{
    text(id, (const uint8_t *)message, len);
}
void AsyncWebSocket::text(uint32_t id, const char * message)
{
    text(id, message, strlen(message));
}
void AsyncWebSocket::text(uint32_t id, const String &message)
{
    text(id, message.c_str(), message.length());
}
void AsyncWebSocket::text(uint32_t id, const __FlashStringHelper *data)
{
    PGM_P p = reinterpret_cast<PGM_P>(data);

    size_t n = 0;
    while (true)
    {
        if (pgm_read_byte(p+n) == 0)
            break;
        n += 1;
    }

    char * message = (char*) malloc(n+1);
    if (message)
    {
        memcpy_P(message, p, n);
        message[n] = 0;
        text(id, message, n);
        free(message);
    }
}
void AsyncWebSocket::text(uint32_t id, AsyncWebSocketMessageBuffer *buffer) 
{
    if (buffer) {
        text(id, std::move(buffer->_buffer));
        delete buffer;
    }
}
void AsyncWebSocket::text(uint32_t id, std::shared_ptr<std::vector<uint8_t>> buffer) 
{
    if (AsyncWebSocketClient *c = client(id))
        c->text(buffer);
}

void AsyncWebSocket::textAll(const uint8_t *message, size_t len)
{
    textAll(makeSharedBuffer(message, len));
}
void AsyncWebSocket::textAll(const char * message, size_t len)
{
    textAll((const uint8_t *)message, len);
}
void AsyncWebSocket::textAll(const char *message)
{
    textAll(message, strlen(message));
}
void AsyncWebSocket::textAll(const String &message)
{
    textAll(message.c_str(), message.length());
}
void AsyncWebSocket::textAll(const __FlashStringHelper *data)
{
    PGM_P p = reinterpret_cast<PGM_P>(data);

    size_t n = 0;
    while (1)
    {
        if (pgm_read_byte(p+n) == 0) break;
            n += 1;
    }

    char *message = (char*)malloc(n+1);
    if(message)
    {
        memcpy_P(message, p, n);
        message[n] = 0;
        textAll(message, n);
        free(message);
    }
}
void AsyncWebSocket::textAll(AsyncWebSocketMessageBuffer * buffer)
{
    if (buffer) {
        textAll(std::move(buffer->_buffer));
        delete buffer;  
    }
}

void AsyncWebSocket::textAll(std::shared_ptr<std::vector<uint8_t>> buffer)
{
    for (auto &c : _clients)
        if (c.status() == WS_CONNECTED)
            c.text(buffer);
}

void AsyncWebSocket::binary(uint32_t id, const uint8_t *message, size_t len)
{
    if (AsyncWebSocketClient *c = client(id))
        c->binary(makeSharedBuffer(message, len));
}
void AsyncWebSocket::binary(uint32_t id, const char * message, size_t len)
{
    binary(id, (const uint8_t *)message, len);
}
void AsyncWebSocket::binary(uint32_t id, const char * message)
{
    binary(id, message, strlen(message));
}
void AsyncWebSocket::binary(uint32_t id, const String &message)
{
    binary(id, message.c_str(), message.length());
}
void AsyncWebSocket::binary(uint32_t id, const __FlashStringHelper *data, size_t len)
{
    PGM_P p = reinterpret_cast<PGM_P>(data);
    char *message = (char*) malloc(len);
    if (message)
    {
        memcpy_P(message, p, len);
        binary(id, message, len);
        free(message);
    }
}
void AsyncWebSocket::binary(uint32_t id, AsyncWebSocketMessageBuffer *buffer)
{
    if (buffer) {
        binary(id, std::move(buffer->_buffer));
        delete buffer;
    }
}
void AsyncWebSocket::binary(uint32_t id, std::shared_ptr<std::vector<uint8_t>> buffer)
{
    if (AsyncWebSocketClient *c = client(id))
        c->binary(buffer);
}


void AsyncWebSocket::binaryAll(const uint8_t *message, size_t len)
{
    binaryAll(makeSharedBuffer(message, len));
}
void AsyncWebSocket::binaryAll(const char *message, size_t len)
{
    binaryAll((const uint8_t *)message, len);
}
void AsyncWebSocket::binaryAll(const char *message)
{
    binaryAll(message, strlen(message));
}
void AsyncWebSocket::binaryAll(const String &message)
{
    binaryAll(message.c_str(), message.length());
}
void AsyncWebSocket::binaryAll(const __FlashStringHelper *data, size_t len)
{
    PGM_P p = reinterpret_cast<PGM_P>(data);
    char * message = (char*) malloc(len);
    if(message)
    {
        memcpy_P(message, p, len);
        binaryAll(message, len);
        free(message);
    }
}
void AsyncWebSocket::binaryAll(AsyncWebSocketMessageBuffer * buffer)
{
    if (buffer) {
        binaryAll(std::move(buffer->_buffer));
        delete buffer;
    }
}
void AsyncWebSocket::binaryAll(std::shared_ptr<std::vector<uint8_t>> buffer)
{
    for (auto &c : _clients)
        if (c.status() == WS_CONNECTED)
            c.binary(buffer);
}

size_t AsyncWebSocket::printf(uint32_t id, const char *format, ...){
    AsyncWebSocketClient * c = client(id);
    if (c)
    {
        va_list arg;
        va_start(arg, format);
        size_t len = c->printf(format, arg);
        va_end(arg);
        return len;
    }
    return 0;
}

size_t AsyncWebSocket::printfAll(const char *format, ...)
{
    va_list arg;
    char *temp = new char[MAX_PRINTF_LEN];
    if (!temp)
        return 0;

    va_start(arg, format);
    size_t len = vsnprintf(temp, MAX_PRINTF_LEN, format, arg);
    va_end(arg);
    delete[] temp;

    std::shared_ptr<std::vector<uint8_t>> buffer = std::make_shared<std::vector<uint8_t>>(len);

    va_start(arg, format);
    vsnprintf( (char *)buffer->data(), len + 1, format, arg);
    va_end(arg);

    textAll(buffer);
    return len;
}

#ifndef ESP32
size_t AsyncWebSocket::printf_P(uint32_t id, PGM_P formatP, ...){
  AsyncWebSocketClient * c = client(id);
  if(c != NULL){
    va_list arg;
    va_start(arg, formatP);
    size_t len = c->printf_P(formatP, arg);
    va_end(arg);
    return len;
  }
  return 0;
}
#endif

size_t AsyncWebSocket::printfAll_P(PGM_P formatP, ...)
{
    va_list arg;
    char *temp = new char[MAX_PRINTF_LEN];
    if (!temp)
        return 0;

    va_start(arg, formatP);
    size_t len = vsnprintf_P(temp, MAX_PRINTF_LEN, formatP, arg);
    va_end(arg);
    delete[] temp;

    std::shared_ptr<std::vector<uint8_t>> buffer = std::make_shared<std::vector<uint8_t>>(len + 1);

    va_start(arg, formatP);
    vsnprintf_P((char *)buffer->data(), len + 1, formatP, arg);
    va_end(arg);

    textAll(buffer);
    return len;
}

const char __WS_STR_CONNECTION[] PROGMEM = { "Connection" };
const char __WS_STR_UPGRADE[] PROGMEM = { "Upgrade" };
const char __WS_STR_ORIGIN[] PROGMEM = { "Origin" };
const char __WS_STR_COOKIE[] PROGMEM = { "Cookie" };
const char __WS_STR_VERSION[] PROGMEM = { "Sec-WebSocket-Version" };
const char __WS_STR_KEY[] PROGMEM = { "Sec-WebSocket-Key" };
const char __WS_STR_PROTOCOL[] PROGMEM = { "Sec-WebSocket-Protocol" };
const char __WS_STR_ACCEPT[] PROGMEM = { "Sec-WebSocket-Accept" };
const char __WS_STR_UUID[] PROGMEM = { "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" };

#define WS_STR_CONNECTION FPSTR(__WS_STR_CONNECTION)
#define WS_STR_UPGRADE FPSTR(__WS_STR_UPGRADE)
#define WS_STR_ORIGIN FPSTR(__WS_STR_ORIGIN)
#define WS_STR_COOKIE FPSTR(__WS_STR_COOKIE)
#define WS_STR_VERSION FPSTR(__WS_STR_VERSION)
#define WS_STR_KEY FPSTR(__WS_STR_KEY)
#define WS_STR_PROTOCOL FPSTR(__WS_STR_PROTOCOL)
#define WS_STR_ACCEPT FPSTR(__WS_STR_ACCEPT)
#define WS_STR_UUID FPSTR(__WS_STR_UUID)

bool AsyncWebSocket::canHandle(AsyncWebServerRequest *request){
    if(!_enabled)
        return false;

    if(request->method() != HTTP_GET || !request->url().equals(_url) || !request->isExpectedRequestedConnType(RCT_WS))
        return false;

    request->addInterestingHeader(WS_STR_CONNECTION);
    request->addInterestingHeader(WS_STR_UPGRADE);
    request->addInterestingHeader(WS_STR_ORIGIN);
    request->addInterestingHeader(WS_STR_COOKIE);
    request->addInterestingHeader(WS_STR_VERSION);
    request->addInterestingHeader(WS_STR_KEY);
    request->addInterestingHeader(WS_STR_PROTOCOL);
    return true;
}

void AsyncWebSocket::handleRequest(AsyncWebServerRequest *request)
{
    if (!request->hasHeader(WS_STR_VERSION) || !request->hasHeader(WS_STR_KEY))
    {
        request->send(400);
        return;
    }
    if ((_username.length() && _password.length()) && !request->authenticate(_username.c_str(), _password.c_str()))
    {
        return request->requestAuthentication();
    }
    if (_handshakeHandler != nullptr){
        if(!_handshakeHandler(request)){
            request->send(401);
            return;
        }
    }
    AsyncWebHeader* version = request->getHeader(WS_STR_VERSION);
    if (version->value().toInt() != 13)
    {
        AsyncWebServerResponse *response = request->beginResponse(400);
        response->addHeader(WS_STR_VERSION, F("13"));
        request->send(response);
        return;
    }
    AsyncWebHeader* key = request->getHeader(WS_STR_KEY);
    AsyncWebServerResponse *response = new AsyncWebSocketResponse(key->value(), this);
    if (request->hasHeader(WS_STR_PROTOCOL))
    {
        AsyncWebHeader* protocol = request->getHeader(WS_STR_PROTOCOL);
        //ToDo: check protocol
        response->addHeader(WS_STR_PROTOCOL, protocol->value());
    }
    request->send(response);
}

AsyncWebSocketMessageBuffer * AsyncWebSocket::makeBuffer(size_t size)
{
    AsyncWebSocketMessageBuffer * buffer = new AsyncWebSocketMessageBuffer(size);
    if (buffer->length() != size)
    {
        delete buffer;
        return nullptr;
    } else {
        return buffer;
    }
}

AsyncWebSocketMessageBuffer * AsyncWebSocket::makeBuffer(uint8_t * data, size_t size)
{
    AsyncWebSocketMessageBuffer * buffer = new AsyncWebSocketMessageBuffer(data, size);
    if (buffer->length() != size)
    {
        delete buffer;
        return nullptr;
    } else {
        return buffer;
    }
}

/*
 * Response to Web Socket request - sends the authorization and detaches the TCP Client from the web server
 * Authentication code from https://github.com/Links2004/arduinoWebSockets/blob/master/src/WebSockets.cpp#L480
 */

AsyncWebSocketResponse::AsyncWebSocketResponse(const String& key, AsyncWebSocket *server)
{
    _server = server;
    _code = 101;
    _sendContentLength = false;

    uint8_t * hash = (uint8_t*)malloc(20);
    if(hash == NULL)
    {
        _state = RESPONSE_FAILED;
        return;
    }
    char * buffer = (char *) malloc(33);
    if(buffer == NULL)
    {
        free(hash);
        _state = RESPONSE_FAILED;
        return;
    }
#ifdef ESP8266
    sha1(key + WS_STR_UUID, hash);
#else
    (String&)key += WS_STR_UUID;
    mbedtls_sha1_context ctx;
    mbedtls_sha1_init(&ctx);
#if ESP_IDF_VERSION_MAJOR == 5
    mbedtls_sha1_starts(&ctx);
    mbedtls_sha1_update(&ctx, (const unsigned char*)key.c_str(), key.length());
    mbedtls_sha1_finish(&ctx, hash);
#else
    mbedtls_sha1_starts_ret(&ctx);
    mbedtls_sha1_update_ret(&ctx, (const unsigned char*)key.c_str(), key.length());
    mbedtls_sha1_finish_ret(&ctx, hash);
#endif
    mbedtls_sha1_free(&ctx);
#endif
    base64_encodestate _state;
    base64_init_encodestate(&_state);
    int len = base64_encode_block((const char *) hash, 20, buffer, &_state);
    len = base64_encode_blockend((buffer + len), &_state);
    addHeader(WS_STR_CONNECTION, WS_STR_UPGRADE);
    addHeader(WS_STR_UPGRADE, F("websocket"));
    addHeader(WS_STR_ACCEPT,buffer);
    free(buffer);
    free(hash);
}

void AsyncWebSocketResponse::_respond(AsyncWebServerRequest *request)
{
    if(_state == RESPONSE_FAILED)
    {
        request->client()->close(true);
        return;
    }
    String out = _assembleHead(request->version());
    request->client()->write(out.c_str(), _headLength);
    _state = RESPONSE_WAIT_ACK;
}

size_t AsyncWebSocketResponse::_ack(AsyncWebServerRequest *request, size_t len, uint32_t time)
{
    (void)time;

    if(len)
        _server->_newClient(request);

    return 0;
}
