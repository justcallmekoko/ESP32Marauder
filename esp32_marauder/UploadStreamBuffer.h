#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

namespace marauder {

constexpr size_t UPLOAD_STREAM_BUFFER_SIZE = 1024;

class UploadStreamBuffer {
 public:
  UploadStreamBuffer()
      : data_(static_cast<uint8_t*>(malloc(UPLOAD_STREAM_BUFFER_SIZE))) {}

  ~UploadStreamBuffer() {
    free(data_);
  }

  UploadStreamBuffer(const UploadStreamBuffer&) = delete;
  UploadStreamBuffer& operator=(const UploadStreamBuffer&) = delete;

  uint8_t* data() const {
    return data_;
  }

  size_t size() const {
    return UPLOAD_STREAM_BUFFER_SIZE;
  }

  explicit operator bool() const {
    return data_ != nullptr;
  }

 private:
  uint8_t* data_;
};

}  // namespace marauder
