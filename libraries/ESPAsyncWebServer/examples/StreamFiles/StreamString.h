#pragma once

#include <Stream.h>

class StreamString : public Stream {
  public:
    size_t write(const uint8_t* p, size_t n) override { return _buffer.concat(reinterpret_cast<const char*>(p), n) ? n : 0; }
    size_t write(uint8_t c) override { return _buffer.concat(static_cast<char>(c)) ? 1 : 0; }
    void flush() override {}

    int available() override { return static_cast<int>(_buffer.length()); }

    int read() override {
      if (_buffer.length() == 0)
        return -1;
      char c = _buffer[0];
      _buffer.remove(0, 1);
      return c;
    }

    size_t readBytes(char* buffer, size_t length) override {
      if (length > _buffer.length())
        length = _buffer.length();
      // Don't use _str.ToCharArray() because it inserts a terminator
      memcpy(buffer, _buffer.c_str(), length);
      _buffer.remove(0, static_cast<unsigned int>(length));
      return length;
    }

    int peek() override { return _buffer.length() > 0 ? _buffer[0] : -1; }

    const String& buffer() const { return _buffer; }

  private:
    String _buffer;
};
