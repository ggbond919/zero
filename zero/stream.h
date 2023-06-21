#ifndef __ZERO_STREAM_H__
#define __ZERO_STREAM_H__

#include <cstddef>
#include <memory>
#include "bytearray.h"

namespace zero {

class Stream {
public:
    typedef std::shared_ptr<Stream> ptr;

    virtual ~Stream() {}

    virtual int read(void* buffer, size_t length) = 0;

    virtual int read(ByteArray::ptr ba, size_t length) = 0;

    virtual int readFixSize(void* buffer, size_t length);

    virtual int readFixSize(ByteArray::ptr ba, size_t length);

    virtual int write(const void* buffer, size_t length) = 0;

    virtual int write(ByteArray::ptr ba, size_t length) = 0;

    virtual int writeFixSize(const void* buffer, size_t length);

    virtual int writeFixSize(ByteArray::ptr ba, size_t length);

    virtual void close() = 0;
};

}


#endif