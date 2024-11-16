#ifndef ASYNCWEBSYNCHRONIZATION_H_
#define ASYNCWEBSYNCHRONIZATION_H_

// Synchronisation is only available on ESP32, as the ESP8266 isn't using FreeRTOS by default

#include <ESPAsyncWebServer.h>

#ifdef ESP32

// This is the ESP32 version of the Sync Lock, using the FreeRTOS Semaphore
// Modified 'AsyncWebLock' to just only use mutex since pxCurrentTCB is not
// always available. According to example by Arjan Filius, changed name,
// added unimplemented version for ESP8266
class AsyncPlainLock
{
private:
  SemaphoreHandle_t _lock;

public:
  AsyncPlainLock() {
    _lock = xSemaphoreCreateBinary();
    // In this fails, the system is likely that much out of memory that
    // we should abort anyways. If assertions are disabled, nothing is lost..
    assert(_lock);
    xSemaphoreGive(_lock);
  }

  ~AsyncPlainLock() {
    vSemaphoreDelete(_lock);
  }

  bool lock() const {
      xSemaphoreTake(_lock, portMAX_DELAY);
      return true;
  }

  void unlock() const {
    xSemaphoreGive(_lock);
  }
};

// This is the ESP32 version of the Sync Lock, using the FreeRTOS Semaphore
class AsyncWebLock
{
private:
    SemaphoreHandle_t _lock;
    mutable TaskHandle_t _lockedBy{};

public:
    AsyncWebLock()
    {
        _lock = xSemaphoreCreateBinary();
        // In this fails, the system is likely that much out of memory that
        // we should abort anyways. If assertions are disabled, nothing is lost..
        assert(_lock);
        _lockedBy = NULL;
        xSemaphoreGive(_lock);
    }

    ~AsyncWebLock() {
        vSemaphoreDelete(_lock);
    }

    bool lock() const {
        const auto currentTask = xTaskGetCurrentTaskHandle();
        if (_lockedBy != currentTask) {
            xSemaphoreTake(_lock, portMAX_DELAY);
            _lockedBy = currentTask;
            return true;
        }
        return false;
    }

    void unlock() const {
        _lockedBy = NULL;
        xSemaphoreGive(_lock);
    }
};

#else

// This is the 8266 version of the Sync Lock which is currently unimplemented
class AsyncWebLock
{

public:
  AsyncWebLock() {
  }

  ~AsyncWebLock() {
  }

  bool lock() const {
    return false;
  }

  void unlock() const {
  }
};

// Same for AsyncPlainLock, for ESP8266 this is just the unimplemented version above. 
using AsyncPlainLock = AsyncWebLock;

#endif

class AsyncWebLockGuard
{
private:
  const AsyncWebLock *_lock;

public:
  AsyncWebLockGuard(const AsyncWebLock &l) {
    if (l.lock()) {
      _lock = &l;
    } else {
      _lock = NULL;
    }
  }

  ~AsyncWebLockGuard() {
    if (_lock) {
      _lock->unlock();
    }
  }

  void unlock() {
    if (_lock) {
      _lock->unlock();
      _lock = NULL;
    }
  }
};

#endif // ASYNCWEBSYNCHRONIZATION_H_
