#pragma once

#include <Stream.h>

class StreamConcat : public Stream {
  public:
    StreamConcat(Stream* s1, Stream* s2) : _s1(s1), _s2(s2) {}

    size_t write(__unused const uint8_t* p, __unused size_t n) override { return 0; }
    size_t write(__unused uint8_t c) override { return 0; }
    void flush() override {}

    int available() override { return _s1->available() + _s2->available(); }

    int read() override {
      int c = _s1->read();
      return c != -1 ? c : _s2->read();
    }

    size_t readBytes(char* buffer, size_t length) override {
      size_t count = _s1->readBytes(buffer, length);
      return count > 0 ? count : _s2->readBytes(buffer, length);
    }

    int peek() override {
      int c = _s1->peek();
      return c != -1 ? c : _s2->peek();
    }

  private:
    Stream* _s1;
    Stream* _s2;
};
